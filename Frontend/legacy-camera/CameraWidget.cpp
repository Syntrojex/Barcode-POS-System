#include "CameraWidget.h"
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
CameraWidget::CameraWidget(QWidget *parent)
    : QWidget(parent), view(new QLabel(this)), status(new QLabel("Camera is off", this)),
      toggle(new QPushButton("Start camera", this)), upload(new QPushButton("Upload image", this))
{
    auto *Layout = new QVBoxLayout(this);
    Layout->addWidget(view);
    view->setMinimumSize(640, 480);
    view->setScaledContents(true);
    // NEW: buttons + status label under the picture
    auto *buttons = new QHBoxLayout();
    buttons->addWidget(toggle);
    buttons->addWidget(upload);
    Layout->addLayout(buttons);
    Layout->addWidget(status);
    connect(&timer, &QTimer::timeout, this, &CameraWidget::onTick);
    connect(toggle, &QPushButton::clicked, this, &CameraWidget::onToggle);
    connect(upload, &QPushButton::clicked, this, &CameraWidget::onUpload);
}
void CameraWidget::start()
{
    if (scanner.openCamera())
    {
        timer.start(30);
        toggle->setText("Stop camera");
        setStatus("Point a barcode at the camera");
    }
    else
    {
        // NEW: error handling - tell the user, and offer the upload instead
        const QString message = QString::fromStdString(scanner.errMessage());
        setStatus(message + " - use Upload image");
        emit scanError(message);
    }
}
void CameraWidget::stop()
{
    timer.stop();
    scanner.closeCamera(); // NEW: really release the camera, otherwise its light stays on
    toggle->setText("Start camera");
    setStatus("Camera is off");
}
// NEW: Start / Stop button
void CameraWidget::onToggle()
{
    if (timer.isActive())
        stop();
    else
        start();
}
// NEW: upload fallback, choose a picture of a barcode
void CameraWidget::onUpload()
{
    const QString path = QFileDialog::getOpenFileName(this, "Choose a barcode picture", QString(),
                                                      "Pictures (*.png *.jpg *.jpeg *.bmp)");
    if (!path.isEmpty())
        scanImage(path);
}
void CameraWidget::scanImage(const QString &path)
{
    QFile file(path); // read the bytes ourselves, so file names with special letters also work on Windows
    if (!file.open(QIODevice::ReadOnly))
    {
        setStatus("Cannot open the picture");
        emit scanError("Cannot open the picture");
        return;
    }
    const QByteArray bytes = file.readAll();
    const cv::Mat picture = cv::imdecode(cv::Mat(1, bytes.size(), CV_8UC1, (void *)bytes.constData()), cv::IMREAD_COLOR);
    if (picture.empty())
    {
        setStatus("This file is not a picture");
        emit scanError("This file is not a picture");
        return;
    }
    std::string six;
    cv::Rect box;
    if (scanner.frameRead(six, box, picture))
    {
        setStatus("Scanned " + QString::fromStdString(six));
        emit barcodeScan(QString::fromStdString(six));
    }
    else
    {
        const QString message = scanner.badCheckDigit() ? "Wrong check digit in the picture" : "No barcode found in the picture";
        setStatus(message);
        emit scanError(message);
    }
}
void CameraWidget::setStatus(const QString &text)
{
    status->setText(text);
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
        stop();
        setStatus(QString::fromStdString(scanner.errMessage()) + " - use Upload image");
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
        // NEW: duplicate-scan prevention - the same barcode still in front of the camera is ignored for 2 seconds
        const QString code = QString::fromStdString(six);
        if (code == lastCode && lastClock.isValid() && lastClock.elapsed() < 2000)
            return;
        lastCode = code;
        lastClock.restart();
        setStatus("Scanned " + code);
        emit barcodeScan(code);
    }
    else if (scanner.badCheckDigit())
        setStatus("Wrong check digit - hold the barcode steady");
}