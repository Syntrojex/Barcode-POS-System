#pragma once
// Shared Syntro-Check POS colors, application stylesheet, and painter-drawn icons.
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

namespace Theme
{
    inline const QColor surface("#f1f2fa");
    inline const QColor surfaceLow("#f7f8fd");
    inline const QColor surfaceContainer("#edf1f7");
    inline const QColor surfaceHigh("#e3e9f3");
    inline const QColor onSurface("#18243a");
    inline const QColor onSurfaceVariant("#526078");
    inline const QColor outline("#79869b");
    inline const QColor outlineVariant("#dce2eb");
    inline const QColor sidebar("#172443");
    inline const QColor primary("#2554d8");
    inline const QColor primaryContainer("#eaf0ff");
    inline const QColor tertiary("#006329");
    inline const QColor tertiaryFixed("#7ffc97");
    inline const QColor error("#ba1a1a");
    inline const QColor errorContainer("#ffdad6");
    inline const QColor white("#ffffff");
    inline const QColor darkSurface("#101626");
    inline const QColor darkPanel("#192337");
    inline const QColor darkText("#edf1fb");
    inline const QColor darkMuted("#afbdd5");
    inline const QColor darkDivider("#374560");

    bool isDark();
    void setDark(bool on); // persists with QSettings
    QColor readable(const QColor &c); // dark text colours are lightened in dark mode
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
