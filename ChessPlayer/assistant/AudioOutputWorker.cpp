#include "AudioOutputWorker.h"
#include <QDebug>
#include <QThread>

// Sure! Pokémon are characters from the world of video games and anime. They're a type of fictional creatures with different abilities, powers or types that can be traded between players in various online adventures known as POKeMON games and the anime. They are designed to appeal both children and adults, focusing on fun gameplay mechanics while also incorporating elements of art design that have become iconic over time. I hope this helps! Let me know if you need more information.
AudioOutputWorker::AudioOutputWorker(QObject *parent)
    : QObject(parent), m_stopped(false), m_audioOutput(nullptr), m_audioDevice(nullptr)
{
    m_mutex = new QMutex;
    m_pauseCond = new QWaitCondition;

    // Default to System TTS for zero-config fallback
    m_currentEngine = new PiperTTSEngine();

    // Standard low-latency PCM configuration format
    QAudioFormat format;
    format.setSampleRate(24000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
    if (!info.isFormatSupported(format)) {
        qWarning() << "⚠️ 24000Hz raw PCM not supported natively. Attempting nearest matching fallback format...";
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
    delete m_currentEngine;
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
        qDebug("AudioOutputWorker::handleToken m_sentenceBuffer %s",
               m_sentenceBuffer.toStdString().c_str());
        m_mutex->lock();
        m_textQueue.enqueue(m_sentenceBuffer);
        m_sentenceBuffer = "";
        m_nextState = AUDIOOUTPUT_PROCESSING;
        m_state = m_nextState;
        m_mutex->unlock();
        togglePause(false);
    }
//    m_mutex->lock();
//    m_textQueue.enqueue("1 ");
//    m_textQueue.enqueue("2 ");
//    m_textQueue.enqueue("3 ");
//    m_nextState = AUDIOOUTPUT_PROCESSING;
//    m_state = m_nextState;
//    m_mutex->unlock();
//    togglePause(false);
}

void AudioOutputWorker::doWork() {
    qDebug("AudioOutputWorker Dowork");
    m_stopped = false; // Reset flags
    m_state = AUDIOOUTPUT_INIT;
    m_nextState = AUDIOOUTPUT_INIT;
    while(!m_stopped){
//        qDebug("AudioOutputWorker doWork m_state[%d] m_nextState[%d]",
//               m_state,m_nextState);
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
        QThread::msleep(8);
        return nextState;
    }
    textToSpeak = m_textQueue.dequeue();
    qDebug("AudioOutputWorker::processAudioLoop [%s]",
           textToSpeak.toStdString().c_str());

    // Branching Execution via Polymorphic Strategy Calls
    if (!m_currentEngine->isPCMGenerator()) {
        // Path A: System native text engine (Runs asynchronous non-blocking OS threads)
        m_currentEngine->speakDirect(textToSpeak);
    } else {
        // Path B: Piper Local Synthesis (Returns chunks into your legacy QAudioOutput line)
        QList<QByteArray> audioChunks = m_currentEngine->generatePCM(textToSpeak);
        for(QByteArray pcmChunk: audioChunks) {
            appendAndPlayPCM(pcmChunk);
        }
    }
    qDebug("AudioOutputWorker::processAudioLoop [%s] done",
           textToSpeak.toStdString().c_str());
    return nextState;
}

void AudioOutputWorker::appendAndPlayPCM(const QByteArray &newPcmData) {
    if (newPcmData.isEmpty()) return;
    qDebug("appendAndPlayPCM %d bytes",newPcmData.size());
    // 2. Track where the speaker was previously reading
    if (m_audioOutput->state() == QAudio::ActiveState) {
        m_readPosition = m_buffer.pos();
    }

    // 3. Move cursor to the absolute end to append the fresh data frame payload
    m_buffer.seek(m_audioData.size());
    m_buffer.write(newPcmData);

    // 4. Force the Qt 5 Audio State Machine reset sequence
    m_audioOutput->stop();
    m_buffer.seek(m_readPosition);
    m_audioOutput->start(&m_buffer);
    while (m_audioOutput->state() == QAudio::ActiveState && !m_stopped) {
        if (m_audioOutput->bytesFree() < 2048) {
            QThread::msleep(10); // Throttle loop processing dynamically
        } else {
            break; // Break throttling loop if the sound hardware requires more data chunks
        }
    }
}
