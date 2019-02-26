#ifndef MYCAM_H
#define MYCAM_H

#include "opencv.hpp"
#include <iostream>

#include <QWidget>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>

using namespace cv;

namespace Ui {
class myCam;
}

class myCam : public QWidget
{
    Q_OBJECT

public:
    explicit myCam(QWidget *parent = 0);
    ~myCam();

public slots:
    void doProcessOpenCam();  // 打开摄像头
    void doProcessSaveImg();  // 保存图片
    void doProcessCloseCam(); // 关闭摄像头
    void doProcessCapture();  // 采集图片

private:
    Ui::myCam *ui;

    VideoCapture cap; //声明视频读入类
    Mat frame;        //帧缓存结构

    QTimer *timer;  //计时用
    int index;      // 图片下标
};

#endif // MYCAM_H
