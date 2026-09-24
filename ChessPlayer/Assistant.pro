TEMPLATE = app
QT += core gui qml quick texttospeech multimedia
CONFIG += c++11 console

TARGET = ChessVoiceAssistant
TEMPLATE = app

CONFIG += use_ai_assistant

use_ai_assistant {
DEFINES += USE_AI_ASSISTANT
    win32 {
        # 1. Update these paths to match where your repositories live on your disk
        LLAMA_SOURCE_DIR = "$$PWD/../../Chatbot/llama.cpp"
        LLAMA_BUILD_DIR  = "$$PWD/../../Chatbot/llama.cpp/build"

        # Update this directory address to match your folder structure
        WHISPER_DIR = "$$PWD/../../Chatbot/whisper.cpp"
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

        msvc {
            QMAKE_CXXFLAGS += /EHsc
            QMAKE_CFLAGS   += /arch:AVX2
            QMAKE_CXXFLAGS += /arch:AVX2
        }
    }
    unix:!macx {
        # 1. Update these paths to match where your repositories live on your disk
        LLAMA_SOURCE_DIR = "$$PWD/../../ai/llama.cpp"

        # Update this directory address to match your folder structure
        WHISPER_DIR = "$$PWD/../../ai/whisper.cpp"
        # 3. Link Compiled Libraries (Windows MSVC syntax)
        LIBS += -L/usr/local/lib/ \
            -lllama \
            -lllama-common \
            -lggml \
            -lggml-cpu \
            -lggml-base \
            -lwhisper \
            -lparakeet
    }
# Add WHISPER_VERSION to your existing DEFINES block
DEFINES += NOMINMAX \
           _CRT_SECURE_NO_WARNINGS \
           WHISPER_VERSION=\\\"1.6.0\\\"  # <--- ADD THIS LINE TO FIX THE COMPILER SCOPE ERROR!
# 2. Include Headers
# Add the missing ggml folder paths here
INCLUDEPATH += $$LLAMA_SOURCE_DIR/include \
               $$LLAMA_SOURCE_DIR/common \
               $$LLAMA_SOURCE_DIR/ggml/include

# Include Header paths for both public boundaries and internal ggml math configurations
INCLUDEPATH += $$WHISPER_DIR/include \
               $$WHISPER_DIR/ggml/include

# Add the core execution files so Qt compiles them natively from source code
SOURCES += \
    $$WHISPER_DIR/src/whisper.cpp

SOURCES += \
    assistant/AssistantController.cpp \
    assistant/AudioModelWorker.cpp \
    assistant/AudioOutputWorker.cpp \
    assistant/LLMWorker.cpp

HEADERS += \
    assistant/AssistantController.h \
    assistant/AudioModelWorker.h \
    assistant/AudioOutputWorker.h \
    assistant/LLMWorker.h \
    assistant/TTSEngines.h
}


SOURCES += \
    main_Assistant.cpp

RESOURCES += qml_Assistant.qrc
