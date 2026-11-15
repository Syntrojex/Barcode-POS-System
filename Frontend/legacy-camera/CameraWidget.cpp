#include "CameraWidget.h"
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <opencv2/imgproc.hpp>
CameraWidget::CameraWidget(QWidget *parent) : QWidget(parent), view(new QLabel(this))
{
    auto *Layout = new QVBoxLayout(this);
    Layout->addWidget(view);
    view->setMinimumSize(640, 480);
    view->setScaledContents(true);
    connect(&timer, &QTimer::timeout, this, &CameraWidget::onTick);
}
void CameraWidget::start()
{
    if (scanner.openCamera())
        timer.start(30);
    else
        emit scanError(QString::fromStdString(scanner.errMessage()));
}
void CameraWidget::stop()
{
    timer.stop();
}
QImage CameraWidget::toQImage(const cv::Mat &BGR)
{
    cv::Mat RGB;
    cv::cvtColor(BGR, RGB, cv::COLOR_BGR2RGB);
    QImage img(RGB.data, RGB.cols, RGB.rows, (int)RGB.step, QImage::Format_RGB888);
    return img.copy();
}
void CameraWidget::onTick()
{
    cv::Mat frame;

    if (!scanner.readFrame(frame))
    {
        timer.stop();
        emit scanError(QString::fromStdString(scanner.errMessage()));
        return;
    }
    std::string six;
    cv::Rect Box;
    bool found = scanner.frameRead(six, Box, frame);
    if (Box.area() > 0)
        cv::rectangle(frame, Box, cv::Scalar(0, 255, 0), 2);
    view->setPixmap(QPixmap::fromImage(toQImage(frame)));
    if (found)
    {
        emit barcodeScan(QString::fromStdString(six));
    }
}