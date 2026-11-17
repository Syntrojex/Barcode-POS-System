#pragma once
// Design tokens from the SmartPOS Desktop design system (Stitch DESIGN.md),
// plus the application style sheet and a small set of painter-drawn icons.
#include <QColor>
#include <QObject>
#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QString>

// Emits changed() after the light/dark mode flips so widgets that cache colours (table cells,
// painted panels) can refresh. Widgets that only use the style sheet need nothing.
class ThemeNotifier : public QObject
{
    Q_OBJECT
signals:
    void changed(bool dark);
};

namespace Theme
{
    // Fixed accents used for icons that are painted once. This blue stays readable on both the
    // light and the dark surfaces, so icons never need recolouring when the mode flips.
    inline const QColor primary("#3b6ae8");
    inline const QColor outline("#7b869c");
    inline const QColor onSurface("#0f1b33");
    inline const QColor onSurfaceVariant("#5b6781");
    inline const QColor error("#ba1a1a");
    inline const QColor white("#ffffff");

    // Every colour the UI needs, by role. One instance per mode (see light()/dark()).
    struct Palette
    {
        // surfaces
        QColor bg, surface, surfaceAlt, hover, border, borderStrong;
        // text
        QColor text, muted, subtle, accentText, dangerText, successText, warnText;
        // brand
        QColor sidebar, primary, primaryHover, primaryPress, primaryTop, primaryHoverTop;
        // soft (tinted) controls
        QColor softBg, softBorder, softEdge, softHover, softPress, softText;
        // painted widgets
        QColor cardBg, cardBorder, alertBg, alertBorder, shadow;
        QColor chartBar, chartBarHot, chartGrid, chartTarget;
        QColor thumbBg;
    };
    const Palette &light();
    const Palette &dark();
    // The palette of the active mode.
    const Palette &palette();

    // ---- mode ------------------------------------------------------------------------
    bool isDark();
    // Switches mode, re-applies palette + style sheet to the running application, saves the
    // choice and notifies. No-op when already in that mode.
    void setDark(bool on);
    // Loads the saved choice (default: light) without notifying. Call once before the first
    // window is created.
    void restore();
    // Applies the active mode's palette and style sheet to the application (startup + setDark).
    void apply(QApplication &app);
    ThemeNotifier &notifier();

    // Colours for hand-coloured text, resolved against the active mode.
    inline QColor accentText() { return palette().accentText; }
    inline QColor dangerText() { return palette().dangerText; }
    inline QColor successText() { return palette().successText; }
    inline QColor warnText() { return palette().warnText; }
    inline QColor subtleText() { return palette().subtle; }

    // Application-wide Qt style sheet for the active mode.
    QString styleSheet();

    // Vector icon drawn with QPainter (no image files needed).
    // Names: dashboard cart orders products barcode bills settings search trash plus minus
    // play stop check warn lock wifi clock user export refresh filter expand camera info tag
    // shield edit eye print logout card cash arrow-right arrow-left chevron-down chevron-up tick
    // pause moon sun
    QPixmap iconPixmap(const QString &name, const QColor &color, int px = 18);
    QIcon icon(const QString &name, const QColor &color = onSurface, int px = 18);

    // Re-apply the style sheet after changing a dynamic property.
    void repolish(QWidget *w);
}
