#include "Theme.h"
#include <QPainter>
#include <QDir>
#include <algorithm>
#include <QFile>
#include <QHash>
#include <QPainterPath>
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
    else if (n == "tick")
    {
        p.drawPolyline(QPolygonF({{5.5, 12.5}, {10, 17}, {18.5, 7.5}}));
    }
    return QPixmap(pm);
}

namespace
{
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
}

QString Theme::styleSheet()
{
    // Design tokens (@name) are substituted below so every colour lives in one place.
    QString css = QStringLiteral(R"(
* { font-family: "Segoe UI Variable Text", "Segoe UI", "Inter", "Helvetica Neue", Arial, sans-serif; font-size: 13px; color: @text; }
QMainWindow, #root, #page { background: @bg; }
QToolTip { background: @navy; color: #ffffff; border: none; padding: 5px 9px; border-radius: 6px; }

/* ---- sidebar: flat navy ---- */
#sidebar { background: @navy; border: none; }
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
#nav[active="true"] { background: @primary; border-bottom: 2px solid #193c9e; }
#nav QLabel { background: transparent; font-weight: 600; font-size: 13.5px; color: #aab6d3; }
#nav[active="true"] QLabel { color: #ffffff; }
#nav[locked="true"] QLabel { color: #5d6a8f; }
#exitBtn { background: transparent; border: 1px solid rgba(255,255,255,12%); border-radius: 8px; color: #c4cee8; padding: 8px 12px; font-weight: 600; }
#exitBtn:hover { background: rgba(255,255,255,8%); }
#exitBtn:pressed { background: rgba(255,255,255,12%); }

/* ---- top bar ---- */
#topbar { background: @surface; border: none; border-bottom: 1px solid @border; }
#pageTitle { font-size: 20px; font-weight: 700; color: @text; }
#pageSub { color: @muted; font-size: 12px; }
#chip { background: @bg; border: 1px solid @border; border-radius: 10px; }
#clockTime { font-weight: 700; font-size: 14px; background: transparent; }
#clockDate { color: @muted; font-size: 11px; background: transparent; }

/* ---- panels ---- */
#subCard { background: @bg; border: 1px solid @border; border-radius: 10px; }
#field { background: @bg; border: 1px solid @border; border-radius: 8px; }
#notice { background: #fff1ef; border: 1px solid #ffc9c2; border-radius: 8px; }
#notice QLabel { color: #93000a; background: transparent; }
#totalBox { background: @navy; border-radius: 12px; }
#totalBox QLabel { color: #ffffff; background: transparent; }
#totalBox #caption { color: #9fb0d8; }
#toast { background: @navy; color: #ffffff; border-radius: 10px; padding: 10px 18px; font-weight: 600; }

/* ---- typography ---- */
#h1 { font-size: 15px; font-weight: 700; }
#h2 { font-size: 14px; font-weight: 600; }
#muted { color: @muted; }
#caption { color: @subtle; font-size: 10.5px; font-weight: 700; letter-spacing: 0.8px; }
#bigValue { font-size: 26px; font-weight: 700; }
#bigTotal { font-size: 32px; font-weight: 700; color: @primary; }
#mono { font-family: "Consolas", "Courier New", monospace; }
#link { color: @primary; font-weight: 600; background: transparent; border: none; text-align: right; padding: 2px 4px; }
#link:hover { text-decoration: underline; background: transparent; }
#link:pressed { padding: 2px 4px; border: none; }

/* ---- badges ---- */
QLabel[badge] { border-radius: 6px; padding: 2px 9px; font-size: 11px; font-weight: 700; }
QLabel[badge="green"] { background: #e3f6ea; color: #12663a; }
QLabel[badge="blue"]  { background: #e6eeff; color: #1d46b4; }
QLabel[badge="red"]   { background: #fde8e6; color: #9b1c1c; }
QLabel[badge="grey"]  { background: #eef1f6; color: #4b5873; }
QLabel[badge="amber"] { background: #fff3d6; color: #7a4b00; }

/* ---- buttons: flat, 1px border, no gradients ---- */
QPushButton { border-radius: 8px; padding: 7px 14px; font-weight: 600; color: @text;
  border: 1px solid @borderStrong; background: @surface; }
QPushButton:hover { background: @hover; border-color: #b9c3d6; }
QPushButton:pressed { background: #e8edf6; }
QPushButton:disabled { color: #a3adc2; border-color: @border; background: #f4f6fa; }
QPushButton[kind="primary"] { color: #ffffff; border: 1px solid @primary; border-bottom: 3px solid @primaryPress; padding: 7px 14px 5px 14px;
  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #3b6ae8, stop:1 @primary); }
QPushButton[kind="primary"]:hover { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #4a77ee, stop:1 @primaryHover); }
QPushButton[kind="primary"]:pressed { border-bottom: 1px solid @primaryPress; padding: 8px 14px 6px 14px; background: @primaryPress; }
QPushButton[kind="primary"]:disabled { color: #ffffff; border: 1px solid #a9bcef; border-bottom: 3px solid #93a9e6; border-radius: 8px; background: #a9bcef; }
QPushButton[kind="soft"] { color: @primary; border: 1px solid #d3e0ff; border-bottom: 2px solid #bccffa; padding: 7px 14px 6px 14px; background: #eef3ff; }
QPushButton[kind="soft"]:hover { background: #e2ebff; border-color: #c2d3fb; }
QPushButton[kind="soft"]:pressed { border-bottom: 1px solid #bccffa; padding: 8px 14px 6px 14px; background: #d6e3ff; }
QPushButton[kind="soft"]:disabled { color: #9fb1dd; border-color: #e2e9fa; border-bottom: 2px solid #e2e9fa; background: #f3f6fd; }
QPushButton[kind="danger"] { color: @danger; border: 1px solid #f4c7c3; background: #ffffff; }
QPushButton[kind="danger"]:hover { background: #fdecea; }
QPushButton[kind="danger"]:pressed { background: #fadbd7; }
QPushButton[kind="flat"] { border: 1px solid transparent; background: transparent; padding: 5px 8px; }
QPushButton[kind="flat"]:hover { background: #e9eef8; }
QPushButton[kind="flat"]:pressed { background: #dde5f4; }
QPushButton[kind="tab"] { color: @muted; padding: 6px 14px; border-radius: 16px; border: 1px solid @border; background: @surface; }
QPushButton[kind="tab"]:hover { background: @hover; }
QPushButton[kind="tab"]:checked { color: #ffffff; border: 1px solid @primary; border-bottom: 2px solid @primaryPress; border-radius: 16px; padding: 6px 14px 5px 14px; background: @primary; }
QPushButton[kind="mini"] { padding: 4px 11px; font-size: 12px; border-radius: 7px; color: @primary; border: 1px solid #d3e0ff; background: #f5f8ff; }
QPushButton[kind="mini"]:hover { background: #e8efff; }
QPushButton[kind="mini"]:pressed { background: #dbe6ff; }
QPushButton[kind="mini"]:disabled { color: #8b96ad; border-color: @border; background: #f4f6fa; }
QPushButton[kind="step"] { padding: 0; border-radius: 7px; min-width: 26px; max-width: 26px; min-height: 24px; max-height: 24px; font-size: 15px; font-weight: 700; color: @primary;
  border: 1px solid #d3e0ff; background: #f5f8ff; }
QPushButton[kind="step"]:hover { background: #e8efff; }
QPushButton[kind="icon"] { padding: 0; min-width: 36px; max-width: 36px; min-height: 34px; max-height: 34px; }

/* ---- inputs ---- */
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { background: @surface; border: 1px solid @borderStrong; border-radius: 8px; padding: 7px 11px; selection-background-color: #bcd0fb; selection-color: @text; }
QLineEdit:hover, QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover { border-color: #b3bfd3; }
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
QComboBox QAbstractItemView { background: @surface; border: 1px solid @border; selection-background-color: #e6eeff; selection-color: @text; outline: none; }
QPlainTextEdit, QTextEdit { background: @surface; border: 1px solid @border; border-radius: 8px; selection-background-color: #bcd0fb; }

/* ---- tables ---- */
QTableWidget { background: transparent; border: none; gridline-color: transparent; selection-background-color: #eef3ff; selection-color: @text; outline: none; alternate-background-color: transparent; }
QTableWidget::item { padding: 4px 8px; border-bottom: 1px solid #edf0f6; }
QTableWidget::item:hover { background: #f7f9fd; }
QTableWidget::item:selected { background: #eef3ff; color: @text; }
QHeaderView { background: transparent; }
QHeaderView::section { background: transparent; color: @subtle; border: none; border-bottom: 1px solid @border; padding: 8px 8px; font-size: 10.5px; font-weight: 700; }
QTableCornerButton::section { background: transparent; border: none; }
QListWidget { background: #fafbfe; border: 1px solid @border; border-radius: 8px; padding: 4px; outline: none; }
QListWidget::item { padding: 9px 8px; border-bottom: 1px solid #edf0f6; font-family: "Consolas", "Courier New", monospace; font-size: 12px; }
QListWidget::item:selected { background: #eef3ff; color: @text; }

/* ---- scrollbars ---- */
QScrollArea { border: none; background: transparent; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: #ccd4e3; border-radius: 3px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: #aab5cb; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle:horizontal { background: #ccd4e3; border-radius: 3px; min-width: 30px; }
QScrollBar::handle:horizontal:hover { background: #aab5cb; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QCheckBox { spacing: 9px; background: transparent; }
QCheckBox::indicator { width: 18px; height: 18px; border-radius: 5px; border: 1px solid #b3bfd3; background: @surface; }
QCheckBox::indicator:hover { border-color: @primary; }
QCheckBox::indicator:checked { background: @primary; border-color: @primary; image: url(@tick); }
QDialog, QMessageBox { background: @bg; }
QMessageBox QPushButton { min-width: 76px; }
)");
    const QHash<QString, QString> tokens = {
        {"@bg", "#f4f6fb"},          {"@surface", "#ffffff"},      {"@hover", "#f1f4fa"},
        {"@border", "#e2e7f0"},      {"@borderStrong", "#cbd3e1"}, {"@text", "#0f1b33"},
        {"@muted", "#5b6781"},       {"@subtle", "#7b869c"},       {"@navy", "#0f1b33"},
        {"@primary", "#2554d8"},     {"@primaryHover", "#1e46b8"}, {"@primaryPress", "#193c9e"},
        {"@danger", "#ba1a1a"},
        {"@chevUp", glyph("chevron-up", QColor("#5b6781"), 20)},
        {"@chevDown", glyph("chevron-down", QColor("#5b6781"), 20)},
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
