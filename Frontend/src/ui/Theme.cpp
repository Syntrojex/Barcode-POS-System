#include "Theme.h"
#include <QPainter>
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
    return QPixmap(pm);
}

QString Theme::styleSheet()
{
    return QStringLiteral(R"(
* { font-family: "Inter", "Segoe UI", "Helvetica Neue", Arial, sans-serif; font-size: 13px; color: #0b1c30; }
QMainWindow, #root, #page { background: #f8f9ff; }
QToolTip { background: #213145; color: #eaf1ff; border: none; padding: 4px 8px; }

/* ---- cards ---- */
#card { background: #ffffff; border: 1px solid #dce9ff; border-radius: 10px; }
#cardAlert { background: #ffffff; border: 1px solid #ffb4ab; border-radius: 10px; }
#strip { background: #ffffff; border: 1px solid #dce9ff; border-radius: 10px; }
#subCard { background: #eff4ff; border: 1px solid #dce9ff; border-radius: 8px; }
#totals { background: #ffffff; border: 1px solid #dce9ff; border-radius: 10px; }
#totalBox { background: #eff4ff; border-radius: 8px; }

/* ---- sidebar ---- */
#sidebar { background: #eff4ff; border-right: 1px solid #dce9ff; }
#logoBox { background: #2563eb; border-radius: 8px; }
#logoBox QLabel { color: white; font-weight: 800; font-size: 16px; }
#brand { color: #004ac6; font-size: 15px; font-weight: 700; }
#userCard { background: #ffffff; border: 1px solid #dce9ff; border-radius: 10px; }
#avatar { background: #2563eb; border-radius: 16px; color: white; font-weight: 700; font-size: 12px; }
#segment { background: #e5eeff; border-radius: 6px; }
#segment QPushButton { background: transparent; border: none; border-radius: 5px; padding: 4px 6px; font-weight: 600; color: #434655; }
#segment QPushButton:checked { background: #ffffff; color: #004ac6; }
#nav { background: transparent; border: none; border-radius: 8px; text-align: left; }
#nav:hover { background: #dce9ff; }
#nav[active="true"] { background: #2563eb; }
#nav QLabel { background: transparent; font-weight: 600; font-size: 13.5px; color: #0b1c30; }
#nav[active="true"] QLabel { color: white; }
#nav #fkey { font-weight: 500; font-size: 11px; color: #737686; }
#nav[active="true"] #fkey { color: #dbe1ff; }
#nav[locked="true"] QLabel { color: #a5a9bb; }
#navFooter { color: #434655; font-size: 11px; }

/* ---- top bar ---- */
#topbar { background: #ffffff; border-bottom: 1px solid #dce9ff; }
#topTitle { font-size: 15px; font-weight: 600; }
#topHint { color: #434655; font-size: 12px; }
#chip { background: #eff4ff; border-radius: 8px; }
#clockTime { font-weight: 700; font-size: 14px; }
#clockDate { color: #434655; font-size: 11px; }
#topAvatar { background: #2563eb; border-radius: 17px; }

/* ---- typography helpers ---- */
#h1 { font-size: 15px; font-weight: 700; }
#h2 { font-size: 14px; font-weight: 600; }
#muted { color: #434655; }
#caption { color: #737686; font-size: 11px; font-weight: 700; letter-spacing: 1px; }
#bigValue { font-size: 26px; font-weight: 700; }
#bigTotal { font-size: 28px; font-weight: 800; color: #004ac6; }
#mono { font-family: "Consolas", "Courier New", monospace; }
#link { color: #004ac6; font-weight: 600; background: transparent; border: none; text-align: right; }
#link:hover { text-decoration: underline; }

/* ---- badges ---- */
QLabel[badge] { border-radius: 4px; padding: 2px 7px; font-size: 11px; font-weight: 700; }
QLabel[badge="green"] { background: #7ffc97; color: #002109; }
QLabel[badge="blue"] { background: #dce9ff; color: #003ea8; }
QLabel[badge="red"] { background: #ffdad6; color: #93000a; }
QLabel[badge="grey"] { background: #e5eeff; color: #434655; }
QLabel[badge="amber"] { background: #ffe9b0; color: #5a4300; }

/* ---- buttons ---- */
QPushButton { border-radius: 7px; padding: 7px 14px; font-weight: 600; border: 1px solid #c3c6d7; background: #ffffff; color: #0b1c30; }
QPushButton:hover { background: #eff4ff; }
QPushButton:pressed { background: #dce9ff; }
QPushButton:disabled { color: #a5a9bb; background: #f3f6fd; }
QPushButton[kind="primary"] { background: #2563eb; border: 1px solid #2563eb; color: white; }
QPushButton[kind="primary"]:hover { background: #004ac6; }
QPushButton[kind="primary"]:pressed { background: #003ea8; }
QPushButton[kind="primary"]:disabled { background: #b4c5ff; border-color: #b4c5ff; color: white; }
QPushButton[kind="soft"] { background: #dae2fd; border: 1px solid #dae2fd; color: #131b2e; }
QPushButton[kind="soft"]:hover { background: #c9d4fa; }
QPushButton[kind="danger"] { background: #ffffff; border: 1px solid #ffb4ab; color: #ba1a1a; }
QPushButton[kind="danger"]:hover { background: #ffdad6; }
QPushButton[kind="flat"] { border: 1px solid transparent; background: transparent; padding: 4px 8px; }
QPushButton[kind="flat"]:hover { background: #dce9ff; }
QPushButton[kind="tab"] { background: #ffffff; border: 1px solid #dce9ff; color: #434655; padding: 5px 12px; }
QPushButton[kind="tab"]:checked { background: #dae2fd; border-color: #2563eb; color: #003ea8; }
QPushButton[kind="mini"] { padding: 3px 9px; font-size: 12px; border-radius: 5px; color: #004ac6; border: 1px solid #b4c5ff; background: #ffffff; }
QPushButton[kind="mini"]:hover { background: #eff4ff; }
QPushButton[kind="mini"]:disabled { color: #737686; border-color: #dce9ff; }
QPushButton[kind="step"] { padding: 0; border-radius: 5px; min-width: 24px; max-width: 24px; min-height: 24px; max-height: 24px; font-weight: 700; background: #eff4ff; border: 1px solid #dce9ff; }
QPushButton[kind="step"]:hover { background: #dce9ff; }
QPushButton[kind="icon"] { padding: 0; min-width: 32px; max-width: 32px; min-height: 32px; max-height: 32px; border-radius: 7px; }

/* ---- inputs ---- */
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { background: #ffffff; border: 1px solid #c3c6d7; border-radius: 7px; padding: 6px 9px; selection-background-color: #b4c5ff; selection-color: #0b1c30; }
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus { border: 1.5px solid #2563eb; }
QLineEdit[flat="true"] { background: transparent; border: none; padding: 0; font-weight: 600; font-size: 14px; }
#field { background: #eff4ff; border: 1px solid #dce9ff; border-radius: 8px; }
#field:focus-within { border-color: #2563eb; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox QAbstractItemView { background: white; border: 1px solid #c3c6d7; selection-background-color: #dce9ff; selection-color: #0b1c30; }

/* ---- tables ---- */
QTableWidget { background: #ffffff; border: none; gridline-color: transparent; alternate-background-color: #f8f9ff; selection-background-color: #e5eeff; selection-color: #0b1c30; outline: none; }
QTableWidget::item { padding: 4px 8px; border-bottom: 1px solid #eff4ff; }
QTableWidget::item:selected { background: #e5eeff; }
QHeaderView { background: #ffffff; }
QHeaderView::section { background: #ffffff; color: #434655; border: none; border-bottom: 1px solid #dce9ff; padding: 8px 8px; font-size: 11px; font-weight: 700; }
QTableCornerButton::section { background: #ffffff; border: none; }

/* ---- scrollbars ---- */
QScrollArea { border: none; background: transparent; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: #c3c6d7; border-radius: 4px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: #a5a9bb; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle:horizontal { background: #c3c6d7; border-radius: 4px; min-width: 30px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }

/* ---- misc ---- */
#shortcutBar { background: #ffffff; border: 1px solid #dce9ff; border-radius: 8px; }
#keycap { background: #eff4ff; border: 1px solid #dce9ff; border-radius: 4px; color: #004ac6; font-weight: 700; font-size: 11px; padding: 1px 5px; }
#notice { background: #ffdad6; border: 1px solid #ffb4ab; border-radius: 6px; }
#notice QLabel { color: #93000a; background: transparent; }
#toast { background: #213145; color: #eaf1ff; border-radius: 8px; padding: 10px 16px; font-weight: 600; }
QCheckBox { spacing: 8px; }
QCheckBox::indicator { width: 18px; height: 18px; border-radius: 4px; border: 1.5px solid #c3c6d7; background: white; }
QCheckBox::indicator:checked { background: #2563eb; border-color: #2563eb; }
QDialog { background: #f8f9ff; }
QMessageBox { background: #f8f9ff; }
)");
}
