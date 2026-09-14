TEMPLATE = app
CONFIG += c++11 console

QT += core gui qml quick serialport multimedia

CONFIG += use_chess_algo
#CONFIG += use_ai_assistant
CONFIG += use_image_processing
CONFIG += use_system_voice
#CONFIG += use_sanitize
use_system_voice {
    QT += texttospeech
}
use_sanitize {
QMAKE_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
QMAKE_LFLAGS += -fsanitize=address
}
#DEFINES += TEST_RANDOM_MOVE
use_image_processing {
#DEFINES += DEBUG_ROI
#DEFINES += DEBUG_SHOW_IMAGE
#DEFINES += DEBUG_WRITE_IMAGE
#DEFINES += DEBUG_SIMPLE_MOVE
#DEFINES += IMAGE_PROCESS_MOVE
unix:!macx: INCLUDEPATH += /usr/local/include/opencv4
unix:!macx: DEPENDPATH += /usr/local/include/opencv4
unix:!macx: LIBS += -L/usr/local/lib/  \
    -lopencv_objdetect \
    -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_features2d -lopencv_calib3d \
    -lopencv_videostab \
    -lopencv_video \
    -lopencv_core \
    -lopencv_highgui \
    -lopencv_imgcodecs \
    -lopencv_imgproc \
    -lopencv_videoio \
    -lopencv_dnn \
    -lopencv_dnn_objdetect \
    -lopencv_dnn_superres

OPENCV_WINDOWS = $$PWD/../../ImageProcessing/compiledopencv
win32: INCLUDEPATH += "$$OPENCV_WINDOWS/include"
win32: DEPENDPATH += "$$OPENCV_WINDOWS/include"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_calib3d4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_core4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_dnn4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_features2d4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_flann4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_gapi4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_highgui4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_imgcodecs4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_imgproc4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_ml4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_objdetect4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_photo4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_stitching4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_video4130.dll.a"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_videoio4130.dll.a"
win32: LIBS += -lpthread

INCLUDEPATH += \
    chessDetector
SOURCES += chessDetector/ChessImageProcessing.cpp
HEADERS += chessDetector/ChessImageProcessing.h
}

use_chess_algo {
INCLUDEPATH += \
    chessAlgo
SOURCES += \
    chessAlgo/ChessController.cpp \
    chessAlgo/BitBoardUtils.cpp \
    chessAlgo/Board.cpp \
    chessAlgo/Eval.cpp \
    chessAlgo/EvalTables.cpp \
    chessAlgo/MagicMoves.cpp \
    chessAlgo/Move.cpp \
    chessAlgo/MoveGen.cpp \
    chessAlgo/MoveOrdering.cpp \
    chessAlgo/Pawn.cpp \
    chessAlgo/Search.cpp \
    chessAlgo/Tables.cpp \
    chessAlgo/TT.cpp \
    chessAlgo/Utils.cpp

HEADERS += \
    chessAlgo/ChessController.h \
    chessAlgo/BitBoardUtils.hpp \
    chessAlgo/Board.hpp \
    chessAlgo/Eval.hpp \
    chessAlgo/EvalTables.hpp \
    chessAlgo/MagicMoves.hpp \
    chessAlgo/Move.hpp \
    chessAlgo/MoveGen.hpp \
    chessAlgo/MoveOrdering.hpp \
    chessAlgo/Pawn.hpp \
    chessAlgo/Piece.hpp \
    chessAlgo/Search.hpp \
    chessAlgo/Tables.hpp \
    chessAlgo/TT.hpp \
    chessAlgo/TTEntry.hpp \
    chessAlgo/Types.hpp \
    chessAlgo/Utils.hpp
}

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
    ChessBot.cpp \
    MasterChessBot.cpp \
    main.cpp

RESOURCES += \
    gui/qml.qrc

HEADERS += \
    ChessBot.h \
    MasterChessBot.h


