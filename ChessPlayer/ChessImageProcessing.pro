TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

CONFIG += use_image_processing
use_image_processing {
DEFINES += DEBUG_SHOW_IMAGE
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
SOURCES += chessDetector/ChessImageProcessing.cpp \
    main_simple_test.cpp
HEADERS += chessDetector/ChessImageProcessing.h
}


