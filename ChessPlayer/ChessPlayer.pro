TEMPLATE = app
CONFIG += c++11 no_keywords console

QT += qml quick qml serialport texttospeech
CONFIG += c++11

CONFIG += use_chess_algo

CONFIG += use_image_processing
use_image_processing {
DEFINES += DEBUG_SHOW_IMAGE
DEFINES += IMAGE_PROCESS_MOVE
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
    -lopencv_videoio

OPENCV_WINDOWS = $$PWD/../../ImageProcessing/opencv-4.7.0/build/install
win32: INCLUDEPATH += "$$OPENCV_WINDOWS/include"
win32: DEPENDPATH += "$$OPENCV_WINDOWS/include"
win32: LIBS += "$$OPENCV_WINDOWS/x64/mingw/lib/libopencv_world470.dll.a"
win32: LIBS += -lpthread

INCLUDEPATH += \
    chessDetector
SOURCES += chessDetector/ChessImageProcessing.cpp
HEADERS += chessDetector/ChessImageProcessing.h
}
use_chess_algo{
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
SOURCES += \
    ChessBot.cpp \
    main.cpp

RESOURCES += \
    gui/qml.qrc
QML_DESIGNER_IMPORT_PATH =

HEADERS += \
    ChessBot.h \


