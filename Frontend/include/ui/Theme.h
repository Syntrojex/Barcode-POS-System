#pragma once
// Design tokens from the SmartPOS Desktop design system (Stitch DESIGN.md),
// plus the application style sheet and a small set of painter-drawn icons.
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

namespace Theme
{
    inline const QColor surface("#f4f7fe");
    inline const QColor surfaceLow("#eff4ff");
    inline const QColor surfaceContainer("#e5eeff");
    inline const QColor surfaceHigh("#dce9ff");
    inline const QColor onSurface("#0f1b33");
    inline const QColor onSurfaceVariant("#5b6781");
    inline const QColor outline("#8a94ab");
    inline const QColor outlineVariant("#c3c6d7");
    inline const QColor primary("#2554d8");
    inline const QColor primaryContainer("#3f7bf6");
    inline const QColor tertiary("#006329");
    inline const QColor tertiaryFixed("#7ffc97");
    inline const QColor error("#ba1a1a");
    inline const QColor errorContainer("#ffdad6");
    inline const QColor white("#ffffff");

    bool isDark();
    void setDark(bool on); // persists with QSettings
    // Application-wide Qt style sheet (light or dark).
    QString styleSheet();

    // Vector icon drawn with QPainter (no image files needed).
    // Names: dashboard cart orders products barcode bills settings search trash plus minus
    // play stop check warn lock wifi clock user export refresh filter expand camera info tag
    // shield edit eye print logout card cash arrow-right arrow-left chevron-down pause
    QPixmap iconPixmap(const QString &name, const QColor &color, int px = 18);
    QIcon icon(const QString &name, const QColor &color = onSurface, int px = 18);

    // Re-apply the style sheet after changing a dynamic property.
    void repolish(QWidget *w);
}
