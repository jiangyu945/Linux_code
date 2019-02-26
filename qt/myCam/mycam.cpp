#include "mycam.h"
#include "ui_mycam.h"

myCam::myCam(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::myCam)
{
    ui->setupUi(this);
    timer = new QTimer(this);
    index = 0;   //初始化图片名下标

    connect(ui->Bt_CamOpen,SIGNAL(clicked(bool)),this,SLOT(doProcessOpenCam()));  //打开摄像头
    connect(ui->Bt_SaveImg,SIGNAL(clicked(bool)),this,SLOT(doProcessSaveImg()));  //保存图片
    connect(ui->Bt_CamClose,SIGNAL(clicked(bool)),this,SLOT(doProcessCloseCam())); //关闭摄像头
    connect(timer,SIGNAL(timeout()),this,SLOT(doProcessCapture()));    //利用定时器超时控制采集频率
}

myCam::~myCam()
{
    delete ui;Mini
}

//打开摄像头
void myCam::doProcessOpenCam(){

    //VideoCapture类获取摄像头视频---读到数据为mat格式
    cap.open(0);//0表示默认摄像头

    if(!cap.isOpened())//先判断是否打开摄像头
    {
        QMessageBox msg;
        msg.about(NULL,"错误提示","摄像头打开失败!");
        return;
    }

    cap.set(CV_CAP_PROP_FRAME_WIDTH,640);  //设置帧宽
    cap.set(CV_CAP_PROP_FRAME_HEIGHT,480); //设置帧高
    cap.set(CV_CAP_PROP_FPS,30);           //设置帧率

    double rate = cap.get(CV_CAP_PROP_FPS); //获取当前视频帧率
    timer->start(1000.000/rate);    //开启定时器,超时发出信号
}

//采集图片
void myCam::doProcessCapture()
{
    bool ret = cap.read(frame);   //获取一帧图像
    if(!ret){
        QMessageBox msg;
        msg.about(NULL,"提示","Cannot read a frame from video stream");
    }

    //cvtColor(pFrame, pFrame, CV_BGR2RGB);
    QImage img = QImage((const unsigned char*)frame.data, // uchar* data
                frame.cols, frame.rows, QImage::Format_RGB888);//格式转换
    //图片显示
    img.scaled(ui->label_show->width(),ui->label_show->height());  //图片自适应窗口大小
    ui->label_show->setPixmap(QPixmap::fromImage(img));  // 将图片显示到label上


}

//保存图片
void myCam::doProcessSaveImg()
{
    index++;
    char outfile[50];
    sprintf(outfile, "./cap_test%d.jpg", index); //利用sprintf格式化图片名
    bool ret = imwrite(outfile,frame);
    if(!ret){
        QMessageBox msg;
        msg.about(NULL,"提示","图片保存成功!");
    }
}

//关闭摄像头
void myCam::doProcessCloseCam()
{
    timer->stop();   // 停止读取数据。
    cap.release();   //释放内存；

    QMessageBox msg;
    msg.about(NULL,"信息","摄像头已关闭!");
}
