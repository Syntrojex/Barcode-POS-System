#include "MainWindow.h"
#include "Theme.h"
#include <QApplication>
#include <QFont>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("BarcodeHub");
    QApplication::setStyle(QStyleFactory::create("Fusion")); // identical look on every platform
    QFont f = app.font();
    f.setPointSizeF(9.5);
    app.setFont(f);
    app.setStyleSheet(Theme::styleSheet());

    MainWindow w;
    w.show();
    return app.exec();
}
