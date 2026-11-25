#include "MainWindow.h"
#include "Theme.h"
#include <QApplication>
#include <QFont>
#include "Dialogs.h"
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

    // Sign in, run the POS, and return to sign-in after "Log out".
    while (true)
    {
        UserRole role = UserRole::Cashier;
        QString name;
        if (!Dialogs::login(nullptr, role, name))
            return 0; // sign-in window closed
        MainWindow w(role, name);
        w.show();
        app.exec();
        if (!w.loggedOut())
            return 0; // "Exit" closes the app
    }
}
