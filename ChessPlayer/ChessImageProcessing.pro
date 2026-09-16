TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

CONFIG += use_image_processing
use_image_processing {
DEFINES += DEBUG_SHOW_IMAGE
#DEFINES += DEBUG_SINGLE_IMAGE
DEFINES += DEBUG_CLASSIFICATION
DEFINES += DEBUG_ROI
#DEFINES += GEN_CHESSBOARD_DATA
DEFINES += GEN_DROPZONE_DATA
use_openmp {
DEFINES += USE_OPENMP
# Enable the OpenMP multi-threading compiler flags
QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS   += -fopenmp

# Link the OpenMP runtime system library
LIBS += -lgomp
}
use_openvino {
DEFINES += USE_OPENVINO
unix:!macx: INCLUDEPATH += /usr/local/runtime/include
unix:!macx: DEPENDPATH += /usr/local/runtime/include
unix:!macx: LIBS += -L/usr/local/runtime/3rdparty/tbb/lib/ -ltbb
unix:!macx: LIBS += -L/usr/local/runtime/lib/intel64/ \
    -lopenvino \
    -lopenvino_tensorflow_lite_frontend \
    -lopenvino_tensorflow_frontend \
    -lopenvino_pytorch_frontend \
    -lopenvino_paddle_frontend \
    -lopenvino_onnx_frontend \
    -lopenvino_hetero_plugin \
    -lopenvino_intel_cpu_plugin \
    -lopenvino_intel_gpu_plugin \
    -lopenvino_intel_npu_plugin \
    -lopenvino_gguf_frontend \
    -lopenvino_auto_plugin \
    -lopenvino_auto_batch_plugin
}
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
SOURCES += chessDetector/ChessImageProcessing.cpp \
    main_simple_test.cpp \
    main_color.cpp \
    main_check_cell.cpp \
    main_mosse.cpp \
    main_dnn.cpp \
    main_dnn_origin.cpp \
    main_gray_filter.cpp \
    main_v4l2.cpp \
    main_calib.cpp \
    main_3d_projection.cpp
HEADERS += chessDetector/ChessImageProcessing.h
}

SOURCES += \
    main_3d_construction.cpp \
    main_benchmark_openvino.cpp \
    main_gen_dataset.cpp \
    main_hough_circle.cpp



