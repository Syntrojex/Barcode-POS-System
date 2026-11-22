#include "Theme.h"
#include <QPainter>
#include <cmath>
#include <QRegularExpression>
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

static QString lightSheet()
{
    return QStringLiteral(R"(
* { font-family: "Segoe UI Variable Text", "Segoe UI", "Inter", "Helvetica Neue", Arial, sans-serif; font-size: 13px; color: #0f1b33; }
QMainWindow, #root { background: #eaf0fc; }
#page { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #f4f7fe, stop:1 #e8eefb); }
QToolTip { background: #16254a; color: #eaf1ff; border: none; padding: 5px 9px; border-radius: 6px; }

/* ---- sidebar (dark, glossy) ---- */
#sidebar { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #0f1b33, stop:1 #0b1426); border: none; }
#logoBox { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #7fb0ff, stop:1 #2b5fe8); border-radius: 12px; border-bottom: 3px solid #1a3fae; }
#logoBox QLabel { color: white; font-weight: 800; font-size: 18px; background: transparent; }
#brand { color: #ffffff; font-size: 18px; font-weight: 800; }
#brandSub { color: #7f8db3; font-size: 11px; }
#userCard { background: rgba(255,255,255,7%); border: 1px solid rgba(255,255,255,10%); border-radius: 14px; }
#avatar { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #7fb0ff, stop:1 #2b5fe8); border-radius: 17px; color: white; font-weight: 700; font-size: 12px; }
#userName { color: #ffffff; font-weight: 700; font-size: 13.5px; background: transparent; }
#userRole { color: #8d9abd; font-size: 11.5px; background: transparent; }
#nav { background: transparent; border: none; border-radius: 12px; text-align: left; }
#nav:hover { background: rgba(255,255,255,8%); }
#nav[active="true"] { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #2e62f2, stop:1 #2554d8); border-bottom: 3px solid #1b3e9f; border-radius: 10px; }
#nav QLabel { background: transparent; font-weight: 600; font-size: 14px; color: #94a3b8; }
#nav[active="true"] QLabel { color: #ffffff; }
#nav[locked="true"] QLabel { color: #5d6a8f; }
#exitBtn { background: rgba(255,255,255,7%); border: 1px solid rgba(255,255,255,10%); border-bottom: 3px solid rgba(0,0,0,25%); border-radius: 11px; color: #c9d3ee; padding: 8px 12px; }
#exitBtn:hover { background: rgba(255,255,255,13%); }
#exitBtn:pressed { border-bottom: 1px solid rgba(0,0,0,25%); padding-top: 10px; padding-bottom: 6px; }

/* ---- top bar ---- */
#topbar { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #ffffff, stop:1 #f3f6fe); border-bottom: 1px solid #dbe4f7; }
#pageTitle { font-size: 21px; font-weight: 800; color: #0f1b33; }
#pageSub { color: #64708a; font-size: 12px; }
#chip { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #ffffff, stop:1 #eef3ff); border: 1px solid #d9e3f8; border-bottom: 2px solid #cbd8f2; border-radius: 12px; }
#clockTime { font-weight: 800; font-size: 14px; }
#clockDate { color: #64708a; font-size: 11px; }
#topAvatar { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #7fb0ff, stop:1 #2b5fe8); border-radius: 18px; }

/* ---- panels ---- */
#subCard { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #f6f9ff, stop:1 #edf2fd); border: 1px solid #dfe7f8; border-radius: 12px; }
#field { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #edf2fd, stop:1 #f6f9ff); border: 1px solid #dbe4f7; border-top: 2px solid #cfdaf3; border-radius: 11px; }
#notice { background: #fff1ef; border: 1px solid #ffc9c2; border-radius: 10px; }
#notice QLabel { color: #93000a; background: transparent; }
#totalBox { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #2a4cb0, stop:1 #172a66); border-radius: 14px; border-bottom: 4px solid #0e1a45; }
#totalBox QLabel { color: #ffffff; background: transparent; }
#totalBox #caption { color: #a9bdf5; }
#toast { background: #16254a; color: #eaf1ff; border-radius: 12px; border-bottom: 3px solid #0b1430; padding: 11px 18px; font-weight: 600; }

/* ---- typography ---- */
#h1 { font-size: 15px; font-weight: 800; }
#h2 { font-size: 14px; font-weight: 700; }
#muted { color: #64708a; }
#caption { color: #7a86a0; font-size: 10.5px; font-weight: 800; letter-spacing: 1px; }
#bigValue { font-size: 28px; font-weight: 800; }
#bigTotal { font-size: 32px; font-weight: 800; color: #004ac6; }
#mono { font-family: "Consolas", "Courier New", monospace; }
#link { color: #2554d8; font-weight: 700; background: transparent; border: none; text-align: right; padding: 2px 4px; }
#link:hover { text-decoration: underline; background: transparent; }
#link:pressed { padding: 2px 4px; border: none; }

/* ---- badges ---- */
QLabel[badge] { border-radius: 7px; padding: 2px 9px; font-size: 11px; font-weight: 800; }
QLabel[badge="green"] { background: #d7f8e2; color: #0b5d2b; border: 1px solid #aeeac3; }
QLabel[badge="blue"] { background: #e0eaff; color: #1d46b4; border: 1px solid #c3d5ff; }
QLabel[badge="red"] { background: #ffe1de; color: #93000a; border: 1px solid #ffc3bd; }
QLabel[badge="grey"] { background: #eaeef8; color: #55617c; border: 1px solid #d9e0ef; }
QLabel[badge="amber"] { background: #fff0c9; color: #6a4a00; border: 1px solid #ffe08f; }

/* ---- buttons: raised, with a thick bottom edge that squashes on press ---- */
QPushButton { border-radius: 10px; padding: 7px 16px 7px 16px; font-weight: 700; color: #1b2a4d;
  border: 1px solid #d3ddf3; border-bottom: 3px solid #bcc9e8;
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #ffffff, stop:1 #eef3fe); }
QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #ffffff, stop:1 #e4ecfd); }
QPushButton:pressed { border-bottom: 1px solid #bcc9e8; padding: 9px 16px 7px 16px; background: #e3eafb; }
QPushButton:disabled { color: #a3adc5; border-bottom: 3px solid #dde4f3; background: #f3f6fc; }
QPushButton[kind="primary"] { color: #ffffff; border: 1px solid #2e63e8; border-bottom: 3px solid #17399f;
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #67a0ff, stop:0.5 #3f7bf6, stop:1 #2d63e4); }
QPushButton[kind="primary"]:hover { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #7cafff, stop:0.5 #4f88fa, stop:1 #3a6feb); }
QPushButton[kind="primary"]:pressed { border-bottom: 1px solid #17399f; padding: 9px 16px 7px 16px; background: #2b5fdc; }
QPushButton[kind="primary"]:disabled { color: #ffffff; border: 1px solid #b7c9f5; border-bottom: 3px solid #a3b8ee; background: #bfd0f8; }
QPushButton[kind="soft"] { color: #1d46b4; border: 1px solid #c9d8fb; border-bottom: 3px solid #aebfee;
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #eaf1ff, stop:1 #d9e6ff); }
QPushButton[kind="soft"]:hover { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #f2f6ff, stop:1 #cfdfff); }
QPushButton[kind="soft"]:pressed { border-bottom: 1px solid #aebfee; padding: 9px 16px 7px 16px; }
QPushButton[kind="danger"] { color: #b3261e; border: 1px solid #ffc3bd; border-bottom: 3px solid #f0a8a0; background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #ffffff, stop:1 #ffefed); }
QPushButton[kind="danger"]:hover { background: #ffe3e0; }
QPushButton[kind="danger"]:pressed { border-bottom: 1px solid #f0a8a0; padding: 9px 16px 7px 16px; }
QPushButton[kind="flat"] { border: 1px solid transparent; background: transparent; padding: 5px 8px; }
QPushButton[kind="flat"]:hover { background: #e4ecfd; }
QPushButton[kind="flat"]:pressed { border: 1px solid transparent; padding: 5px 8px; background: #d5e1fb; }
QPushButton[kind="tab"] { color: #55617c; padding: 6px 15px; border-radius: 11px; border: 1px solid #d9e3f8; border-bottom: 3px solid #c6d3ee; }
QPushButton[kind="tab"]:pressed { border-bottom: 1px solid #c6d3ee; padding: 8px 15px 6px 15px; }
QPushButton[kind="tab"]:checked { color: #ffffff; border: 1px solid #2e63e8; border-bottom: 3px solid #17399f;
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #67a0ff, stop:1 #2d63e4); }
QPushButton[kind="mini"] { padding: 4px 11px; font-size: 12px; border-radius: 8px; color: #1d46b4; border-bottom: 2px solid #b9c9ee; }
QPushButton[kind="mini"]:pressed { border-bottom: 1px solid #b9c9ee; padding: 5px 11px 4px 11px; }
QPushButton[kind="mini"]:disabled { color: #8b96ad; border-bottom: 2px solid #dde4f3; }
QPushButton[kind="step"] { padding: 0; border-radius: 8px; min-width: 26px; max-width: 26px; min-height: 24px; max-height: 24px; font-size: 15px; font-weight: 800; color: #1d46b4;
  border-bottom: 2px solid #b9c9ee; }
QPushButton[kind="step"]:pressed { border-bottom: 1px solid #b9c9ee; padding: 1px 0 0 0; }
QPushButton[kind="icon"] { padding: 0; min-width: 36px; max-width: 36px; min-height: 34px; max-height: 34px; }

/* ---- inputs (slightly inset) ---- */
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { background: #ffffff; border: 1px solid #d3ddf3; border-top: 2px solid #c3d0ee; border-radius: 10px; padding: 7px 11px; selection-background-color: #b9cffd; selection-color: #0f1b33; }
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus { border: 1px solid #3f7bf6; border-top: 2px solid #2d63e4; }
QLineEdit[flat="true"] { background: transparent; border: none; padding: 0; font-weight: 700; font-size: 14px; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox QAbstractItemView { background: white; border: 1px solid #d3ddf3; selection-background-color: #e0eaff; selection-color: #0f1b33; }

/* ---- tables ---- */
QTableWidget { background: transparent; border: none; gridline-color: transparent; selection-background-color: #e6eefe; selection-color: #0f1b33; outline: none; }
QTableWidget::item { padding: 4px 8px; border-bottom: 1px solid #e8eefb; }
QTableWidget::item:selected { background: #e6eefe; }
QHeaderView { background: transparent; }
QHeaderView::section { background: transparent; color: #7a86a0; border: none; border-bottom: 1px solid #dbe4f7; padding: 8px 8px; font-size: 10.5px; font-weight: 800; }
QTableCornerButton::section { background: transparent; border: none; }
QListWidget { background: #f6f9ff; outline: none; }
QListWidget::item:selected { background: #e0eaff; color: #0f1b33; }

/* ---- scrollbars ---- */
QScrollArea { border: none; background: transparent; }
QScrollBar:vertical { background: transparent; width: 11px; margin: 2px; }
QScrollBar::handle:vertical { background: #c3cfea; border-radius: 4px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: #a3b3d8; }
QScrollBar:horizontal { background: transparent; height: 11px; margin: 2px; }
QScrollBar::handle:horizontal { background: #c3cfea; border-radius: 4px; min-width: 30px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QCheckBox { spacing: 9px; }
QCheckBox::indicator { width: 18px; height: 18px; border-radius: 5px; border: 1.5px solid #bcc9e8; background: white; }
QCheckBox::indicator:checked { background: #3f7bf6; border-color: #2d63e4; }
QDialog, QMessageBox { background: #f1f5fd; }

/* ---- Increment 3 (Stitch redesign) ---- */
#eyebrow { color: #2554d8; font-size: 10.5px; font-weight: 800; letter-spacing: 1px; background: transparent; }
#pageH1 { font-size: 26px; font-weight: 800; color: #0f1b33; background: transparent; }
#pageDesc { color: #64748b; font-size: 13px; background: transparent; }
#kbd { background: #e5eeff; color: #1d46b4; border-radius: 5px; padding: 1px 6px; font-size: 10.5px; font-weight: 800; }
#modalCard { background: transparent; }
#modalHead { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #eaf1ff, stop:1 #e0eaff); border-top-left-radius: 14px; border-top-right-radius: 14px; }
#modalHead QLabel { background: transparent; }
#modalTitle { font-size: 21px; font-weight: 800; background: transparent; }
#modalFoot { background: #f4f7fe; border-bottom-left-radius: 14px; border-bottom-right-radius: 14px; border-top: 1px solid #dbe4f7; }
#tagBlue { background: #2554d8; color: #ffffff; border-radius: 4px; padding: 1px 6px; font-size: 9.5px; font-weight: 800; letter-spacing: 0.5px; }
#tagGrey { background: rgba(255,255,255,18%); color: #cbd5e1; border-radius: 4px; padding: 1px 6px; font-size: 9.5px; font-weight: 800; letter-spacing: 0.5px; }
#modeChip { background: #047857; color: #d1fae5; border-radius: 6px; padding: 2px 8px; font-size: 10px; font-weight: 800; letter-spacing: 0.5px; }
#readyPill { background: #d1fae5; border: 1px solid #a7f3d0; border-radius: 14px; }
#readyText { color: #065f46; font-weight: 700; font-size: 12px; background: transparent; }
)");
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
    QString s = lightSheet();
    if (!isDark())
        return s;
    // Dark mode: swap the light palette for dark equivalents (order matters: specific first).
    const char *map[][2] = {
        {"#0f1b33", "#e6ecfb"}, {"#1b2a4d", "#dbe4fa"}, {"#f4f7fe", "#141b2e"}, {"#e8eefb", "#0f1524"},
        {"#eaf0fc", "#0f1524"}, {"#f3f6fe", "#18213a"}, {"#eef3ff", "#1d2742"}, {"#f6f9ff", "#1a2340"},
        {"#edf2fd", "#161e36"}, {"#f1f5fd", "#141b2e"}, {"#eef3fe", "#1d2742"}, {"#e4ecfd", "#26324f"},
        {"#e3eafb", "#26324f"}, {"#f3f6fc", "#1a2238"}, {"#eaf1ff", "#22304f"}, {"#d9e6ff", "#1b2845"},
        {"#e6eefe", "#26324f"}, {"#e0eaff", "#22304f"}, {"#e5eeff", "#22304f"}, {"#ffffff", "#1c2540"},
        {"white", "#1c2540"}, {"#dbe4f7", "#2a3556"}, {"#d3ddf3", "#2f3b5e"}, {"#d9e3f8", "#2f3b5e"},
        {"#dfe7f8", "#2a3556"}, {"#cbd8f2", "#26324f"}, {"#bcc9e8", "#232d4a"}, {"#c3d0ee", "#2f3b5e"},
        {"#c9d8fb", "#2f3b5e"}, {"#aebfee", "#232d4a"}, {"#b9c9ee", "#232d4a"}, {"#c6d3ee", "#232d4a"},
        {"#e8eefb", "#243050"}, {"#64708a", "#9aa7c7"}, {"#7a86a0", "#8795b8"}, {"#55617c", "#a9b6d6"},
        {"#1d46b4", "#8fb2ff"}, {"#004ac6", "#8fb2ff"}, {"#2554d8", "#8fb2ff"}, {"#dce9ff", "#2a3556"},
        {"#c3cfea", "#3a4770"}, {"#a3b3d8", "#4a5a8a"}, {"#a3adc5", "#5d6a8f"}, {"#8b96ad", "#6a7799"}};
    s.replace(QRegularExpression("color:\\s*(#ffffff|white)"), "color: @KEEPWHITE@");
    for (const auto &m : map)
        s.replace(QString::fromLatin1(m[0]), QString::fromLatin1(m[1]));
    s.replace("@KEEPWHITE@", "#ffffff");
    return s;
}
