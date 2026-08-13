#include "AssistantController.h"
#include <QDebug>
#include <cmath>

// ============================================================================
// WORKER IMPLEMENTATION
// ============================================================================
PrivateAudioModelWorker::~PrivateAudioModelWorker() {
    if (m_ctx) llama_free(m_ctx);
    if (m_model) llama_free_model(m_model);
    llama_backend_free();
    if (m_whisperCtx) whisper_free(m_whisperCtx);
}

void PrivateAudioModelWorker::initService() {
    llama_backend_init();
    initializeLlama();
    initializeWhisper();
    initializeAudio();
}

void PrivateAudioModelWorker::handleManualPrompt(const QString &prompt) {
    if (m_audioInput) m_audioInput->suspend();
    runLlamaInference(prompt);
}

void PrivateAudioModelWorker::initializeLlama() {
    m_model = llama_load_model_from_file(".\\qwen2.5-1.5b-instruct-q4_k_m.gguf", llama_model_default_params());
    if (!m_model) { qWarning() << "Failed to find Llama GGUF model path."; return; }

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 2048;
    ctx_params.n_threads = QThread::idealThreadCount();
    m_ctx = llama_new_context_with_model(m_model, ctx_params);
}

void PrivateAudioModelWorker::initializeWhisper() {
    m_whisperCtx = whisper_init_from_file("ggml-base.en.bin");
    if (!m_whisperCtx) { qWarning() << "Failed to find Whisper BIN model path."; return; }
    m_whisperParams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    m_whisperParams.language = "en";
}

void PrivateAudioModelWorker::initializeAudio() {
    QAudioFormat format;
    format.setSampleRate(16000); format.setChannelCount(1); format.setSampleSize(16);
    format.setCodec("audio/pcm"); format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    m_audioInput = new QAudioInput(QAudioDeviceInfo::defaultInputDevice(), format, this);
    m_audioIOStream = m_audioInput->start();

    if (m_audioIOStream) {
        connect(m_audioIOStream, &QIODevice::readyRead, this, &PrivateAudioModelWorker::processIncomingAudio);
        Q_EMIT isListeningChanged(true);
        qDebug() << "Microphone auto-monitoring is active.";
    } else {
        qWarning() << "Audio hardware input stream failed to open.";
    }
}

void PrivateAudioModelWorker::processIncomingAudio() {
    QByteArray freshBytes = m_audioIOStream->readAll();
    if (freshBytes.isEmpty()) return;

    const int16_t* samples = reinterpret_cast<const int16_t*>(freshBytes.constData());
    int sampleCount = freshBytes.size() / sizeof(int16_t);

    float sumSquares = 0.0f;
    for (int i = 0; i < sampleCount; ++i) {
        float normalized = samples[i] / 32768.0f;
        sumSquares += normalized * normalized;
    }
    float currentVolume = std::sqrt(sumSquares / sampleCount);

    if (currentVolume > 0.015f) { // Active Speech Threshold
        if (!m_isSpeaking) {
            qDebug() << "Speech detected. Recording...";
            m_isSpeaking = true;
            m_accumulatedPcmData.clear();
        }
        m_accumulatedPcmData.append(freshBytes);
        m_consecutiveSilenceSamples = 0;
    } else {
        if (m_isSpeaking) {
            m_accumulatedPcmData.append(freshBytes);
            m_consecutiveSilenceSamples += sampleCount;

            // 1.5 Seconds of Silence Cutoff (16 samples per millisecond)
            if ((m_consecutiveSilenceSamples / 16) >= 1500) {
                qDebug() << "Speech finished. Starting processing pipeline.";
                m_isSpeaking = false;
                m_consecutiveSilenceSamples = 0;
                handleSpeechFinished();
            }
        }
    }
}

void PrivateAudioModelWorker::handleSpeechFinished() {
    m_audioInput->suspend();
    Q_EMIT isThinkingChanged(true);

    const int16_t* samples = reinterpret_cast<const int16_t*>(m_accumulatedPcmData.constData());
    int sampleCount = m_accumulatedPcmData.size() / sizeof(int16_t);
    if (sampleCount == 0) { m_audioInput->resume(); Q_EMIT isThinkingChanged(false); return; }

    QVector<float> whisperSamples(sampleCount);
    for (int i = 0; i < sampleCount; ++i) whisperSamples[i] = samples[i] / 32768.0f;

    if (whisper_full(m_whisperCtx, m_whisperParams, whisperSamples.constData(), whisperSamples.size()) == 0) {
        std::string textResult = "";
        int n_segments = whisper_full_n_segments(m_whisperCtx);
        for (int i = 0; i < n_segments; ++i) textResult += whisper_full_get_segment_text(m_whisperCtx, i);

        QString parsedPrompt = QString::fromStdString(textResult).trimmed();
        qDebug() << "Whisper Transcribed:" << parsedPrompt;
        if (!parsedPrompt.isEmpty()) {
            runLlamaInference(parsedPrompt);
            return;
        }
    }

    m_audioInput->resume();
    Q_EMIT isThinkingChanged(false);
}

