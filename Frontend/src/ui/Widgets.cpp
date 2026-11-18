#include "Widgets.h"
#include "Theme.h"
#ifdef WITH_CAMERA
#include "cameraScanner.h"
#include <opencv2/imgproc.hpp>
#endif
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QtMath>
#include <QResizeEvent>

namespace W
{
    QFrame *card(QWidget *parent, const char *objectName)
    {
        const QString n = objectName;
        if (n == "card" || n == "strip" || n == "totals")
            return new Card(parent, false);
        if (n == "cardAlert")
            return new Card(parent, true);
        auto *f = new QFrame(parent);
        f->setObjectName(objectName);
        return f;
    }

    QVBoxLayout *pageLayout(QWidget *page)
    {
        page->setObjectName("page");
        auto *l = new QVBoxLayout(page);
        l->setContentsMargins(14, 10, 14, 6);
        l->setSpacing(0);
        return l;
    }

    QLabel *iconTile(const QString &icon, const QColor &fg, const QString &top, const QString &bottom, int size)
    {
        auto *l = new QLabel;
        l->setFixedSize(size, size);
        l->setAlignment(Qt::AlignCenter);
        l->setStyleSheet(QString("background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %1, stop:1 %2);"
                                 "border-radius:%3px; border-bottom:3px solid rgba(0,0,0,18%);")
                             .arg(top, bottom)
                             .arg(size / 3));
        l->setPixmap(Theme::iconPixmap(icon, fg, size / 2));
        return l;
    }

    QLabel *label(const QString &text, const char *objectName, QWidget *parent)
    {
        auto *l = new QLabel(text, parent);
        if (objectName)
            l->setObjectName(objectName);
        return l;
    }

    void setBadge(QLabel *l, const QString &text, const QString &kind)
    {
        l->setText(text);
        l->setProperty("badge", kind);
        Theme::repolish(l);
    }

    QLabel *badge(const QString &text, const QString &kind, QWidget *parent)
    {
        auto *l = new QLabel(parent);
        l->setAlignment(Qt::AlignCenter);
        setBadge(l, text, kind);
        return l;
    }

    QPushButton *button(const QString &text, const QString &kind, const QString &icon, QWidget *parent)
    {
        auto *b = new QPushButton(text, parent);
        if (!kind.isEmpty())
            b->setProperty("kind", kind);
        if (!icon.isEmpty())
        {
            const bool light = kind == "primary";
            const bool danger = kind == "danger";
            const QColor c = light ? QColor("white") : danger ? Theme::error
                                                              : Theme::primary;
            b->setIcon(Theme::icon(icon, c, 16));
            b->setIconSize({16, 16});
        }
        b->setCursor(Qt::PointingHandCursor);
        return b;
    }

    QLabel *iconLabel(const QString &icon, const QColor &color, int px, QWidget *parent)
    {
        auto *l = new QLabel(parent);
        l->setPixmap(Theme::iconPixmap(icon, color, px));
        l->setFixedSize(px, px);
        l->setAttribute(Qt::WA_TranslucentBackground);
        return l;
    }

    QLabel *dot(const QColor &color, QWidget *parent)
    {
        auto *l = new QLabel(parent);
        l->setFixedSize(8, 8);
        l->setStyleSheet(QString("background:%1; border-radius:4px;").arg(color.name()));
        return l;
    }

    QFrame *hline(QWidget *parent)
    {
        auto *f = new QFrame(parent);
        f->setFixedHeight(1);
        f->setStyleSheet("background:#dce9ff; border:none;");
        return f;
    }

    QString money(double v, bool sign)
    {
        const QString s = QLocale(QLocale::English).toString(qAbs(v), 'f', 2);
        if (v < 0)
            return "-PKR " + s;
        return (sign ? "+PKR " : "PKR ") + s;
    }

    QString statusBadgeKind(const QString &status)
    {
        if (status == "Paid")
            return "green";
        if (status == "In Progress")
            return "blue";
        if (status == "On Hold")
            return "amber";
        return "grey";
    }

    QHBoxLayout *headerRow(QVBoxLayout *into, const QString &icon, const QString &title, QWidget *right)
    {
        auto *h = new QHBoxLayout;
        h->setSpacing(8);
        if (!icon.isEmpty())
            h->addWidget(iconLabel(icon, Theme::primary, 18));
        h->addWidget(label(title, "h1"));
        h->addStretch();
        if (right)
            h->addWidget(right, 0, Qt::AlignVCenter);
        into->addLayout(h);
        return h;
    }
}

// ------------------------------------------------------------------ Card
Card::Card(QWidget *parent, bool alert) : QFrame(parent), m_alert(alert)
{
    setContentsMargins(8, 4, 8, 10);
    setAttribute(Qt::WA_StyledBackground, false);
}

