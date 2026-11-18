#pragma once

#include <QString>
#include <QKeySequence>
#include <QVector>

/**
 * Represents a single navigation entry in the sidebar.
 * Used by MainWindow to build the UI dynamically.
 */
struct NavItem {
    QString id;          // internal identifier (e.g. "dashboard")
    QString text;        // visible label
    QString icon;        // icon name used by Theme::iconPixmap
    QKeySequence shortcut; // optional hot‑key (F1, F2, …)
};

/**
 * NavigationModel supplies the list of pages for the POS UI.
 * It can later be backed by a JSON file or a plugin system – for now it
 * returns a static vector, making it OCP‑friendly.
 */
class NavigationModel {
public:
    static QVector<NavItem> items();
};
