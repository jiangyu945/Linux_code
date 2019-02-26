#include "widget.h"
#include <QApplication>
#include <opencv2/opencv.hpp>


int main()
{
    cv::Mat image = cv::imread("/test.jpg", cv::IMREAD_COLOR);
    cv::imshow("opencv_test",image);
    cv::waitKey(0);
    return 0;
}