void Card::resizeEvent(QResizeEvent *e)
{
    QFrame::resizeEvent(e);
    const qreal dpr = devicePixelRatioF();
    m_shadow = QPixmap(QSize(width(), height()) * dpr);
    m_shadow.setDevicePixelRatio(dpr);
    m_shadow.fill(Qt::transparent);
    QPainter p(&m_shadow);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    const QRectF body = QRectF(rect()).adjusted(8, 4, -8, -10);
    for (int i = 9; i >= 0; --i)
    {
        p.setBrush(QColor(25, 50, 120, 5 + (9 - i) / 3));
        p.drawRoundedRect(body.translated(0, 4).adjusted(-i, -i + 1, i, i + 1), 14 + i, 14 + i);
    }
}

void Card::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.drawPixmap(0, 0, m_shadow);
    const QRectF body = QRectF(rect()).adjusted(8, 4, -8, -10);
    QLinearGradient g(body.topLeft(), body.bottomLeft());
    g.setColorAt(0, m_alert ? QColor("#ffffff") : QColor("#ffffff"));
    g.setColorAt(1, m_alert ? QColor("#fff2f0") : QColor("#f4f7ff"));
    p.setBrush(g);
    p.setPen(QPen(m_alert ? QColor(255, 140, 130, 190) : QColor(40, 70, 150, 34), 1));
    p.drawRoundedRect(body.adjusted(.5, .5, -.5, -.5), 14, 14);
    // lit top edge for the "raised" look
    p.setPen(QPen(QColor(255, 255, 255, 240), 1.2));
    p.drawLine(QPointF(body.left() + 14, body.top() + 1.6), QPointF(body.right() - 14, body.top() + 1.6));
}

// ------------------------------------------------------------------ CameraFeedWidget
CameraFeedWidget::CameraFeedWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(150);
#ifdef WITH_CAMERA
    connect(&m_timer, &QTimer::timeout, this, [this] {
        if (!m_camera) return;
        cv::Mat bgr;
        if (!m_camera->readFrame(bgr) || bgr.empty()) {
            const QString why = QString::fromStdString(m_camera->errMessage());
            setRunning(false);
            emit scanError(why.isEmpty() ? "Camera stopped sending frames" : why);
            return;
        }
        std::string code;
        cv::Rect box;
        if (m_camera->frameRead(code, box, bgr) && !code.empty()) {
            if (box.area() > 0) cv::rectangle(bgr, box, cv::Scalar(0, 255, 0), 2);
            const QString value = QString::fromStdString(code);
            if (value != m_lastDetected || !m_lastEmit.isValid() || m_lastEmit.elapsed() > 3000) {
                m_lastDetected = value;
                m_lastEmit.restart();
                emit barcodeDetected(value);
            }
        }
        cv::Mat rgb;
        cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
        m_frame = QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
        update();
    });
#endif
}
CameraFeedWidget::~CameraFeedWidget() = default;

void CameraFeedWidget::setRunning(bool on)
{
    if (!on) {
        m_timer.stop();
#ifdef WITH_CAMERA
        m_camera.reset();
#endif
        m_running = false;
        m_frame = QImage();
        update();
        return;
    }
#ifdef WITH_CAMERA
    if (m_running) return;
    m_camera = std::make_unique<cameraScanner>();
    if (!m_camera->openCamera()) {
        const QString why = QString::fromStdString(m_camera->errMessage());
        m_camera.reset();
        emit scanError(why);
        return;
    }
    m_lastDetected.clear();
    m_lastEmit.invalidate();
    m_running = true;
    m_timer.start(40);
#else
    emit scanError("Camera support is disabled. Configure with -DCAMERA=ON and install OpenCV 4.");
#endif
    update();
}

void CameraFeedWidget::setCode(const QString &code)
{
    m_code = code;
    update();
}

void CameraFeedWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRect r = rect().adjusted(1, 1, -1, -1);
    p.fillRect(r, QColor("#162644"));
    if (!m_frame.isNull()) {
        const QImage scaled = m_frame.scaled(r.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        p.drawImage(QPoint((width() - scaled.width()) / 2, (height() - scaled.height()) / 2), scaled);
    } else {
        p.setPen(QColor("#c2d0ef"));
#ifdef WITH_CAMERA
        p.drawText(r, Qt::AlignCenter, "Camera stopped — press Start camera");
#else
        p.drawText(r.adjusted(15, 0, -15, 0), Qt::AlignCenter | Qt::TextWordWrap,
                   "Camera unavailable in this build.\nUse barcode lookup or build with CAMERA=ON.");
#endif
    }
    if (!m_code.isEmpty()) {
        p.setPen(Qt::white);
        p.drawText(r.adjusted(12, 0, -12, -8), Qt::AlignBottom | Qt::AlignRight, m_code);
    }
}

// ------------------------------------------------------------------ RevenueChart (3D bars)
RevenueChart::RevenueChart(QWidget *parent) : QWidget(parent) {}

void RevenueChart::setData(const QStringList &labels, const QVector<double> &actual, const QVector<double> &target)
{
    m_labels = labels;
    m_actual = actual;
    m_target = target;
    update();
}