void PrivateAudioModelWorker::runLlamaInference(const QString &prompt) {
    Q_EMIT tokenGenerated("");

    // --- Modern Stream Token Loop ---
    QString simulatedOutput = "Answer to: " + prompt + "\nThis unified file approach runs beautifully.";
    QStringList chunks = simulatedOutput.split(" ");
    QString currentResponse = "";

    // 1. CHAT HISTORY MANAGEMENT
    // Push the new user turn into your structural tracking history vector
    m_conversationHistory.push_back({"user", prompt.toStdString()});

    // Safety Sliding Door: Limit history to the last 8 turns so the 0.5B model doesn't drop in intelligence
    while (m_conversationHistory.size() > 8) {
        m_conversationHistory.erase(m_conversationHistory.begin());
    }

    // 2. FULL GENERAL-PURPOSE PROMPT CONSTRUCTION
    std::string system_content =
        "You are a helpful, polite, and highly intelligent AI assistant. "
        "Your task is to provide accurate, clear, and direct answers. "
        "Adhere strictly to these rules:\n"
        "- If you do not know the answer to a question, say 'I don't know' instead of making up facts.\n"
        "- Keep your responses brief, concise, and focused on the core answer.\n"
        "- Do not repeat yourself or loop the same sentence structural phrases.";

    // Anchor the system instructions at the top of the context block
    std::string full_prompt = "<|im_start|>system\n" + system_content + "<|im_end|>\n";

    // Append previous back-and-forth conversational steps sequentially
    for (const auto& msg : m_conversationHistory) {
        full_prompt += "<|im_start|>" + msg.role + "\n" + msg.content + "<|im_end|>\n";
    }

    // Open the final structural lane for Qwen to generate text
    full_prompt += "<|im_start|>assistant\n";

    // 3. Get Vocabulary Pointer
    const struct llama_vocab* vocab = llama_model_get_vocab(m_model);
    if (!vocab) {
        QMetaObject::invokeMethod(this, [this]() {
            Q_EMIT isThinkingChanged(false);
        }, Qt::QueuedConnection);
        return;
    }

    // 4. Tokenize the entire consolidated clean prompt window
    std::vector<llama_token> tokens;
    tokens.resize(full_prompt.size() + 4);
    int n_tokens = llama_tokenize(vocab, full_prompt.c_str(), full_prompt.size(),
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
        Q_EMIT isThinkingChanged(false);
        return;
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

    std::string final_output = "";
    int max_new_tokens = 150;

    // 7. Generation Loop (Token by Token generation)
    for (int i = 0; i < max_new_tokens; i++) {
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

            final_output += piece;
            qDebug() << "Response: " << QString::fromStdString(piece);

            Q_EMIT tokenGenerated(QString::fromStdString(final_output));
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
    currentResponse = QString::fromStdString(final_output);
    // Clean up memory structures natively
    llama_sampler_free(smpl);
    llama_batch_free(batch);

    Q_EMIT generationFinished(currentResponse);
    m_accumulatedPcmData.clear();
    Q_EMIT isThinkingChanged(false);
    m_audioInput->resume(); // Listen again automatically
}

// ============================================================================
// MAIN CONTROLLER INTERFACE IMPLEMENTATION
// ============================================================================
AssistantController::AssistantController(QObject *parent) : QObject(parent) {
    m_workerThread = new QThread(this);
    d_worker = new PrivateAudioModelWorker();

    d_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, d_worker, &PrivateAudioModelWorker::initService);

    connect(d_worker, &PrivateAudioModelWorker::isListeningChanged, this, [this](bool listening) {
        m_isListening = listening; Q_EMIT isListeningChanged();
    });
    connect(d_worker, &PrivateAudioModelWorker::isThinkingChanged, this, [this](bool thinking) {
        m_isThinking = thinking; Q_EMIT isThinkingChanged();
    });
    connect(d_worker, &PrivateAudioModelWorker::tokenGenerated, this, [this](const QString &text) {
        m_responseText = text; Q_EMIT responseTextChanged(text);
    });
    connect(d_worker, &PrivateAudioModelWorker::generationFinished, this, [this](const QString &finalText) {
        Q_EMIT generationFinished(finalText);
    });

    connect(m_workerThread, &QThread::finished, d_worker, &QObject::deleteLater);
}

AssistantController::~AssistantController() {
    stopService();
}

void AssistantController::startService() {
    if (!m_workerThread->isRunning()) {
        m_workerThread->start();
    }
}

void AssistantController::stopService() {
    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void AssistantController::generateResponse(const QString &prompt) {
    if (m_isThinking || prompt.isEmpty()) return;

    m_isThinking = true;
    Q_EMIT isThinkingChanged();

    QMetaObject::invokeMethod(d_worker, [this, prompt]() {
        d_worker->handleManualPrompt(prompt);
    }, Qt::QueuedConnection);
}
