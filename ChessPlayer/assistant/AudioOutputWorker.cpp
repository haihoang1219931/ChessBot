#include "AudioOutputWorker.h"
#include <QDebug>
#include <QThread>
#define DEBUG_AUDIO_OUTPUT
// Voice model: https://huggingface.co/rhasspy/piper-voices/tree/main/en/en_US
AudioOutputWorker::AudioOutputWorker(QObject *parent)
    : QObject(parent), m_stopped(false), m_audioOutput(nullptr), m_audioDevice(nullptr)
{
    m_mutex = new QMutex;
    m_pauseCond = new QWaitCondition;

    // Default to System TTS for zero-config fallback
    m_currentEngine = new PiperTTSEngine();

    // Standard low-latency PCM configuration format
    QAudioFormat format;
    format.setSampleRate(22050);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
    if (!info.isFormatSupported(format)) {
        qWarning() << "⚠️ 22050Hz raw PCM not supported natively. Attempting nearest matching fallback format...";
        format = info.nearestFormat(format);
    }

    m_audioOutput = new QAudioOutput(info, format, this);
    m_buffer.setBuffer(&m_audioData);
    if (!m_buffer.open(QIODevice::ReadWrite)) {
        qWarning() << "Failed to open shared QBuffer in ReadWrite mode.";
    }
    m_readPosition  = 0;
}

AudioOutputWorker::~AudioOutputWorker()
{
    stopWorker();
    m_currentEngine->deleteLater();
}

void AudioOutputWorker::stop() {
    m_interrupted.storeRelease(1);
    m_nextState = AUDIOOUTPUT_PROCESSING_EXIT;
    m_state = m_nextState;
    m_stopped = true;
    togglePause(false);
}

void AudioOutputWorker::togglePause(bool paused)
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

void AudioOutputWorker::requestInterruption() {
    m_interrupted.storeRelease(1);
}

void AudioOutputWorker::setVoiceModel(const QString& botName,
                    const QString& piperExePath,
                   const QString& modelPath)
{
    m_botName = botName;
    m_piperExePath = piperExePath;
    m_modelPath = modelPath;
}

void AudioOutputWorker::startWorker()
{
    if (m_stopped) return;

    m_mutex->lock();
    m_stopped = false;
    m_mutex->unlock();
}

void AudioOutputWorker::stopWorker()
{
    m_mutex->lock();
    m_stopped = false;
    m_pauseCond->wakeAll();
    if (m_currentEngine) m_currentEngine->stop();
    m_mutex->unlock();

    if (m_audioOutput) m_audioOutput->stop();
}

void AudioOutputWorker::clearQueue()
{
    m_mutex->lock();
    m_textQueue.clear();
    m_previousText.clear();
    m_sentenceBuffer.clear();
    if (m_currentEngine) m_currentEngine->stop(); // Interrupts active speech patterns instantly
    if (m_audioOutput) m_audioOutput->reset();
    m_mutex->unlock();
}

void AudioOutputWorker::handleToken(const QString &token)
{
    if (token.isEmpty()) return;
    m_sentenceBuffer.append(token);

    // Clause boundary checks for clean phrasing pipeline mechanics
    if (m_sentenceBuffer.contains('.') || m_sentenceBuffer.contains(',') ||
        m_sentenceBuffer.contains('?') || m_sentenceBuffer.contains('!') ||
        m_sentenceBuffer.contains('\n')) {
#ifdef DEBUG_AUDIO_OUTPUT
        qDebug("AudioOutputWorker::handleToken voice[%s]",
               m_sentenceBuffer.toStdString().c_str());
#endif
        m_mutex->lock();
        m_textQueue.enqueue(m_sentenceBuffer);
        m_sentenceBuffer = "";
        m_nextState = AUDIOOUTPUT_PROCESSING;
        m_state = m_nextState;
        m_mutex->unlock();
        togglePause(false);
    }
}