void RevenueChart::paintEvent(QPaintEvent *)
{
    const int n = m_actual.size();
    if (n < 2)
        return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal depth = 7;
    const QRectF area(40, 12 + depth, width() - 40 - 10 - depth, height() - 12 - depth - 24);
    double maxV = 1;
    for (double v : m_actual)
        maxV = qMax(maxV, v);
    for (double v : m_target)
        maxV = qMax(maxV, v);
    maxV = qCeil(maxV * 1.1 / 200.0) * 200.0;

    QFont f = font();
    f.setPixelSize(11);
    p.setFont(f);
    // grid + y labels
    for (int g = 0; g <= 4; ++g)
    {
        const qreal y = area.bottom() - area.height() * g / 4.0;
        p.setPen(QPen(QColor("#e1e8f8"), 1, g == 0 ? Qt::SolidLine : Qt::DashLine));
        p.drawLine(QPointF(area.left(), y), QPointF(area.right() + depth, y));
        p.setPen(Theme::outline);
        p.drawText(QRectF(0, y - 8, 34, 16), Qt::AlignRight | Qt::AlignVCenter, QString("$%1").arg(int(maxV * g / 4)));
    }

    const qreal slot = area.width() / n;
    const qreal bw = slot * 0.54;
    int peak = 0;
    for (int i = 1; i < n; ++i)
        if (m_actual[i] > m_actual[peak])
            peak = i;

    for (int i = 0; i < n; ++i)
    {
        const bool hot = i == peak;
        const qreal x = area.left() + slot * i + (slot - bw) / 2;
        const qreal h = qMax<qreal>(3, area.height() * m_actual[i] / maxV);
        const qreal y = area.bottom() - h;
        const QColor c1 = hot ? QColor("#7fb0ff") : QColor("#7aa6f7");
        const QColor c2 = hot ? QColor("#2457e6") : QColor("#3f6fe0");
        // contact shadow
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(20, 40, 110, 40));
        p.drawEllipse(QRectF(x - 2, area.bottom() - 3, bw + depth + 4, 7));
        // front
        QLinearGradient fg(x, y, x, y + h);
        fg.setColorAt(0, c1);
        fg.setColorAt(1, c2);
        p.setBrush(fg);
        p.drawRect(QRectF(x, y, bw, h));
        // side
        p.setBrush(c2.darker(135));
        p.drawPolygon(QPolygonF({{x + bw, y}, {x + bw + depth, y - depth}, {x + bw + depth, y + h - depth}, {x + bw, y + h}}));
        // top
        p.setBrush(c1.lighter(125));
        p.drawPolygon(QPolygonF({{x, y}, {x + depth, y - depth}, {x + bw + depth, y - depth}, {x + bw, y}}));
        // gloss strip
        p.setBrush(QColor(255, 255, 255, 45));
        p.drawRect(QRectF(x + 2, y + 2, bw * 0.25, h - 4));

        if (hot)
        {
            p.setPen(Theme::primary);
            QFont bf = f;
            bf.setBold(true);
            p.setFont(bf);
            p.drawText(QRectF(x - 16, y - depth - 18, bw + depth + 32, 16), Qt::AlignCenter, QString("$%1").arg(int(m_actual[i])));
            p.setFont(f);
        }
        p.setPen(hot ? Theme::primary : Theme::onSurfaceVariant);
        if (i < m_labels.size())
        {
            QFont lf = f;
            lf.setBold(hot);
            p.setFont(lf);
            p.drawText(QRectF(x - 6, area.bottom() + 6, bw + depth + 12, 16), Qt::AlignCenter, m_labels[i]);
            p.setFont(f);
        }
    }

    // target line
    if (m_target.size() == n)
    {
        QPainterPath path;
        for (int i = 0; i < n; ++i)
        {
            const qreal x = area.left() + slot * i + slot / 2 + depth / 2;
            const qreal y = area.bottom() - area.height() * m_target[i] / maxV - depth / 2;
            if (i == 0)
                path.moveTo(x, y);
            else
                path.lineTo(x, y);
        }
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor("#f08a24"), 2, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(path);
    }
}

// ------------------------------------------------------------------ QtyStepper
QtyStepper::QtyStepper(int value, QWidget *parent) : QWidget(parent)
{
    auto *h = new QHBoxLayout(this);
    h->setContentsMargins(4, 2, 4, 2);
    h->setSpacing(6);
    auto *m = W::button("−", "step");
    auto *pl = W::button("+", "step");
    auto *v = W::label(QString::number(value));
    v->setAlignment(Qt::AlignCenter);
    v->setMinimumWidth(22);
    v->setStyleSheet("font-weight:700; color:#004ac6;");
    h->addWidget(m);
    h->addWidget(v);
    h->addWidget(pl);
    connect(m, &QPushButton::clicked, this, &QtyStepper::minus);
    connect(pl, &QPushButton::clicked, this, &QtyStepper::plus);
}
