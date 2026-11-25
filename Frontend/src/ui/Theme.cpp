#include "Theme.h"
#include <QPainter>
#include <cmath>
#include <QSettings>
#include <QPainterPath>
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
    else if (n == "close")
    {
        line(6, 6, 18, 18);
        line(18, 6, 6, 18);
    }
    else if (n == "inventory")
    {
        rect(3.5, 4, 17, 5, 1.5);
        rect(5, 9, 14, 11, 1.5);
        line(10, 13, 14, 13);
    }
    else if (n == "categories")
    {
        p.drawPolygon(QPolygonF({{7.5, 3.5}, {11.5, 10}, {3.5, 10}}));
        rect(13, 13, 7.5, 7.5, 1.5);
        circle(7.5, 16.8, 3.7);
    }
    else if (n == "reports")
    {
        line(4, 20, 20, 20);
        rect(5, 12, 3, 8, 0.8);
        rect(10.5, 8, 3, 12, 0.8);
        rect(16, 4, 3, 16, 0.8);
    }
    else if (n == "percent")
    {
        line(19, 5, 5, 19);
        circle(7, 7, 2.4);
        circle(17, 17, 2.4);
    }
    else if (n == "moon")
    {
        QPainterPath path;
        path.moveTo(20, 14.5);
        path.cubicTo(13.5, 17.5, 6.5, 12.5, 8.5, 4);
        path.cubicTo(4, 5, 2.5, 11, 5.5, 15.5);
        path.cubicTo(8.5, 20.5, 17, 21, 20, 14.5);
        p.drawPath(path);
    }
    else if (n == "download")
    {
        line(12, 4, 12, 15);
        p.drawPolyline(QPolygonF({{7.5, 10.8}, {12, 15.2}, {16.5, 10.8}}));
        line(5, 20, 19, 20);
    }
    else if (n == "unlock")
    {
        rect(5, 11, 14, 10, 2);
        p.drawArc(QRectF(8, 3, 8, 12), 0, 150 * 16);
    }
    else if (n == "history")
    {
        p.drawArc(QRectF(4, 4, 16, 16), 20 * 16, 300 * 16);
        p.drawPolyline(QPolygonF({{3.5, 4.5}, {4.2, 9}, {8.6, 8.2}}));
        p.drawPolyline(QPolygonF({{12, 8}, {12, 12}, {15, 13.8}}));
    }
    else if (n == "mail")
    {
        rect(3, 5.5, 18, 13, 2);
        p.drawPolyline(QPolygonF({{3.5, 7}, {12, 13.5}, {20.5, 7}}));
    }
    else if (n == "message")
    {
        QPainterPath path;
        path.moveTo(4, 5);
        path.lineTo(20, 5);
        path.lineTo(20, 16);
        path.lineTo(10, 16);
        path.lineTo(6, 20);
        path.lineTo(6, 16);
        path.lineTo(4, 16);
        path.closeSubpath();
        p.drawPath(path);
        line(8, 9.5, 16, 9.5);
        line(8, 12.5, 13, 12.5);
    }
    else if (n == "bolt")
    {
        fillShape([&]
                  { p.drawPolygon(QPolygonF({{13.5, 2.5}, {5.5, 13.5}, {11, 13.5}, {10, 21.5}, {18.5, 10}, {13, 10}})); });
    }
    else if (n == "qr")
    {
        rect(3.5, 3.5, 6, 6, 1);
        rect(14.5, 3.5, 6, 6, 1);
        rect(3.5, 14.5, 6, 6, 1);
        line(14.5, 14.5, 14.5, 14.7);
        line(18, 14.5, 20.5, 14.5);
        line(14.5, 18, 14.5, 20.5);
        line(18, 18, 20.5, 20.5);
    }
    else if (n == "calendar")
    {
        rect(3.5, 5, 17, 15.5, 2.5);
        line(3.5, 10, 20.5, 10);
        line(8, 3, 8, 7);
        line(16, 3, 16, 7);
    }
    else if (n == "wallet")
    {
        rect(3, 6, 18, 13, 2.5);
        p.drawPolyline(QPolygonF({{3.5, 9.5}, {17, 9.5}}));
        circle(16.5, 13.5, 1.2);
    }
    else if (n == "idcard")
    {
        rect(3, 5.5, 18, 13, 2);
        circle(8.5, 11.5, 2);
        line(13, 10.5, 18, 10.5);
        line(13, 14, 17, 14);
    }
    else if (n == "undo")
    {
        p.drawPolyline(QPolygonF({{8, 6}, {4, 10}, {8, 14}}));
        QPainterPath path;
        path.moveTo(4.5, 10);
        path.lineTo(14, 10);
        path.cubicTo(19, 10, 19.5, 18, 14, 18);
        path.lineTo(9, 18);
        p.drawPath(path);
    }
    else if (n == "star")
    {
        QPolygonF st;
        for (int i = 0; i < 10; ++i)
        {
            const qreal r = i % 2 == 0 ? 9 : 4;
            const qreal a = -M_PI / 2 + i * M_PI / 5;
            st << QPointF(12 + r * std::cos(a), 12.5 + r * std::sin(a));
        }
        p.drawPolygon(st);
    }
    else if (n == "drawer")
    {
        rect(3, 9, 18, 10, 2);
        line(3, 13, 21, 13);
        line(10, 16, 14, 16);
        p.drawPolyline(QPolygonF({{6, 9}, {8, 4}, {16, 4}, {18, 9}}));
    }
    else if (n == "building")
    {
        rect(5, 4, 14, 17, 1.5);
        line(9, 8, 9, 8.2);
        line(15, 8, 15, 8.2);
        line(9, 12, 9, 12.2);
        line(15, 12, 15, 12.2);
        line(10, 21, 10, 17);
        line(14, 21, 14, 17);
    }
    else if (n == "sun")
    {
        circle(12, 12, 4);
        for (int i = 0; i < 8; ++i)
        {
            p.save();
            p.translate(12, 12);
            p.rotate(i * 45);
            line(0, -7, 0, -9.5);
            p.restore();
        }
    }
    else if (n == "scan")
    {
        p.drawPolyline(QPolygonF({{4, 9}, {4, 4}, {9, 4}}));
        p.drawPolyline(QPolygonF({{15, 4}, {20, 4}, {20, 9}}));
        p.drawPolyline(QPolygonF({{20, 15}, {20, 20}, {15, 20}}));
        p.drawPolyline(QPolygonF({{9, 20}, {4, 20}, {4, 15}}));
        line(7.5, 12, 16.5, 12);
    }
    return QPixmap(pm);
}

