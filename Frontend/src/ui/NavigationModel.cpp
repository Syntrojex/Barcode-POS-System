#include "NavigationModel.h"

QVector<NavItem> NavigationModel::items()
{
    return {
        {"dashboard",   QStringLiteral("Dashboard"),   QStringLiteral("dashboard"),   QKeySequence(Qt::Key_F1)},
        {"recordorder", QStringLiteral("Record Order"), QStringLiteral("cart"),       QKeySequence(Qt::Key_F2)},
        {"orders",      QStringLiteral("Orders"),      QStringLiteral("orders"),    QKeySequence(Qt::Key_F3)},
        {"products",    QStringLiteral("Products"),    QStringLiteral("products"),  QKeySequence(Qt::Key_F4)},
        {"scanner",     QStringLiteral("Barcode Scanner"), QStringLiteral("barcode"), QKeySequence(Qt::Key_F5)},
        {"bills",       QStringLiteral("Bills"),       QStringLiteral("bills"),    QKeySequence(Qt::Key_F6)},
        {"settings",    QStringLiteral("Settings"),    QStringLiteral("settings"), QKeySequence(Qt::Key_F10)}
    };
}