void AudioOutputWorker::doWork() {
    qDebug("AudioOutputWorker Dowork");
    m_stopped = false; // Reset flags
    m_sentenceBuffer = "I'm "+m_botName+". Let us play a match.";
    m_textQueue.enqueue(m_sentenceBuffer);
    m_sentenceBuffer = "";
    m_nextState = AUDIOOUTPUT_PROCESSING;
    m_state = m_nextState;
    while(!m_stopped){
        // Check for Stop
        m_mutex->lock();
        if(m_pause)
            m_pauseCond->wait(m_mutex); // in this place, your thread will stop to execute until someone calls resume
        m_mutex->unlock();
        if(m_nextState != m_state && m_state != AUDIOOUTPUT_WAITING) {
            m_state = m_nextState;
        }
        switch (m_state) {
        case AUDIOOUTPUT_INIT: {
            m_state = AUDIOOUTPUT_WAITING;
        }
            break;
        case AUDIOOUTPUT_WAITING: {
            QThread::msleep(30);
        }
            break;
        case AUDIOOUTPUT_PROCESSING :{
            int audioResult = processAudioLoop();
            if(audioResult == AUDIOOUTPUT_DONE_SUCCESS) {
                m_state = AUDIOOUTPUT_WAITING;
            } else if(audioResult == AUDIOOUTPUT_DONE_FAILED) {
                m_state = AUDIOOUTPUT_WAITING;
            }
        }
            break;
        case AUDIOOUTPUT_PROCESSING_EXIT :{
            m_stopped = true;
        }
            break;
        }
    }
    qDebug("AudioOutputWorker Dowork finished");
}
int AudioOutputWorker::processAudioLoop()
{
    QString textToSpeak;
    int nextState = AUDIOOUTPUT_PENDING;
    if(m_textQueue.size() == 0) {
        if(!m_emitVoiceStop) {
            Q_EMIT voiceFinished();
            m_emitVoiceStop = true;
        } else {
            QThread::msleep(8);
        }
        return nextState;
    }
    m_emitVoiceStop = false;
    Q_EMIT voiceStarted();
    textToSpeak = m_textQueue.dequeue();
    m_textQueue.clear();
    qDebug("AudioOutputWorker::processAudioLoop m_textQueue size[%d]",
           m_textQueue.size());
#ifdef DEBUG_AUDIO_OUTPUT
    qDebug("AudioOutputWorker::processAudioLoop [%s]",
           textToSpeak.toStdString().c_str());
#endif
    // Branching Execution via Polymorphic Strategy Calls
    if (!m_currentEngine->isPCMGenerator()) {
        // Path A: System native text engine (Runs asynchronous non-blocking OS threads)
        m_currentEngine->speakDirect(textToSpeak);
    } else {
        // Path B: Piper Local Synthesis (Returns chunks into your legacy QAudioOutput line)
        QByteArray audioChunks = m_currentEngine->generatePCM(textToSpeak,
                                                              m_piperExePath,
                                                              m_modelPath);
        appendAndPlayPCM(audioChunks);
    }
#ifdef DEBUG_AUDIO_OUTPUT
    qDebug("AudioOutputWorker::processAudioLoop [%s] done",
           textToSpeak.toStdString().c_str());
#endif
    return nextState;
}

void AudioOutputWorker::appendAndPlayPCM(const QByteArray &newPcmData) {
    if (newPcmData.isEmpty()) return;
#ifdef DEBUG_AUDIO_OUTPUT
    qDebug("appendAndPlayPCM %d bytes", newPcmData.size());
#endif

    // 1. Stop any currently active hardware playback safely
    m_audioOutput->stop();
    m_buffer.close();

    // 2. Clear old data completely so we NEVER repeat past phrases
    m_audioData = newPcmData;

    // 3. Reinitialize the buffer with the fresh payload
    m_buffer.setBuffer(&m_audioData);
    if (!m_buffer.open(QIODevice::ReadOnly)) { // ReadOnly is safer for playback lines
        qWarning() << "Failed to open QBuffer in ReadOnly mode.";
        return;
    }
    m_buffer.seek(0);

    // 4. Fire up the audio hardware device pointing to the brand new data
    m_audioOutput->start(&m_buffer);

    // 5. Corrected Throttling: Block the worker thread until this discrete chunk finishes playing
    while (m_audioOutput->state() != QAudio::StoppedState && !m_stopped) {
        if (m_audioOutput->state() == QAudio::IdleState) {
            // IdleState in QAudioOutput means the entire buffer has been consumed
            break;
        }
        QThread::msleep(10);
    }
}