namespace
{
    int g_dark = -1;
}

bool Theme::isDark()
{
    if (g_dark < 0)
        g_dark = QSettings("BarcodeHub", "SmartPOS").value("darkMode", false).toBool() ? 1 : 0;
    return g_dark == 1;
}

void Theme::setDark(bool on)
{
    g_dark = on ? 1 : 0;
    QSettings("BarcodeHub", "SmartPOS").setValue("darkMode", on);
}

QColor Theme::readable(const QColor &c)
{
    // In dark mode a dark colour (e.g. dark blue text) would vanish on the dark cards:
    // keep its hue but raise its lightness.
    if (!isDark() || c.lightnessF() >= 0.6)
        return c;
    return QColor::fromHslF(c.hslHueF() < 0 ? 0 : c.hslHueF(), c.hslSaturationF(), 0.72);
}

QString Theme::styleSheet()
{
    QString sheet = QStringLiteral(R"(
* { font-family: "Segoe UI"; font-size: 13px; color: @text@; }
QMainWindow, #root, #page { background: @canvas@; }
QToolTip { background: @navy@; color: @white@; border: 1px solid #415274; padding: 5px 9px; border-radius: 8px; }

/* Sidebar */
#sidebar { background: qlineargradient(x1:0, y1:0, x2:0.85, y2:1, stop:0 @sidebarTop@, stop:1 @sidebarBottom@); border-right: 1px solid #2b3e62; }
#logoBox, #avatar, #topAvatar { background: @brand@; border-radius: 12px; }
#logoBox { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #4179ee, stop:0.68 #2b54bb, stop:1 #88574f); }
#logoBox QLabel { color: @white@; font-weight: 700; font-size: 18px; background: transparent; }
#brand { color: @white@; font-size: 18px; font-weight: 700; }
#brandSub { color: #b9c6da; font-size: 11px; }
#sidebar #tagBlue, #sidebar #tagGrey { background: transparent; color: #b9c6da; font-size: 11px; font-weight: 400; padding: 0; }
#sidebar #navSection { color: #a4b2ca; font-size: 11px; font-weight: 600; padding: 0 12px; }
#userCard { background: #202f50; border: 1px solid #394969; border-radius: 12px; }
#avatar { color: @white@; font-weight: 700; font-size: 12px; }
#userName { color: @white@; font-weight: 700; background: transparent; }
#userRole { color: #b9c6da; font-size: 12px; background: transparent; }
#nav { background: transparent; border: 1px solid transparent; border-radius: 10px; text-align: left; }
#nav:hover { background: #243456; }
#nav:pressed { background: #304467; }
#nav[active="true"] { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 @navActive@, stop:1 @navTail@); border: 1px solid #5379bf; }
#nav[active="true"]:hover { background: @brandHover@; }
#nav[active="true"]:pressed { background: @brandPressed@; }
#nav QLabel { background: transparent; font-weight: 600; font-size: 14px; color: #b9c6da; }
#nav[locked="true"] QLabel { color: #a4b2ca; }
#nav[active="true"] QLabel { color: @white@; }
#exitBtn { background: #202f50; border: 1px solid #64789b; border-radius: 10px; color: #d7e1f2; padding: 8px 12px; }
#exitBtn:hover { background: #2a3c60; }
#exitBtn:pressed { background: #344970; }
#nav:focus, #exitBtn:focus { border-color: #dce6ff; }

/* Top bar and panels */
#topbar { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 @panel@, stop:1 @panelLow@); border-bottom: 1px solid @divider@; }
#pageTitle { font-size: 21px; font-weight: 700; color: @text@; }
#pageSub { color: @muted@; font-size: 12px; }
#chip { background: @panelLow@; border: 1px solid @divider@; border-radius: 10px; }
#clockTime { font-weight: 700; font-size: 14px; }
#clockDate { color: @muted@; font-size: 11px; }
#topbar #readyPill { background: @panelLow@; border: 1px solid @divider@; border-radius: 10px; }
#topbar #readyText { color: @muted@; font-weight: 600; }
#topbar #topAvatar { color: @white@; font-weight: 700; border: 1px solid transparent; }
#topbar #topAvatar:hover { background: @brandHover@; }
#topbar #topAvatar:pressed, #topbar #topAvatar:open { background: @brandPressed@; }
#topbar #topAvatar:focus { border-color: #dce6ff; }
QToolButton#topAvatar::menu-indicator { image: none; width: 0; }
QMenu#accountMenu { background: @panel@; border: 1px solid @divider@; padding: 4px; }
QMenu#accountMenu::item { padding: 8px 12px; border-radius: 6px; }
QMenu#accountMenu::item:selected { background: @hover@; color: @text@; }
QMenu#accountMenu::separator { height: 1px; background: @divider@; margin: 4px 8px; }

/* Match the shared controls even after DarkModeManager appends its toggle rules. */
#topbar #darkModeToggle { background: @panel@; border: 1px solid @outline@; border-radius: 10px; padding: 5px 12px; color: @text@; font-size: 12px; font-weight: 600; }
#topbar #darkModeToggle:hover { background: @hover@; }
#topbar #darkModeToggle:pressed { background: @pressed@; padding: 5px 12px; }
#topbar #darkModeToggle:focus { border-color: @focus@; }

/* Dashboard */
#dashboardBody { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 @dashboardWash@, stop:0.55 @canvas@, stop:1 @panelLow@); }
#dashboardHero { border-radius: 18px; background: qlineargradient(x1:0, y1:0, x2:1, y2:0.85, stop:0 @heroStart@, stop:0.52 @heroMid@, stop:0.82 @heroViolet@, stop:1 @heroWarm@); }
#dashboardHeroTitle { color: #ffffff; font-size: 34px; font-weight: 800; }
#dashboardHeroDate { color: #e1eaff; font-size: 13px; }
#dashboardSearch { background: rgba(14, 24, 54, 57%); border: 1px solid rgba(255, 255, 255, 28%); border-radius: 13px; }
#dashboardSearchTitle { color: #ffffff; font-weight: 600; }
#dashboardCheckout { background: #2554d8; border: 1px solid #94b4ff; color: #ffffff; }
#dashboardCheckout:hover { background: #2049bd; }
#dashboardCheckout:pressed { background: #193c9f; }
#dashboardCheckout:focus { border-color: #ffffff; }
#dashboardPanel { background: @panel@; border: 1px solid @divider@; border-radius: 15px; }
#dashboardKpi { color: @text@; font-size: 22px; font-weight: 800; }
#dashboardIcon { border-radius: 9px; }
#dashboardIcon[tone="blue"] { background: #dce8ff; }
#dashboardIcon[tone="green"] { background: #dcefe6; }
#dashboardIcon[tone="amber"] { background: #fae8cb; }
#dashboardIcon[tone="rose"] { background: #f8e0e8; }
#dashboardAction { background: @panelLow@; color: @text@; text-align: left; border: 1px solid @divider@; }
#dashboardAction:hover { background: @selection@; }
#dashboardAction:pressed { background: @softPressed@; }
#dashboardAction:focus { border-color: @focus@; }

#subCard { background: @panel@; border: 1px solid @divider@; border-radius: 12px; }
#field { background: @panelLow@; border: 1px solid @divider@; border-radius: 10px; }
#notice { background: @dangerBg@; border: 1px solid @dangerBorder@; border-radius: 10px; }
#notice QLabel { color: @dangerText@; background: transparent; }
#totalBox { background: @navy@; border-radius: 12px; }
#totalBox QLabel { color: @white@; background: transparent; }
#totalBox #caption { color: #bdcbee; }
#toast { background: @navy@; color: @white@; border-radius: 12px; padding: 11px 18px; font-weight: 600; }

/* Typography and status */
#h1 { font-size: 16px; font-weight: 700; }
#h2 { font-size: 14px; font-weight: 700; }
#muted, #caption { color: @muted@; }
#caption { font-size: 11px; font-weight: 700; }
#bigValue { font-size: 28px; font-weight: 700; }
#bigTotal { font-size: 32px; font-weight: 700; color: @accent@; }
#checkoutPrice { font-size: 17px; font-weight: 700; color: @accent@; }
#checkoutStock { font-weight: 700; color: @successText@; }
#checkoutStock[lowStock="true"], #errorText { font-weight: 700; color: @dangerText@; }
#quantityValue { font-weight: 700; color: @accent@; }
#mono { font-family: "Consolas"; }
#link { color: @accent@; font-weight: 600; background: transparent; border: 1px solid transparent; border-radius: 8px; text-align: right; padding: 1px 4px; }
#link:hover { text-decoration: underline; background: transparent; }
#link:pressed { background: @hover@; }
#link:focus { border-color: @focus@; }

QLabel[badge] { border-radius: 8px; padding: 2px 9px; font-size: 11px; font-weight: 700; }
QLabel[badge="green"], #readyPill { background: @successBg@; color: @successText@; border: 1px solid @successBorder@; }
QLabel[badge="blue"] { background: @softBg@; color: @accent@; border: 1px solid @softBorder@; }
QLabel[badge="red"] { background: @dangerBg@; color: @dangerText@; border: 1px solid @dangerBorder@; }
QLabel[badge="grey"] { background: @hover@; color: @muted@; border: 1px solid @divider@; }
QLabel[badge="amber"] { background: @warningBg@; color: @warningText@; border: 1px solid @warningBorder@; }

/* Buttons */
QPushButton { background: @panel@; color: @text@; border: 1px solid @outline@; border-radius: 10px; padding: 7px 16px; font-weight: 600; }
QPushButton:hover { background: @hover@; }
QPushButton:pressed { background: @pressed@; }
QPushButton:focus { border-color: @focus@; }
QPushButton[kind="primary"], QPushButton[kind="tab"]:checked { background: @brand@; color: @white@; border-color: @brand@; }
QPushButton[kind="primary"]:hover, QPushButton[kind="tab"]:checked:hover { background: @brandHover@; }
QPushButton[kind="primary"]:pressed, QPushButton[kind="tab"]:checked:pressed { background: @brandPressed@; }
QPushButton[kind="primary"]:focus, QPushButton[kind="tab"]:checked:focus { border-color: #dce6ff; }
QPushButton[kind="soft"] { background: @softBg@; color: @accent@; }
QPushButton[kind="soft"]:hover { background: @selection@; }
QPushButton[kind="soft"]:pressed { background: @softPressed@; }
QPushButton[kind="danger"] { background: @dangerBg@; color: @dangerText@; border-color: @dangerBorder@; }
QPushButton[kind="danger"]:hover { background: @dangerHover@; }
QPushButton[kind="danger"]:pressed { background: @dangerPressed@; }
QPushButton[kind="danger"]:focus { border-color: @focus@; }
QPushButton[kind="flat"] { border-color: transparent; background: transparent; padding: 5px 8px; }
QPushButton[kind="flat"]:hover { background: @hover@; }
QPushButton[kind="flat"]:pressed { background: @pressed@; }
QPushButton[kind="flat"]:focus { border-color: @focus@; }
QPushButton[kind="tab"] { padding: 6px 15px; }
QPushButton[kind="mini"] { padding: 4px 11px; font-size: 12px; border-radius: 8px; color: @accent@; }
QPushButton[kind="step"] { padding: 0; border-radius: 8px; min-width: 26px; max-width: 26px; min-height: 24px; max-height: 24px; font-size: 15px; font-weight: 700; color: @accent@; }
QPushButton[kind="icon"] { padding: 0; min-width: 36px; max-width: 36px; min-height: 34px; max-height: 34px; }
QPushButton:disabled, QPushButton[kind]:disabled { background: @hover@; color: @muted@; border-color: @divider@; }

/* Fields */
QLineEdit, QAbstractSpinBox, QComboBox {
    background: @panel@; border: 1px solid @outline@; border-radius: 10px; padding: 7px 11px;
    selection-background-color: @selection@; selection-color: @text@;
}
QLineEdit { placeholder-text-color: @muted@; }
QLineEdit:hover, QAbstractSpinBox:hover, QComboBox:hover { border-color: @muted@; }
QLineEdit:focus, QAbstractSpinBox:focus, QComboBox:focus { border-color: @focus@; }
QLineEdit:disabled, QAbstractSpinBox:disabled, QComboBox:disabled { background: @hover@; color: @muted@; border-color: @divider@; }
QLineEdit[flat="true"] { background: transparent; border: none; border-bottom: 1px solid transparent; border-radius: 0; padding: 0; font-weight: 700; font-size: 14px; }
QLineEdit[flat="true"]:focus { border-bottom-color: @focus@; }
QComboBox::drop-down, QDateEdit::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 22px; height: 18px; margin-right: 5px; }
QComboBox QAbstractItemView { background: @panel@; border: 1px solid @outline@; selection-background-color: @selection@; selection-color: @text@; }
QCalendarWidget QAbstractItemView { background: @panel@; color: @text@; selection-background-color: @brand@; selection-color: @white@; }
QCheckBox { spacing: 9px; border: 1px solid transparent; border-radius: 8px; }
QCheckBox:focus { border-color: @focus@; }
QCheckBox::indicator:unchecked { background: @panel@; border: 1px solid @outline@; border-radius: 3px; width: 14px; height: 14px; }

/* Tables and scrolling */
QTableWidget { background: transparent; border: none; gridline-color: transparent; selection-background-color: @selection@; selection-color: @text@; }
QTableWidget::item { padding: 4px 8px; border-bottom: 1px solid @divider@; }
QTableWidget::item:selected { background: @selection@; }
QHeaderView { background: transparent; }
QHeaderView::section { background: transparent; color: @muted@; border: none; border-bottom: 1px solid @divider@; padding: 8px; font-size: 11px; font-weight: 700; }
QTableCornerButton::section { background: transparent; border: none; }
QListWidget { background: @panelLow@; }
QListWidget::item:selected { background: @selection@; color: @text@; }

QScrollArea { border: none; background: transparent; }
QScrollBar:vertical { background: transparent; width: 11px; margin: 2px; }
QScrollBar:horizontal { background: transparent; height: 11px; margin: 2px; }
QScrollBar::handle { background: @outline@; border-radius: 4px; }
QScrollBar::handle:vertical { min-height: 30px; }
QScrollBar::handle:horizontal { min-width: 30px; }
QScrollBar::handle:hover { background: @muted@; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

/* Page headings and dialogs */
#eyebrow { color: @accent@; font-size: 11px; font-weight: 700; background: transparent; }
#pageH1 { font-size: 26px; font-weight: 700; color: @text@; background: transparent; }
#pageDesc { color: @muted@; font-size: 13px; background: transparent; }
#kbd { background: @softBg@; color: @accent@; border-radius: 5px; padding: 1px 6px; font-size: 11px; font-weight: 700; }
QDialog, QMessageBox { background: @canvas@; }
#modalCard { background: transparent; }
#modalHead { background: @softBg@; border-top-left-radius: 12px; border-top-right-radius: 12px; }
#modalHead QLabel { background: transparent; }
#modalTitle { font-size: 21px; font-weight: 700; background: transparent; }
#modalFoot { background: @panelLow@; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; border-top: 1px solid @divider@; }
#tagBlue { background: @brand@; color: @white@; border-radius: 4px; padding: 1px 6px; font-size: 10px; font-weight: 700; }
#tagGrey { background: #354565; color: #d7e1f2; border-radius: 4px; padding: 1px 6px; font-size: 10px; font-weight: 700; }
#modeChip { background: @successBg@; color: @successText@; border-radius: 6px; padding: 2px 8px; font-size: 10px; font-weight: 700; }
#readyPill { border-radius: 12px; }
#readyText { color: @successText@; font-weight: 700; font-size: 12px; background: transparent; }
)");

    // Resolve named roles so text, fills, and borders can vary independently.
    const struct
    {
        const char *token;
        QColor light;
        QColor dark;
    } colors[] = {
        {"@canvas@", surface, darkSurface},
        {"@panel@", white, darkPanel},
        {"@panelLow@", surfaceLow, "#202b42"},
        {"@text@", onSurface, darkText},
        {"@muted@", onSurfaceVariant, darkMuted},
        {"@outline@", outline, "#667895"},
        {"@divider@", outlineVariant, darkDivider},
        {"@sidebarTop@", sidebar, "#131e37"},
        {"@sidebarBottom@", "#0d1b32", "#0c1326"},
        {"@navActive@", "#2754b5", "#264c9f"},
        {"@navTail@", "#424263", "#393953"},
        {"@dashboardWash@", "#e8edfb", "#152139"},
        {"@heroStart@", "#17254b", "#152047"},
        {"@heroMid@", "#2554bb", "#244696"},
        {"@heroViolet@", "#574974", "#46395d"},
        {"@heroWarm@", "#a95842", "#824437"},
        {"@hover@", surfaceContainer, "#28364e"},
        {"@pressed@", surfaceHigh, "#354662"},
        {"@navy@", sidebar, sidebar},
        {"@white@", white, white},
        {"@brand@", primary, primary},
        {"@brandHover@", "#2049bd", "#2049bd"},
        {"@brandPressed@", "#193c9f", "#193c9f"},
        {"@accent@", primary, "#a7bfff"},
        {"@focus@", primary, "#91b0ff"},
        {"@softBg@", primaryContainer, "#233458"},
        {"@softBorder@", "#c5d3ee", "#486295"},
        {"@selection@", "#dce7ff", "#304772"},
        {"@softPressed@", "#d1dfff", "#344a75"},
        {"@successBg@", "#e5f5ed", "#17372d"},
        {"@successBorder@", "#b3ddc7", "#406957"},
        {"@successText@", "#125437", "#a4e1bf"},
        {"@dangerBg@", "#fff0ef", "#42272e"},
        {"@dangerBorder@", "#bf716c", "#a96870"},
        {"@dangerText@", "#9a251e", "#ffb8b3"},
        {"@dangerHover@", "#ffe5e2", "#533039"},
        {"@dangerPressed@", "#ffd9d5", "#61363f"},
        {"@warningBg@", "#fff4d7", "#3a2e1b"},
        {"@warningBorder@", "#ead29a", "#775c33"},
        {"@warningText@", "#674600", "#f7d48a"},
    };

    const bool dark = isDark();
    for (const auto &color : colors)
        sheet.replace(QLatin1String(color.token), (dark ? color.dark : color.light).name());
    return sheet;
}
