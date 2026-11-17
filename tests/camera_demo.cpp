#include <QApplication>
#include "CameraWidget.h"
int main(int argc, char *argv[])
{
    QApplication app(argc, argv); // starts Qt
    CameraWidget cam;             // your widget, as its own window
    QObject::connect(&cam, &CameraWidget::barcodeScan, [&cam](const QString &six)
                     {
                         cam.setWindowTitle("Scanned: " + six); // show the digits in the title bar
                     });
    QObject::connect(&cam, &CameraWidget::scanError, [&cam](const QString &msg)
                     {
                         cam.setWindowTitle("Error: " + msg); // show the problem instead
                     });
    cam.show();        // open the window
    cam.start();       // open the camera (connect first, start last)
    return app.exec(); // keep the window alive
}