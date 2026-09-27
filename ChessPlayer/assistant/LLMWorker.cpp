#include <QDebug>
#include <vector>
#include "LLMWorker.h"

LLMWorker::LLMWorker(QObject *parent)
    : QObject(parent)
{
    m_mutex = new QMutex;
    m_pauseCond = new QWaitCondition;
}

LLMWorker::~LLMWorker() {
    if (m_ctx) llama_free(m_ctx);
    if (m_model) llama_free_model(m_model);
    llama_backend_free();
    // unique_ptr automatically cleans up m_recognizer memory allocations
}

void dummy_llama_log_callback(ggml_log_level level, const char * text, void * user_data) {
    (void)level;     // Unused
    (void)text;      // Unused
    (void)user_data; // Unused
}

void LLMWorker::initializeLlama() {
    llama_log_set(dummy_llama_log_callback, nullptr);
    llama_backend_init();
    m_model = llama_load_model_from_file(m_llmModelPath.toStdString().c_str(),
                                         llama_model_default_params());
    if (!m_model) {
        qDebug("Failed to find Llama GGUF model path[%s]",
               m_llmModelPath.toStdString().c_str());
        return;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 516;
    ctx_params.n_threads = 2;
    m_ctx = llama_new_context_with_model(m_model, ctx_params);
}

void LLMWorker::stop() {
    m_interrupted.storeRelease(1);
    m_nextState = LLM_PROCESSING_EXIT;
    m_state = m_nextState;
    m_stopped = true;
    togglePause(false);
}

void LLMWorker::togglePause(bool paused)
{
    if(paused == true){
        m_mutex->lock();
        m_pause = true;
        m_mutex->unlock();
    }else{
        m_mutex->lock();
        m_pause = false;
        m_mutex->unlock();
        m_pauseCond->wakeAll();
    }
}

void LLMWorker::doWork() {
    qDebug("LLMWorker Dowork");
    m_stopped = false;
    m_state = LLM_INIT;
    m_nextState = LLM_INIT;
    while(!m_stopped){
        m_mutex->lock();
        if(m_pause)
            m_pauseCond->wait(m_mutex);
        m_mutex->unlock();
        if(m_nextState != m_state && m_state != LLM_WAITING) {
            m_state = m_nextState;
        }
        switch (m_state) {
        case LLM_INIT: {
            m_state = LLM_WAITING;
        }
            break;
        case LLM_WAITING: {
            QThread::msleep(30);
        }
            break;
        case LLM_TRANSCRIBE :{
            int transribeResult = transcribeAudio();
            if(transribeResult == LLM_DONE_SUCCESS) {
                m_nextState = LLM_PROCESSING;
            } else if(transribeResult == LLM_DONE_FAILED) {
                m_state = LLM_WAITING;
            }
        }
            break;
        case LLM_PROCESSING :{
            int llamaResult = runLlamaInference();
            if(llamaResult == LLM_DONE_SUCCESS) {
                m_state = LLM_WAITING;
            } else if(llamaResult == LLM_DONE_FAILED) {
                m_state = LLM_WAITING;
            }
        }
            break;
        case LLM_PROCESSING_EXIT :{
            m_stopped = true;
        }
            break;
        }
    }
    qDebug("LLMWorker Dowork finished");
}

void LLMWorker::handleSpeech(const QByteArray& pcmData) {
    qDebug("LLMWorker handleSpeech [%d] bytes m_state[%d]",pcmData.size(),m_state);
    if(m_state != LLM_TRANSCRIBE) {
        if(m_state == LLM_PROCESSING)
            requestInterruption();
        togglePause(true);
        m_pcmData = pcmData;
        m_pcmData.detach();
        m_mutex->lock();
        m_nextState = LLM_TRANSCRIBE;
        if(m_state == LLM_WAITING) m_state = m_nextState;
        m_mutex->unlock();
        togglePause(false);
        qDebug("LLMWorker handleSpeech m_state[%d] m_nextState[%d]",
               m_state,m_nextState);
    }
}

QString LLMWorker::generatePromptChat(const QString& userPrompt) {
    m_conversationHistory.push_back({"user", userPrompt});

    while (m_conversationHistory.size() > 3) {
        m_conversationHistory.erase(m_conversationHistory.begin());
    }

    QString systemContent =
            "You are "+m_name+"."
                              "Adhere strictly to these rules:\n"
                              "- If you do not know the answer to a question, say 'Sorry. I don't know' instead of making up facts.\n"
                              "- Keep your responses brief, concise, and focused on the core answer.\n"
                              "- Do not repeat yourself or loop the same sentence structural phrases.";

    QString fullPrompt = "<|im_start|>system\n" + systemContent + "<|im_end|>\n";

    for (const auto& msg : m_conversationHistory) {
        fullPrompt += "<|im_start|>" + msg.role + "\n" + msg.content + "<|im_end|>\n";
    }

    fullPrompt += "<|im_start|>assistant\n";
    return fullPrompt;
}

QString LLMWorker::generatePromptChess(QString fen, QString playColor, QString move) {
    QString fullPrompt = "<|im_start|>system\n"
                         "You are an expert chess grandmaster. Analyze the given move based on the FEN board state. "
                         "Explain the strategic intent, tactical implications, and whether it is a standard book move.\n"
                         "<|im_end|>\n"
                         "<|im_start|>user\n"
                         "FEN State: " + fen + "\n"
                                               "Color: " + playColor + "\n"
                                                                       "Move played: " + move + "\n"
                                                                                                "Provide one sentence, maximum 10 words analysis on this move.<|im_end|>\n"
                                                                                                "<|im_start|>assistant\n";
    return fullPrompt;
}

int LLMWorker::handlePrompt(const QString& prompt) {
    togglePause(true);
    m_mutex->lock();
    m_userPrompt = prompt;
    m_fullPrompt = generatePromptChat(prompt);
    m_nextState = LLM_PROCESSING;
    if(m_state == LLM_WAITING) m_state = m_nextState;
    m_mutex->unlock();
    togglePause(false);
    return 0;
}

int LLMWorker::analyzeChessMove(QString fen, QString playColor, QString move) {
    togglePause(true);
    m_mutex->lock();
    m_userPrompt =
            m_fullPrompt = generatePromptChess(fen, playColor, move);
    m_nextState = LLM_PROCESSING;
    if(m_state == LLM_WAITING) m_state = m_nextState;
    m_mutex->unlock();
    togglePause(false);
    return 0;
}

void LLMWorker::requestInterruption() {
    m_interrupted.storeRelease(1);
}

void LLMWorker::setModel(const QString& name,
                         const QString& sherpaModelPath,
                         const QString& sherpaTokensPath,
                         const QString& llmModelPath) {
    m_name = name;
    m_sherpaModelPath = sherpaModelPath;
    m_sherpaTokensPath = sherpaTokensPath;
    m_llmModelPath = llmModelPath;
}

void LLMWorker::initializeSherpaOnnx() {
    if (m_sherpaModelPath.isEmpty() || m_sherpaTokensPath.isEmpty()) {
        qDebug("Sherpa-onnx model paths are unassigned.");
        return;
    }

    // Clean up any historical instance if initializing a second time
    if (m_recognizer) {
        delete m_recognizer;
        m_recognizer = nullptr;
    }

    sherpa_onnx::cxx::OfflineRecognizerConfig config;
    config.model_config.sense_voice.model = m_sherpaModelPath.toStdString();
    config.model_config.sense_voice.language = "en";
    config.model_config.sense_voice.use_itn = true;
    config.model_config.tokens = m_sherpaTokensPath.toStdString();
    config.model_config.num_threads = 4;
    config.decoding_method = "greedy_search";

    try {
        // Allocate via new using the factory return value copy constructor
        m_recognizer = new sherpa_onnx::cxx::OfflineRecognizer(
            sherpa_onnx::cxx::OfflineRecognizer::Create(config)
        );

        if (m_recognizer && m_recognizer->Get()) {
            qDebug("Sherpa-onnx SenseVoice Recognizer created successfully on Heap.");
        } else {
            qDebug("Failed to create Sherpa-onnx Recognizer handle runtime mapping.");
        }
    } catch (const std::exception& e) {
        qDebug("Exception during Sherpa allocation: %s", e.what());
    }
}


int LLMWorker::transcribeAudio() {
    // Access validation pointer directly via arrow
    if (!m_recognizer || !m_recognizer->Get() || m_pcmData.isEmpty()) {
        qDebug() << "Sherpa Recognizer not initialized or PCM data payload is empty.";
        return LLM_DONE_FAILED;
    }

    qDebug() << "transcribeAudio via Sherpa-onnx: " << m_pcmData.size() << "bytes";

    const int16_t* samples = reinterpret_cast<const int16_t*>(m_pcmData.constData());
    int sampleCount = m_pcmData.size() / sizeof(int16_t);

    std::vector<float> sherpaSamples(sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        sherpaSamples[i] = samples[i] / 32768.0f;
    }

    try {
        // Access methods via arrow (->)
        sherpa_onnx::cxx::OfflineStream stream = m_recognizer->CreateStream();

        stream.AcceptWaveform(16000, sherpaSamples.data(), sherpaSamples.size());

        m_recognizer->Decode(&stream);

        sherpa_onnx::cxx::OfflineRecognizerResult result = m_recognizer->GetResult(&stream);
        std::string textResult = result.text;

        QString parsedPrompt = QString::fromUtf8(textResult.c_str()).trimmed();
        qDebug() << "Sherpa-Onnx Transcribed Result:" << parsedPrompt;

        if (!parsedPrompt.isEmpty() && parsedPrompt.length() >= 1 &&
            !parsedPrompt.contains("[") && !parsedPrompt.contains("]") &&
            !parsedPrompt.contains("(") && !parsedPrompt.contains(")") &&
            !parsedPrompt.contains("*")) {

            m_userPrompt = parsedPrompt;
            return LLM_DONE_SUCCESS;
        }
    } catch (const std::exception& e) {
        qDebug() << "Exception inside evaluation pipeline framework step:" << e.what();
    }

    return LLM_DONE_FAILED;
}

int LLMWorker::runLlamaInference() {
    int nextState = LLM_PENDING;
    Q_EMIT tokenGenerated("");
    m_fullPrompt = generatePromptChat(m_userPrompt);
    if(m_userPrompt.isEmpty() || m_fullPrompt.isEmpty()) return LLM_DONE_FAILED;

    // 3. Get Vocabulary Pointer
    const struct llama_vocab* vocab = llama_model_get_vocab(m_model);
    if (!vocab) {
        nextState = LLM_DONE_FAILED;
        return nextState;
    }

    // 4. Tokenize the entire consolidated clean prompt window
    std::vector<llama_token> tokens;
    tokens.resize(m_fullPrompt.size() + 4);
    int n_tokens = llama_tokenize(vocab, m_fullPrompt.toStdString().c_str(),
                                  m_fullPrompt.size(),
                                  tokens.data(), tokens.size(), true, true);
    tokens.resize(n_tokens);

    // HARD RESET THE KV GRAPH FOR CLEAN EVALUATION
    // Wiping the graph and re-evaluating the short prompt prevents attention dilution!
    llama_memory_t mem = llama_get_memory(m_ctx);
    llama_memory_seq_rm(mem, 0, 0, -1);
    m_pastTokensCount = 0;

    // 5. Initialize a Modern Batch Allocation Structure
    llama_batch batch = llama_batch_init(tokens.size(), 0, 1);
    batch.n_tokens = tokens.size();
    for (int i = 0; i < batch.n_tokens; ++i) {
        batch.token[i]    = tokens[i];
        batch.pos[i]      = m_pastTokensCount + i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i]   = false;
    }
    // Instruct the engine to only compute logit metrics for the absolute last token
    batch.logits[batch.n_tokens - 1] = true;

    // Evaluate the initial prompt batch chunk
    if (llama_decode(m_ctx, batch) != 0) {
        qWarning() << "Failed to decode context evaluation batch.";
        llama_batch_free(batch);
        return LLM_DONE_FAILED;
    }

    // Update tracking marker to the position where prompt decode finished
    m_pastTokensCount += batch.n_tokens;

    // 6. INITIALIZE HIGH-PENALTY SAMPLER CHAIN
    int32_t vocab_size = llama_n_vocab((const struct llama_vocab *)m_model);
    struct llama_sampler * smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());

    // Step A: Apply repetition penalties to stop looping phrases
    llama_sampler_chain_add(smpl, llama_sampler_init_penalties(vocab_size, 64, 1.25f, 0.15f, 0.15f));

    // Step B: Prune out low-scoring garbage tokens (keeps logic sharp like llama-cli)
    llama_sampler_chain_add(smpl, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(smpl, llama_sampler_init_top_p(0.95f, 1)); // min_keep=1 ensures at least 1 token is preserved

    // Step C: Scale the logits using temperature
    // (Crucial: This converts raw scores into valid mathematical probabilities right before distribution mapping!)
    llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.7f));

    // Step D: Apply the randomized distribution sampler with a fallback seed
    // Using a non-zero integer or a timestamp (like time(NULL)) ensures natural phrasing distribution.
    llama_sampler_chain_add(smpl, llama_sampler_init_dist(1234));

    QString final_output = "";
    int max_new_tokens = 30;
    nextState = LLM_DONE_SUCCESS;
    // 7. Generation Loop (Token by Token generation)
    for (int i = 0; i < max_new_tokens; i++) {
        if (m_interrupted.loadAcquire() == 1) {
            Q_EMIT tokenGenerated("... [Interrupted]");
            nextState = LLM_DONE_INTERRUPT;
            m_interrupted.storeRelease(0);
            break; // Break the execution loop instantly
        }
        llama_token curr_token = llama_sampler_sample(smpl, m_ctx, -1);

        // Break instantly if model hits an end-of-generation structural barrier
        if (llama_vocab_is_eog(vocab, curr_token)) {
            break;
        }

        llama_sampler_accept(smpl, curr_token);

        std::vector<char> piece_buf(32);
        int n_chars = llama_token_to_piece(vocab, curr_token, piece_buf.data(), piece_buf.size(), 0, true);
        if (n_chars < 0) {
            piece_buf.resize(-n_chars);
            n_chars = llama_token_to_piece(vocab, curr_token, piece_buf.data(), piece_buf.size(), 0, true);
        }

        if (n_chars > 0) {
            std::string piece(piece_buf.data(), n_chars);

            // Fail-safe cut-off for custom Qwen text blocks
            if (piece.find("<|im_end|>") != std::string::npos) {
                break;
            }

            final_output += QString::fromStdString(piece);
#ifdef DEBUG_LLM
            qDebug() << "Response[" <<i << "]: "<< QString::fromStdString(piece);
#endif
            Q_EMIT tokenGenerated(QString::fromStdString(piece));
        }

        // SAFE GENERATION COUNTER ALIGNMENT
        batch.n_tokens = 1;
        batch.token[0] = curr_token;
        batch.pos[0]   = m_pastTokensCount;
        batch.n_seq_id[0] = 1;
        batch.seq_id[0][0] = 0;
        batch.logits[0]   = true;

        if (llama_decode(m_ctx, batch) != 0) {
            break;
        }

        // Increment the tracker index sequentially per frame choice
        m_pastTokensCount++;
    }

    // 8. Push the completed assistant response into the conversation vector map history
    m_conversationHistory.push_back({"assistant", final_output});
    // Clean up memory structures natively
    llama_sampler_free(smpl);
    llama_batch_free(batch);

    Q_EMIT generationFinished(final_output);
    return nextState;
}
