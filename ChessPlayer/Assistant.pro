TEMPLATE = app
QT += core gui qml quick axcontainer multimedia
CONFIG += c++11 console

TARGET = ChessVoiceAssistant
TEMPLATE = app

# 1. Update these paths to match where your repositories live on your disk
LLAMA_SOURCE_DIR = "$$PWD/../../Chatbot/llama.cpp"
LLAMA_BUILD_DIR  = "$$PWD/../../Chatbot/llama.cpp/build"

# 2. Include Headers
# Add the missing ggml folder paths here
INCLUDEPATH += $$LLAMA_SOURCE_DIR/include \
               $$LLAMA_SOURCE_DIR/common \
               $$LLAMA_SOURCE_DIR/ggml/include
win32 {
    # 3. Link Compiled Libraries (Windows MSVC syntax)
    LIBS += "$$LLAMA_BUILD_DIR/common/Release/llama-common.lib"
    LIBS += "$$LLAMA_BUILD_DIR/common/Release/llama-common-base.lib"
    LIBS += "$$LLAMA_BUILD_DIR/src/Release/llama.lib"
    LIBS += "$$LLAMA_BUILD_DIR/ggml/src/Release/ggml.lib"
    LIBS += "$$LLAMA_BUILD_DIR/ggml/src/Release/ggml-base.lib"
    LIBS += "$$LLAMA_BUILD_DIR/ggml/src/Release/ggml-cpu.lib"

    DEFINES += NOMINMAX

    # Only apply /EHsc if using Microsoft Visual Studio compiler
    msvc {
        QMAKE_CXXFLAGS += /EHsc
    }

    # If using MinGW, use the standard GCC exception flag instead
    gcc {
        QMAKE_CXXFLAGS += -fexceptions
    }
}

# Update this directory address to match your folder structure
WHISPER_DIR = "$$PWD/../../Chatbot/whisper.cpp"

# Include Header paths for both public boundaries and internal ggml math configurations
INCLUDEPATH += $$WHISPER_DIR/include \
               $$WHISPER_DIR/ggml/include

# Add the core execution files so Qt compiles them natively from source code
SOURCES += \
    $$WHISPER_DIR/src/whisper.cpp

win32 {
    # Add WHISPER_VERSION to your existing DEFINES block
    DEFINES += NOMINMAX \
               _CRT_SECURE_NO_WARNINGS \
               WHISPER_VERSION=\\\"1.6.0\\\"  # <--- ADD THIS LINE TO FIX THE COMPILER SCOPE ERROR!

    msvc {
        QMAKE_CXXFLAGS += /EHsc
        QMAKE_CFLAGS   += /arch:AVX2
        QMAKE_CXXFLAGS += /arch:AVX2
    }
}

# 5. Application Files
HEADERS += \
    assistant/AssistantController.h \
    assistant/AudioModelWorker.h \
    assistant/LLMWorker.h
#    voice/VoiceStreamer.h \
#    assistant/PipelineController.h \
#    assistant/StreamingAudioWorker.h


SOURCES += \
    main_Assistant.cpp \
    assistant/AssistantController.cpp \
#    voice/VoiceStreamer.cpp \
#    assistant/PipelineController.cpp \
#    assistant/StreamingAudioWorker.cpp \
    assistant/AudioModelWorker.cpp \
    assistant/LLMWorker.cpp

RESOURCES += qml_Assistant.qrc
