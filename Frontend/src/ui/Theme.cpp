#include "Theme.h"
#include <QPainter>
#include <QApplication>
#include <QDir>
#include <algorithm>
#include <QFile>
#include <QHash>
#include <QPainterPath>
#include <QPalette>
#include <QSettings>
#include <QTemporaryDir>
#include <QStyle>
#include <QWidget>

void Theme::repolish(QWidget *w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

QIcon Theme::icon(const QString &name, const QColor &color, int px)
{
    return QIcon(iconPixmap(name, color, px));
}

QPixmap Theme::iconPixmap(const QString &n, const QColor &c, int px)
{
    const qreal dpr = 2.0;
    QPixmap pm(int(px * dpr), int(px * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(px / 24.0, px / 24.0);
    p.setPen(QPen(c, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    auto line = [&](qreal x1, qreal y1, qreal x2, qreal y2)
    { p.drawLine(QPointF(x1, y1), QPointF(x2, y2)); };
    auto rect = [&](qreal x, qreal y, qreal w, qreal h, qreal r = 2)
    { p.drawRoundedRect(QRectF(x, y, w, h), r, r); };
    auto circle = [&](qreal x, qreal y, qreal r)
    { p.drawEllipse(QPointF(x, y), r, r); };
    auto fillShape = [&](auto fn)
    {
        p.save();
        p.setBrush(c);
        fn();
        p.restore();
    };

    if (n == "dashboard")
    {
        rect(3, 3, 7.5, 7.5);
        rect(13.5, 3, 7.5, 7.5);
        rect(3, 13.5, 7.5, 7.5);
        rect(13.5, 13.5, 7.5, 7.5);
    }
    else if (n == "cart")
    {
        QPainterPath path;
        path.moveTo(2, 3.5);
        path.lineTo(5, 3.5);
        path.lineTo(7.2, 15);
        path.lineTo(18.5, 15);
        path.lineTo(20.5, 7);
        path.lineTo(5.6, 7);
        p.drawPath(path);
        circle(9.5, 19.3, 1.5);
        circle(16.5, 19.3, 1.5);
    }
    else if (n == "orders")
    {
        rect(5, 3, 14, 18, 2);
        line(8.5, 8, 15.5, 8);
        line(8.5, 12, 15.5, 12);
        line(8.5, 16, 12.5, 16);
    }
    else if (n == "products")
    {
        rect(3, 4, 18, 5, 1.5);
        rect(4.5, 9, 15, 11, 1.5);
        line(10, 13, 14, 13);
    }
    else if (n == "barcode")
    {
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        const qreal xs[] = {3, 5.5, 8, 10, 13, 15, 17.5, 20};
        const qreal ws[] = {1.4, 0.9, 1.8, 0.9, 1.4, 1.8, 0.9, 1.2};
        for (int i = 0; i < 8; ++i)
            p.drawRect(QRectF(xs[i], 4, ws[i], 16));
    }
    else if (n == "bills")
    {
        QPainterPath path;
        path.moveTo(6, 3);
        path.lineTo(14, 3);
        path.lineTo(19, 8);
        path.lineTo(19, 21);
        path.lineTo(6, 21);
        path.closeSubpath();
        p.drawPath(path);
        line(14, 3, 14, 8);
        line(14, 8, 19, 8);
        line(9, 13, 16, 13);
        line(9, 17, 14, 17);
    }
    else if (n == "settings")
    {
        circle(12, 12, 3);
        circle(12, 12, 7);
        for (int i = 0; i < 8; ++i)
        {
            p.save();
            p.translate(12, 12);
            p.rotate(i * 45);
            line(0, -7, 0, -10);
            p.restore();
        }
    }
    else if (n == "search")
    {
        circle(10.5, 10.5, 6);
        line(15, 15, 20.5, 20.5);
    }
    else if (n == "trash")
    {
        line(4, 6, 20, 6);
        line(9.5, 3.5, 14.5, 3.5);
        QPainterPath path;
        path.moveTo(6, 6);
        path.lineTo(7, 20);
        path.lineTo(17, 20);
        path.lineTo(18, 6);
        p.drawPath(path);
        line(10, 10, 10, 16);
        line(14, 10, 14, 16);
    }
    else if (n == "plus")
    {
        line(12, 5, 12, 19);
        line(5, 12, 19, 12);
    }
    else if (n == "minus")
    {
        line(5, 12, 19, 12);
    }
    else if (n == "play")
    {
        fillShape([&]
                  { p.drawPolygon(QPolygonF({{7, 4.5}, {19, 12}, {7, 19.5}})); });
    }
    else if (n == "pause")
    {
        line(9, 5, 9, 19);
        line(15, 5, 15, 19);
    }
    else if (n == "stop")
    {
        rect(6, 6, 12, 12, 1.5);
    }
    else if (n == "check")
    {
        circle(12, 12, 9);
        p.drawPolyline(QPolygonF({{7.8, 12.3}, {10.7, 15.2}, {16.2, 9.2}}));
    }
    else if (n == "warn")
    {
        p.drawPolygon(QPolygonF({{12, 3.5}, {21.5, 20}, {2.5, 20}}));
        line(12, 9.5, 12, 14);
        line(12, 17, 12, 17.2);
    }
    else if (n == "lock")
    {
        rect(5, 11, 14, 10, 2);
        p.drawArc(QRectF(8, 3, 8, 12), 0, 180 * 16);
    }
    else if (n == "wifi")
    {
        p.drawArc(QRectF(2, 4, 20, 20), 50 * 16, 80 * 16);
        p.drawArc(QRectF(6, 8, 12, 12), 45 * 16, 90 * 16);
        fillShape([&]
                  { circle(12, 18.5, 1.3); });
    }
    else if (n == "clock")
    {
        circle(12, 12, 9);
        p.drawPolyline(QPolygonF({{12, 7}, {12, 12}, {15.5, 14}}));
    }
    else if (n == "user")
    {
        circle(12, 8, 4);
        p.drawArc(QRectF(4, 14, 16, 14), 0, 180 * 16);
    }
    else if (n == "export")
    {
        line(12, 4, 12, 15);
        p.drawPolyline(QPolygonF({{7.5, 10.8}, {12, 15.2}, {16.5, 10.8}}));
        p.drawPolyline(QPolygonF({{4.5, 15}, {4.5, 20}, {19.5, 20}, {19.5, 15}}));
    }
    else if (n == "refresh")
    {
        p.drawArc(QRectF(4, 4, 16, 16), 40 * 16, 270 * 16);
        p.drawPolyline(QPolygonF({{17.5, 2.8}, {17.8, 7.2}, {13.4, 7.4}}));
    }
    else if (n == "filter")
    {
        line(4, 7, 20, 7);
        line(7, 12, 17, 12);
        line(10, 17, 14, 17);
    }
    else if (n == "expand")
    {
        p.drawPolyline(QPolygonF({{14, 4}, {20, 4}, {20, 10}}));
        line(20, 4, 13, 11);
        p.drawPolyline(QPolygonF({{10, 20}, {4, 20}, {4, 14}}));
        line(4, 20, 11, 13);
    }
    else if (n == "camera")
    {
        rect(3, 7, 18, 13, 2.5);
        p.drawPolyline(QPolygonF({{8, 7}, {9.5, 4}, {14.5, 4}, {16, 7}}));
        circle(12, 13.5, 3.5);
    }
    else if (n == "info")
    {
        circle(12, 12, 9);
        line(12, 11, 12, 16.5);
        line(12, 7.8, 12, 8);
    }
    else if (n == "tag")
    {
        QPainterPath path;
        path.moveTo(3.5, 12);
        path.lineTo(12, 3.5);
        path.lineTo(20.5, 3.5);
        path.lineTo(20.5, 12);
        path.lineTo(12, 20.5);
        path.closeSubpath();
        p.drawPath(path);
        circle(16, 8, 1.3);
    }
    else if (n == "shield")
    {
        QPainterPath path;
        path.moveTo(12, 3);
        path.lineTo(19.5, 6);
        path.lineTo(19.5, 12);
        path.cubicTo(19.5, 16.5, 16, 19.5, 12, 21);
        path.cubicTo(8, 19.5, 4.5, 16.5, 4.5, 12);
        path.lineTo(4.5, 6);
        path.closeSubpath();
        p.drawPath(path);
        p.drawPolyline(QPolygonF({{8.8, 12}, {11, 14.2}, {15.2, 9.6}}));
    }
    else if (n == "edit")
    {
        p.drawPolygon(QPolygonF({{5, 19}, {5.8, 15}, {16, 4.8}, {19.2, 8}, {9, 18.2}}));
        line(14, 6.8, 17.2, 10);
    }
    else if (n == "eye")
    {
        QPainterPath path;
        path.moveTo(2.5, 12);
        path.cubicTo(6, 5.5, 18, 5.5, 21.5, 12);
        path.cubicTo(18, 18.5, 6, 18.5, 2.5, 12);
        p.drawPath(path);
        circle(12, 12, 3);
    }
    else if (n == "print")
    {
        rect(4, 9, 16, 8, 2);
        p.drawPolyline(QPolygonF({{7, 9}, {7, 4}, {17, 4}, {17, 9}}));
        rect(7, 14, 10, 6, 1);
    }
    else if (n == "logout")
    {
        p.drawPolyline(QPolygonF({{10, 4}, {4, 4}, {4, 20}, {10, 20}}));
        line(9, 12, 20, 12);
        p.drawPolyline(QPolygonF({{16, 8}, {20, 12}, {16, 16}}));
    }
    else if (n == "card")
    {
        rect(2.5, 5, 19, 14, 2.5);
        line(2.5, 10, 21.5, 10);
        line(6, 15, 10, 15);
    }
    else if (n == "cash")
    {
        rect(2.5, 6, 19, 12, 2);
        circle(12, 12, 2.8);
        line(5.5, 12, 5.6, 12);
        line(18.4, 12, 18.5, 12);
    }
    else if (n == "arrow-right")
    {
        line(5, 12, 19, 12);
        p.drawPolyline(QPolygonF({{13, 6}, {19, 12}, {13, 18}}));
    }
    else if (n == "arrow-left")
    {
        line(19, 12, 5, 12);
        p.drawPolyline(QPolygonF({{11, 6}, {5, 12}, {11, 18}}));
    }
    else if (n == "chevron-down")
    {
        p.drawPolyline(QPolygonF({{6, 9}, {12, 15}, {18, 9}}));
    }
    else if (n == "chevron-up")
    {
        p.drawPolyline(QPolygonF({{6, 15}, {12, 9}, {18, 15}}));
    }
    else if (n == "moon")
    {
        QPainterPath path;
        path.moveTo(20, 14.5);
        path.cubicTo(18.8, 15, 17.6, 15.3, 16.3, 15.3);
        path.cubicTo(11.8, 15.3, 8.2, 11.7, 8.2, 7.2);
        path.cubicTo(8.2, 5.9, 8.5, 4.7, 9, 3.6);
        path.cubicTo(5.8, 4.8, 3.5, 7.9, 3.5, 11.5);
        path.cubicTo(3.5, 16.2, 7.3, 20, 12, 20);
        path.cubicTo(15.6, 20, 18.8, 17.6, 20, 14.5);
        path.closeSubpath();
        p.drawPath(path);
    }
    else if (n == "sun")
    {
        circle(12, 12, 4);
        for (int i = 0; i < 8; ++i)
        {
            p.save();
            p.translate(12, 12);
            p.rotate(i * 45);
            line(0, -7.5, 0, -10);
            p.restore();
        }
    }
    else if (n == "tick")
    {
        p.drawPolyline(QPolygonF({{5.5, 12.5}, {10, 17}, {18.5, 7.5}}));
    }
    return QPixmap(pm);
}

// ====================================================================== palettes
namespace
{
    Theme::Palette makeLight()
    {
        Theme::Palette p;
        p.bg = "#f4f6fb";         p.surface = "#ffffff";    p.surfaceAlt = "#f7f9fd";
        p.hover = "#f1f4fa";      p.border = "#e2e7f0";     p.borderStrong = "#cbd3e1";
        p.text = "#0f1b33";       p.muted = "#5b6781";      p.subtle = "#7b869c";
        p.accentText = "#2554d8"; p.dangerText = "#c0271d"; p.successText = "#0b7a36";
        p.warnText = "#b36b00";
        p.sidebar = "#0f1b33";    p.primary = "#2554d8";    p.primaryHover = "#1e46b8";
        p.primaryPress = "#193c9e"; p.primaryTop = "#3b6ae8"; p.primaryHoverTop = "#4a77ee";
        p.softBg = "#eef3ff";     p.softBorder = "#d3e0ff"; p.softEdge = "#bccffa";
        p.softHover = "#e2ebff";  p.softPress = "#d6e3ff";  p.softText = "#2554d8";
        p.cardBg = "#ffffff";     p.cardBorder = "#e2e7f0"; p.alertBg = "#fffafa";
        p.alertBorder = "#f3c4bf"; p.shadow = QColor(15, 27, 51, 7);
        p.chartBar = "#b7c9f6";   p.chartBarHot = "#2554d8"; p.chartGrid = "#e9edf4";
        p.chartTarget = "#e08a2c"; p.thumbBg = "#e6eeff";
        return p;
    }

    Theme::Palette makeDark()
    {
        Theme::Palette p;
        p.bg = "#0e1526";         p.surface = "#161f35";    p.surfaceAlt = "#1b2641";
        p.hover = "#1f2b49";      p.border = "#27334f";     p.borderStrong = "#38476a";
        p.text = "#e6ebf5";       p.muted = "#a2aecb";      p.subtle = "#8592b0";
        p.accentText = "#86a8ff"; p.dangerText = "#ff8f86"; p.successText = "#62d68f";
        p.warnText = "#f0b955";
        p.sidebar = "#0a1020";    p.primary = "#3b6ae8";    p.primaryHover = "#4a77ee";
        p.primaryPress = "#2f5ad0"; p.primaryTop = "#4874f0"; p.primaryHoverTop = "#5a83f4";
        p.softBg = "#1b2a52";     p.softBorder = "#2b3f78"; p.softEdge = "#253769";
        p.softHover = "#22356a";  p.softPress = "#2a407c";  p.softText = "#9cb8ff";
        p.cardBg = "#161f35";     p.cardBorder = "#27334f"; p.alertBg = "#241a26";
        p.alertBorder = "#5b2f38"; p.shadow = QColor(0, 0, 0, 22);
        p.chartBar = "#2c4175";   p.chartBarHot = "#5b8cff"; p.chartGrid = "#27334f";
        p.chartTarget = "#f0a04a"; p.thumbBg = "#1f2f5a";
        return p;
    }

    bool g_dark = false;

    // Qt style sheets can only draw glyphs (check mark, spin/combo arrows) from image files, so the
    // few we need are painted once with Theme::iconPixmap and written to a temp folder.
    QString glyph(const QString &name, const QColor &color, int px)
    {
        static QTemporaryDir dir;
        if (!dir.isValid())
            return QString();
        const QString file = QDir(dir.path()).filePath(QString("%1-%2.png").arg(name, color.name().mid(1)));
        if (!QFile::exists(file))
            Theme::iconPixmap(name, color, px).save(file, "PNG");
        return QDir::fromNativeSeparators(file);
    }

    QPalette qtPalette(const Theme::Palette &c)
    {
        // Native-looking widgets that the style sheet does not cover (message boxes, input
        // dialogs, tooltips) read their colours from here.
        QPalette p;
        p.setColor(QPalette::Window, c.bg);
        p.setColor(QPalette::WindowText, c.text);
        p.setColor(QPalette::Base, c.surface);
        p.setColor(QPalette::AlternateBase, c.surfaceAlt);
        p.setColor(QPalette::Text, c.text);
        p.setColor(QPalette::Button, c.surface);
        p.setColor(QPalette::ButtonText, c.text);
        p.setColor(QPalette::PlaceholderText, c.subtle);
        p.setColor(QPalette::Highlight, c.primary);
        p.setColor(QPalette::HighlightedText, Qt::white);
        p.setColor(QPalette::ToolTipBase, c.sidebar);
        p.setColor(QPalette::ToolTipText, Qt::white);
        p.setColor(QPalette::Disabled, QPalette::Text, c.subtle);
        p.setColor(QPalette::Disabled, QPalette::ButtonText, c.subtle);
        return p;
    }
}

const Theme::Palette &Theme::light()
{
    static const Palette p = makeLight();
    return p;
}

const Theme::Palette &Theme::dark()
{
    static const Palette p = makeDark();
    return p;
}

const Theme::Palette &Theme::palette() { return g_dark ? dark() : light(); }
bool Theme::isDark() { return g_dark; }

ThemeNotifier &Theme::notifier()
{
    static ThemeNotifier n;
    return n;
}

void Theme::restore()
{
    g_dark = QSettings("BarcodeHub", "SmartPOS").value("ui/darkMode", false).toBool();
}

void Theme::setDark(bool on)
{
    if (on == g_dark)
        return;
    g_dark = on;
    QSettings("BarcodeHub", "SmartPOS").setValue("ui/darkMode", on);
    if (auto *app = qApp)
    {
        apply(*app);
        for (QWidget *w : app->allWidgets()) // painted widgets (cards, chart) redraw in the new colours
            w->update();
    }
    emit notifier().changed(on);
}

void Theme::apply(QApplication &app)
{
    app.setPalette(qtPalette(palette()));
    app.setStyleSheet(styleSheet());
}

// ====================================================================== style sheet
QString Theme::styleSheet()
{
    const Palette &c = palette();
    // Design tokens (@name) are substituted below, so every colour lives in the palette.
    QString css = QStringLiteral(R"(
* { font-family: "Segoe UI Variable Text", "Segoe UI", "Inter", "Helvetica Neue", Arial, sans-serif; font-size: 13px; color: @text; }
QMainWindow, #root, #page { background: @bg; }
QToolTip { background: @sidebar; color: #ffffff; border: none; padding: 5px 9px; border-radius: 6px; }

/* ---- sidebar: flat navy ---- */
#sidebar { background: @sidebar; border: none; }
#logoBox { background: @primary; border-radius: 10px; }
#logoBox QLabel { color: #ffffff; font-weight: 700; font-size: 18px; background: transparent; }
#brand { color: #ffffff; font-size: 17px; font-weight: 700; background: transparent; }
#brandSub { color: #8d9abd; font-size: 11px; background: transparent; }
#userCard { background: rgba(255,255,255,6%); border: 1px solid rgba(255,255,255,8%); border-radius: 10px; }
#avatar { background: #31436b; border-radius: 17px; color: #ffffff; font-weight: 700; font-size: 12px; }
#userName { color: #ffffff; font-weight: 600; font-size: 13px; background: transparent; }
#userRole { color: #8d9abd; font-size: 11.5px; background: transparent; }
#nav { background: transparent; border: none; border-radius: 8px; text-align: left; }
#nav:hover { background: rgba(255,255,255,7%); }
#nav[active="true"] { background: @primary; border-bottom: 2px solid @primaryPress; }
#nav QLabel { background: transparent; font-weight: 600; font-size: 13.5px; color: #aab6d3; }
#nav[active="true"] QLabel { color: #ffffff; }
#nav[locked="true"] QLabel { color: #5d6a8f; }
#exitBtn { background: transparent; border: 1px solid rgba(255,255,255,12%); border-radius: 8px; color: #c4cee8; padding: 8px 12px; font-weight: 600; }
#exitBtn:hover { background: rgba(255,255,255,8%); }
#exitBtn:pressed { background: rgba(255,255,255,12%); }

/* ---- top bar ---- */
#topbar { background: @surface; border: none; border-bottom: 1px solid @border; }
#pageTitle { font-size: 20px; font-weight: 700; color: @text; background: transparent; }
#pageSub { color: @muted; font-size: 12px; background: transparent; }
#chip { background: @bg; border: 1px solid @border; border-radius: 10px; }
#clockTime { font-weight: 700; font-size: 14px; background: transparent; }
#clockDate { color: @muted; font-size: 11px; background: transparent; }
QPushButton#themeToggle { padding: 0; min-width: 38px; max-width: 38px; min-height: 36px; max-height: 36px; border-radius: 10px; }

/* ---- panels ---- */
#subCard { background: @bg; border: 1px solid @border; border-radius: 10px; }
#field { background: @bg; border: 1px solid @border; border-radius: 8px; }
#notice { background: #fff1ef; border: 1px solid #ffc9c2; border-radius: 8px; }
#notice QLabel { color: #93000a; background: transparent; }
#totalBox { background: @sidebar; border: 1px solid @border; border-radius: 12px; }
#totalBox QLabel { color: #ffffff; background: transparent; }
#totalBox #caption { color: #9fb0d8; }
#toast { background: @sidebar; color: #ffffff; border: 1px solid @border; border-radius: 10px; padding: 10px 18px; font-weight: 600; }
#hline { background: @border; border: none; }

/* ---- typography ---- */
QLabel { background: transparent; }
#h1 { font-size: 15px; font-weight: 700; }
#h2 { font-size: 14px; font-weight: 600; }
#muted { color: @muted; }
#subtle { color: @subtle; }
#caption { color: @subtle; font-size: 10.5px; font-weight: 700; letter-spacing: 0.8px; }
#bigValue { font-size: 26px; font-weight: 700; }
#bigValueAlert { font-size: 26px; font-weight: 700; color: @dangerText; }
#bigTotal { font-size: 32px; font-weight: 700; color: @accentText; }
#price { color: @accentText; font-size: 17px; font-weight: 700; }
#mono { font-family: "Consolas", "Courier New", monospace; }
#link { color: @accentText; font-weight: 600; background: transparent; border: none; text-align: right; padding: 2px 4px; }
#link:hover { text-decoration: underline; background: transparent; }
#link:pressed { padding: 2px 4px; border: none; }

/* ---- badges ---- */
QLabel[badge] { border-radius: 6px; padding: 2px 9px; font-size: 11px; font-weight: 700; }
QLabel[badge="green"] { background: @gBg; color: @gFg; }
QLabel[badge="blue"]  { background: @bBg; color: @bFg; }
QLabel[badge="red"]   { background: @rBg; color: @rFg; }
QLabel[badge="grey"]  { background: @nBg; color: @nFg; }
QLabel[badge="amber"] { background: @aBg; color: @aFg; }

/* ---- buttons: flat base, slightly raised primary actions ---- */
QPushButton { border-radius: 8px; padding: 7px 14px; font-weight: 600; color: @text;
  border: 1px solid @borderStrong; background: @surface; }
QPushButton:hover { background: @hover; border-color: @subtle; }
QPushButton:pressed { background: @border; }
QPushButton:disabled { color: @disabledFg; border-color: @border; background: @disabledBg; }
QPushButton[kind="primary"] { color: #ffffff; border: 1px solid @primary; border-bottom: 3px solid @primaryPress; padding: 7px 14px 5px 14px;
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 @primaryTop, stop:1 @primary); }
QPushButton[kind="primary"]:hover { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 @primaryHoverTop, stop:1 @primaryHover); }
QPushButton[kind="primary"]:pressed { border-bottom: 1px solid @primaryPress; padding: 8px 14px 6px 14px; background: @primaryPress; }
QPushButton[kind="primary"]:disabled { color: @primaryOffFg; border: 1px solid @primaryOff; border-bottom: 3px solid @primaryOffEdge; border-radius: 8px; background: @primaryOff; }
QPushButton[kind="soft"] { color: @softText; border: 1px solid @softBorder; border-bottom: 2px solid @softEdge; padding: 7px 14px 6px 14px; background: @softBg; }
QPushButton[kind="soft"]:hover { background: @softHover; border-color: @softEdge; }
QPushButton[kind="soft"]:pressed { border-bottom: 1px solid @softEdge; padding: 8px 14px 6px 14px; background: @softPress; }
QPushButton[kind="soft"]:disabled { color: @disabledFg; border-color: @border; border-bottom: 2px solid @border; background: @disabledBg; }
QPushButton[kind="danger"] { color: @dangerText; border: 1px solid @dangerBorder; background: @surface; }
QPushButton[kind="danger"]:hover { background: @dangerHover; }
QPushButton[kind="danger"]:pressed { background: @dangerPress; }
QPushButton[kind="flat"] { border: 1px solid transparent; background: transparent; padding: 5px 8px; }
QPushButton[kind="flat"]:hover { background: @hover; }
QPushButton[kind="flat"]:pressed { background: @border; }
QPushButton[kind="tab"] { color: @muted; padding: 6px 14px; border-radius: 14px; border: 1px solid @border; background: @surface; }
QPushButton[kind="tab"]:hover { background: @hover; }
QPushButton[kind="tab"]:checked { color: #ffffff; border: 1px solid @primary; border-bottom: 2px solid @primaryPress; border-radius: 14px; padding: 6px 14px 5px 14px; background: @primary; }
QPushButton[kind="mini"] { padding: 4px 11px; font-size: 12px; border-radius: 7px; color: @softText; border: 1px solid @softBorder; background: @miniBg; }
QPushButton[kind="mini"]:hover { background: @softHover; }
QPushButton[kind="mini"]:pressed { background: @softPress; }
QPushButton[kind="mini"]:disabled { color: @disabledFg; border-color: @border; background: @disabledBg; }
QPushButton[kind="step"] { padding: 0; border-radius: 7px; min-width: 26px; max-width: 26px; min-height: 24px; max-height: 24px; font-size: 15px; font-weight: 700; color: @softText;
  border: 1px solid @softBorder; background: @miniBg; }
QPushButton[kind="step"]:hover { background: @softHover; }
QPushButton[kind="icon"] { padding: 0; min-width: 36px; max-width: 36px; min-height: 34px; max-height: 34px; }

/* ---- inputs ---- */
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { background: @surface; border: 1px solid @borderStrong; border-radius: 8px; padding: 7px 11px; selection-background-color: @selection; selection-color: @text; }
QLineEdit:hover, QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover { border-color: @subtle; }
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus { border: 1px solid @primary; }
QLineEdit[flat="true"] { background: transparent; border: none; padding: 0; font-weight: 600; font-size: 14px; }
QSpinBox, QDoubleSpinBox { padding-right: 24px; }
QSpinBox::up-button, QDoubleSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; width: 22px; border: none; border-left: 1px solid @border; border-top-right-radius: 8px; background: transparent; }
QSpinBox::down-button, QDoubleSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; width: 22px; border: none; border-left: 1px solid @border; border-bottom-right-radius: 8px; background: transparent; }
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover, QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover { background: @hover; }
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow { image: url(@chevUp); width: 10px; height: 10px; }
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow { image: url(@chevDown); width: 10px; height: 10px; }
QComboBox::drop-down { border: none; width: 24px; }
QComboBox::down-arrow { image: url(@chevDown); width: 10px; height: 10px; }
QComboBox QAbstractItemView { background: @surface; border: 1px solid @border; selection-background-color: @rowSel; selection-color: @text; outline: none; }
QPlainTextEdit, QTextEdit { background: @surface; border: 1px solid @border; border-radius: 8px; selection-background-color: @selection; }
QPlainTextEdit#paper { border-radius: 10px; padding: 14px; }

/* ---- tables ---- */
QTableWidget { background: transparent; border: none; gridline-color: transparent; selection-background-color: @rowSel; selection-color: @text; outline: none; alternate-background-color: transparent; }
QTableWidget#boxedTable { background: @surface; border: 1px solid @border; border-radius: 10px; }
QTableWidget::item { padding: 4px 8px; border-bottom: 1px solid @rowLine; }
QTableWidget::item:hover { background: @rowHover; }
QTableWidget::item:selected { background: @rowSel; color: @text; }
QHeaderView { background: transparent; }
QHeaderView::section { background: transparent; color: @subtle; border: none; border-bottom: 1px solid @border; padding: 8px 8px; font-size: 10.5px; font-weight: 700; }
QTableCornerButton::section { background: transparent; border: none; }
QListWidget { background: @listBg; border: 1px solid @border; border-radius: 8px; padding: 4px; outline: none; }
QListWidget::item { padding: 9px 8px; border-bottom: 1px solid @rowLine; font-family: "Consolas", "Courier New", monospace; font-size: 12px; }
QListWidget::item:selected { background: @rowSel; color: @text; }

/* ---- scrollbars ---- */
QScrollArea { border: none; background: transparent; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: @scroll; border-radius: 3px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: @scrollHover; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle:horizontal { background: @scroll; border-radius: 3px; min-width: 30px; }
QScrollBar::handle:horizontal:hover { background: @scrollHover; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QCheckBox { spacing: 9px; background: transparent; }
QCheckBox::indicator { width: 18px; height: 18px; border-radius: 5px; border: 1px solid @checkBorder; background: @surface; }
QCheckBox::indicator:hover { border-color: @primary; }
QCheckBox::indicator:checked { background: @primary; border-color: @primary; image: url(@tick); }
QDialog, QMessageBox, QInputDialog { background: @bg; }
QMessageBox QPushButton { min-width: 76px; }
)");

    const bool d = g_dark;
    const QHash<QString, QString> tokens = {
        {"@bg", c.bg.name()}, {"@surface", c.surface.name()}, {"@hover", c.hover.name()},
        {"@border", c.border.name()}, {"@borderStrong", c.borderStrong.name()},
        {"@text", c.text.name()}, {"@muted", c.muted.name()}, {"@subtle", c.subtle.name()},
        {"@accentText", c.accentText.name()}, {"@dangerText", c.dangerText.name()},
        {"@sidebar", c.sidebar.name()},
        {"@primary", c.primary.name()}, {"@primaryHover", c.primaryHover.name()},
        {"@primaryPress", c.primaryPress.name()}, {"@primaryTop", c.primaryTop.name()},
        {"@primaryHoverTop", c.primaryHoverTop.name()},
        {"@primaryOffEdge", d ? "#1a2540" : "#93a9e6"}, {"@primaryOff", d ? "#1f2b49" : "#a9bcef"},
        {"@primaryOffFg", d ? "#6f7ea3" : "#ffffff"},
        {"@softBg", c.softBg.name()}, {"@softBorder", c.softBorder.name()}, {"@softEdge", c.softEdge.name()},
        {"@softHover", c.softHover.name()}, {"@softPress", c.softPress.name()}, {"@softText", c.softText.name()},
        {"@miniBg", d ? "#18264a" : "#f5f8ff"},
        {"@dangerBorder", d ? "#6a343b" : "#f4c7c3"}, {"@dangerHover", d ? "#2e1c25" : "#fdecea"},
        {"@dangerPress", d ? "#3a222c" : "#fadbd7"},
        {"@disabledFg", d ? "#5d6a8c" : "#a3adc2"}, {"@disabledBg", d ? "#121a2d" : "#f4f6fa"},
        {"@selection", d ? "#2d4a8a" : "#bcd0fb"}, {"@rowSel", d ? "#1d2c52" : "#eef3ff"},
        {"@rowLine", d ? "#222d48" : "#edf0f6"}, {"@rowHover", d ? "#1a2540" : "#f7f9fd"},
        {"@listBg", d ? "#121a2d" : "#fafbfe"},
        {"@scroll", d ? "#33415f" : "#ccd4e3"}, {"@scrollHover", d ? "#4a5b80" : "#aab5cb"},
        {"@checkBorder", d ? "#4a5b80" : "#b3bfd3"},
        {"@gBg", d ? "#16361f" : "#e3f6ea"}, {"@gFg", d ? "#7be3a1" : "#12663a"},
        {"@bBg", d ? "#1b2b52" : "#e6eeff"}, {"@bFg", d ? "#9cb8ff" : "#1d46b4"},
        {"@rBg", d ? "#3d1c20" : "#fde8e6"}, {"@rFg", d ? "#ff9d96" : "#9b1c1c"},
        {"@nBg", d ? "#232d45" : "#eef1f6"}, {"@nFg", d ? "#b3bdd6" : "#4b5873"},
        {"@aBg", d ? "#3d3010" : "#fff3d6"}, {"@aFg", d ? "#f3c36a" : "#7a4b00"},
        {"@chevUp", glyph("chevron-up", c.muted, 20)},
        {"@chevDown", glyph("chevron-down", c.muted, 20)},
        {"@tick", glyph("tick", QColor("#ffffff"), 20)},
    };
    // longest keys first so "@primaryHover" is not clobbered by "@primary"
    QStringList keys = tokens.keys();
    std::sort(keys.begin(), keys.end(), [](const QString &a, const QString &b)
              { return a.size() > b.size(); });
    for (const QString &k : keys)
        css.replace(k, tokens.value(k));
    return css;
}
