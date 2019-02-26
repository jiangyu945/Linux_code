#-------------------------------------------------
#
# Project created by QtCreator 2019-02-15T17:35:24
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = myCam
TEMPLATE = app

#INCLUDEPATH +=  /usr/local/include \
#                /usr/local/include/opencv \
#                /usr/local/include/opencv2

#LIBS += /usr/local/lib/libopencv_highgui.so \
#        /usr/local/lib/libopencv_core.so    \
#        /usr/local/lib/libopencv_imgproc.so \
#        /usr/local/lib/libopencv_imgcodecs.so \
#        /usr/local/lib/libopencv_videoio.so

INCLUDEPATH +=  /home/jiangyu/opencv/opencv-3.2.0/output/include \
                /home/jiangyu/opencv/opencv-3.2.0/output/include/opencv \
                /home/jiangyu/opencv/opencv-3.2.0/output/include/opencv2

LIBS += /home/jiangyu/opencv/opencv-3.2.0/output/lib/libopencv_highgui.so \
        /home/jiangyu/opencv/opencv-3.2.0/output/lib/libopencv_core.so    \
        /home/jiangyu/opencv/opencv-3.2.0/output/lib/libopencv_imgproc.so \
        /home/jiangyu/opencv/opencv-3.2.0/output/lib/libopencv_imgcodecs.so \
        /home/jiangyu/opencv/opencv-3.2.0/output/lib/libopencv_videoio.so


SOURCES += main.cpp\
        mycam.cpp

HEADERS  += mycam.h

FORMS    += mycam.ui
