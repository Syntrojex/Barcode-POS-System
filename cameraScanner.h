#pragma once
#include "decoder.h"
#include <opencv2/videoio.hpp>
#include <string>
class cameraScanner
{
    cv::VideoCapture camera;
    Decoder decoder;
    int index;
    int width;
    int height;
    std::string errorMsg;
    bool find(const cv::Mat &gray, cv::Rect &box) const;

public:
    cameraScanner(int idx = 0, int width = 640, int height = 480);
    bool openCamera();
    bool readFrame(cv::Mat &frame);
    bool scan(std::string &six);
    bool frameRead(std::string &six, cv::Rect &box, const cv::Mat &frame);
    std::string errMessage() const;
    ~cameraScanner();
    void closeCamera();         // release camera when scanning stops
    bool badCheckDigit() const; // did the last frame have a wrong check digit?
};
