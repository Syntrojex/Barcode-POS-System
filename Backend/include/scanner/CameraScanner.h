#pragma once
#include "Decoder.h"
#include <opencv2/videoio.hpp>
#include <string>
class CameraScanner
{
    cv::VideoCapture camera;
    Decoder decoder;
    int index;
    int width;
    int height;
    std::string errorMsg;
    bool find(const cv::Mat &gray, cv::Rect &box) const;

public:
    CameraScanner(int idx = 0, int width = 640, int height = 480);
    bool openCamera();
    bool readFrame(cv::Mat &frame);
    bool scan(std::string &six);
    bool frameRead(std::string &six, cv::Rect &box, const cv::Mat &frame);
    std::string errMessage() const;
    ~CameraScanner();
};