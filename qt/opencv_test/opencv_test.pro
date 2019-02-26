#-------------------------------------------------
#
# Project created by QtCreator 2019-02-13T12:46:15
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = opencv_test


INCLUDEPATH +=  /home/jiangyu/opencv/opencv-3.2.0/output/include \
                /home/jiangyu/opencv/opencv-3.2.0/output/include/opencv \
                /home/jiangyu/opencv/opencv-3.2.0/output/include/opencv2

LIBS += /home/jiangyu/opencv/opencv-3.2.0/output/lib/libopencv_highgui.so \
        /home/jiangyu/opencv/opencv-3.2.0/output/lib/libopencv_core.so    \
        /home/jiangyu/opencv/opencv-3.2.0/output/lib/libopencv_imgproc.so \
        /home/jiangyu/opencv/opencv-3.2.0/output/lib/libopencv_imgcodecs.so



TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp

HEADERS  += widget.h

FORMS    += widget.ui
