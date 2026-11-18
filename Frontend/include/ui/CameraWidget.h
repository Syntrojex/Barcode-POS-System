#pragma once
#include <QWidget>
#include <QTimer>
#include <QString>
#include <QImage>
#include <opencv2/core.hpp>
#include "cameraScanner.h"
class QLabel;
class CameraWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CameraWidget(QWidget *parent = nullptr);
    void start();
    void stop();
signals:
    void barcodeScan(const QString &six);
    void scanError(const QString &message);
private slots:
    void onTick();

private:
    // OpenCV picture to Qt picture
    static QImage toQImage(const cv::Mat &BGR);
    // backend work of decoding and scanning
    cameraScanner scanner;
    QLabel *view;
    QTimer timer;
};