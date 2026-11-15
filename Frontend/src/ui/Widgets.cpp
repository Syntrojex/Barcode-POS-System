#include "Widgets.h"
#include "Theme.h"
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QtMath>

namespace W
{
    QFrame *card(QWidget *parent, const char *objectName)
    {
        auto *f = new QFrame(parent);
        f->setObjectName(objectName);
        return f;
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
            return "-$" + s;
        return (sign ? "+$" : "$") + s;
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
            h->addWidget(right);
        into->addLayout(h);
        return h;
    }
}

// ------------------------------------------------------------------ CameraFeedWidget
CameraFeedWidget::CameraFeedWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(150);
    connect(&m_timer, &QTimer::timeout, this, [this]
            {
        m_phase += 0.012;
        if (m_phase > 1) m_phase = 0;
        update(); });
    m_timer.start(33);
}

void CameraFeedWidget::setRunning(bool on)
{
    m_running = on;
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
    const QRectF r = rect();
    QPainterPath clip;
    clip.addRoundedRect(r, 8, 8);
    p.setClipPath(clip);

    QLinearGradient g(r.topLeft(), r.bottomRight());
    g.setColorAt(0, QColor("#2b3f5c"));
    g.setColorAt(1, QColor("#0b1c30"));
    p.fillRect(r, g);

    if (!m_running)
    {
        p.fillRect(r, QColor(0, 0, 0, 120));
        p.setPen(QColor("#b4c5ff"));
        QFont f = font();
        f.setPointSizeF(11);
        f.setBold(true);
        p.setFont(f);
        p.drawText(r, Qt::AlignCenter, "Camera stopped");
        return;
    }

    // soft "scene" shapes
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 16));
    p.drawRoundedRect(QRectF(r.width() * .06, r.height() * .16, r.width() * .30, r.height() * .52), 12, 12);
    p.drawRoundedRect(QRectF(r.width() * .64, r.height() * .10, r.width() * .30, r.height() * .62), 12, 12);

    // a product label with the barcode
    const QRectF label(r.width() * .28, r.height() * .22, r.width() * .44, r.height() * .5);
    p.setBrush(QColor(238, 244, 255, 235));
    p.drawRoundedRect(label, 6, 6);
    p.setBrush(QColor("#0b1c30"));
    const QString digits = m_code.isEmpty() ? QString("00000000") : m_code;
    qreal x = label.left() + label.width() * .08;
    const qreal barW = label.width() * .84 / (digits.size() * 7 + 6);
    const qreal top = label.top() + label.height() * .14, h = label.height() * .56;
    auto bar = [&](int w)
    { p.drawRect(QRectF(x, top, barW * w, h)); };
    for (QChar ch : digits)
    {
        const int d = ch.isDigit() ? ch.digitValue() : 0;
        for (int i = 0; i < 4; ++i)
        {
            const int w = 1 + ((d + i * 3) % 3 == 0 ? 1 : 0) + (i == 1 ? 1 : 0);
            if (i % 2 == 0)
                bar(w);
            x += barW * w * 1.0;
        }
        x += barW * 0.2;
    }
    p.setPen(QColor("#0b1c30"));
    QFont mf("Consolas");
    mf.setStyleHint(QFont::Monospace);
    mf.setPixelSize(qMax(9.0, label.height() * .12));
    p.setFont(mf);
    p.drawText(QRectF(label.left(), top + h, label.width(), label.bottom() - top - h),
               Qt::AlignCenter, digits);

    // corner brackets
    const QColor green("#4ade80");
    p.setPen(QPen(green, 2.2));
    const QRectF b = r.adjusted(r.width() * .14, r.height() * .10, -r.width() * .14, -r.height() * .14);
    const qreal L = 18;
    auto corner = [&](QPointF c, int sx, int sy)
    {
        p.drawLine(c, c + QPointF(sx * L, 0));
        p.drawLine(c, c + QPointF(0, sy * L));
    };
    corner(b.topLeft(), 1, 1);
    corner(b.topRight(), -1, 1);
    corner(b.bottomLeft(), 1, -1);
    corner(b.bottomRight(), -1, -1);

    // sweeping scan line
    const qreal y = b.top() + (b.height()) * (0.5 + 0.5 * qSin(m_phase * 2 * M_PI));
    QLinearGradient sg(b.left(), y, b.right(), y);
    sg.setColorAt(0, QColor(74, 222, 128, 0));
    sg.setColorAt(.5, QColor(74, 222, 128, 230));
    sg.setColorAt(1, QColor(74, 222, 128, 0));
    p.setPen(QPen(QBrush(sg), 2));
    p.drawLine(QPointF(b.left(), y), QPointF(b.right(), y));

    // overlay chips
    QFont cf = font();
    cf.setPixelSize(11);
    cf.setWeight(QFont::DemiBold);
    p.setFont(cf);
    auto chip = [&](const QString &t, QPointF at, bool right, const QColor &bg, const QColor &fg)
    {
        QFontMetricsF fm(cf);
        const qreal w = fm.horizontalAdvance(t) + 16, hh = 20;
        QRectF rc(right ? at.x() - w : at.x(), at.y() - hh, w, hh);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(rc, 4, 4);
        p.setPen(fg);
        p.drawText(rc, Qt::AlignCenter, t);
    };
    chip("Scanning Reticle Active", QPointF(r.center().x() - 60, r.height() * .5 + 10), false,
         QColor(11, 28, 48, 170), QColor("#7ffc97"));
    chip("Lens: Dual-Band CMOS", QPointF(10, r.bottom() - 8), false, QColor(11, 28, 48, 170), QColor("#eaf1ff"));
    chip("Gain: +1.2EV", QPointF(r.right() - 10, r.bottom() - 8), true, QColor("#2563eb"), Qt::white);
    chip("SIMULATED", QPointF(r.right() - 10, 30), true, QColor(255, 255, 255, 40), QColor("#dbe1ff"));
}

// ------------------------------------------------------------------ RevenueChart
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
    if (m_actual.size() < 2)
        return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF area(8, 26, width() - 16, height() - 26 - 24);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#f8f9ff"));
    p.drawRoundedRect(QRectF(0, 0, width(), height()), 8, 8);

    // header: title + legend
    QFont f = font();
    f.setPixelSize(11);
    f.setBold(true);
    p.setFont(f);
    p.setPen(Theme::onSurfaceVariant);
    p.drawText(QRectF(10, 4, 200, 18), Qt::AlignVCenter | Qt::AlignLeft, "Revenue Pace ($)");
    f.setBold(false);
    p.setFont(f);
    {
        const QFontMetricsF fm(f);
        const qreal w2 = fm.horizontalAdvance("Target Quota"), w1 = fm.horizontalAdvance("Actual Revenue");
        qreal x = width() - 10 - w2;
        p.setPen(Theme::onSurfaceVariant);
        p.drawText(QRectF(x, 4, w2 + 2, 18), Qt::AlignVCenter, "Target Quota");
        x -= 22;
        p.setPen(QPen(Theme::outline, 1.3, Qt::DashLine));
        p.drawLine(QPointF(x, 13), QPointF(x + 16, 13));
        x -= 16 + w1;
        p.setPen(Theme::onSurfaceVariant);
        p.drawText(QRectF(x, 4, w1 + 2, 18), Qt::AlignVCenter, "Actual Revenue");
        x -= 14;
        p.setBrush(Theme::primaryContainer);
        p.setPen(Qt::NoPen);
        p.drawRect(QRectF(x, 9, 8, 8));
    }

    double maxV = 1;
    for (double v : m_actual)
        maxV = qMax(maxV, v);
    for (double v : m_target)
        maxV = qMax(maxV, v);
    maxV *= 1.15;
    const int n = m_actual.size();
    auto pt = [&](int i, double v)
    {
        return QPointF(area.left() + 8 + (area.width() - 16) * i / (n - 1), area.bottom() - area.height() * v / maxV);
    };

    // grid
    p.setPen(QPen(QColor("#dce9ff"), 1, Qt::DashLine));
    for (int g = 1; g <= 3; ++g)
    {
        const qreal y = area.bottom() - area.height() * g / 3.5;
        p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
    }

    auto smooth = [&](const QVector<double> &d)
    {
        QPainterPath path;
        path.moveTo(pt(0, d[0]));
        for (int i = 1; i < d.size(); ++i)
        {
            const QPointF a = pt(i - 1, d[i - 1]), b = pt(i, d[i]);
            const qreal mx = (a.x() + b.x()) / 2;
            path.cubicTo(QPointF(mx, a.y()), QPointF(mx, b.y()), b);
        }
        return path;
    };

    // target (dashed)
    if (m_target.size() == n)
    {
        p.setPen(QPen(Theme::outline, 1.4, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawPath(smooth(m_target));
    }
    // actual area + line
    QPainterPath line = smooth(m_actual);
    QPainterPath fill = line;
    fill.lineTo(pt(n - 1, 0));
    fill.lineTo(pt(0, 0));
    fill.closeSubpath();
    QLinearGradient ag(0, area.top(), 0, area.bottom());
    ag.setColorAt(0, QColor(37, 99, 235, 70));
    ag.setColorAt(1, QColor(37, 99, 235, 4));
    p.setPen(Qt::NoPen);
    p.setBrush(ag);
    p.drawPath(fill);
    p.setPen(QPen(Theme::primary, 2.4));
    p.setBrush(Qt::NoBrush);
    p.drawPath(line);

    // peak marker
    int peak = 0;
    for (int i = 1; i < n; ++i)
        if (m_actual[i] > m_actual[peak])
            peak = i;
    const QPointF pk = pt(peak, m_actual[peak]);
    p.setBrush(Qt::white);
    p.setPen(QPen(Theme::primary, 2.2));
    p.drawEllipse(pk, 5.5, 5.5);
    p.setBrush(Theme::primary);
    p.drawEllipse(pk, 2.2, 2.2);

    // x labels
    for (int i = 0; i < n && i < m_labels.size(); ++i)
    {
        const bool hot = (i == peak);
        QFont lf = font();
        lf.setPixelSize(11);
        lf.setBold(hot);
        p.setFont(lf);
        p.setPen(hot ? Theme::primary : Theme::onSurfaceVariant);
        const QPointF c = pt(i, 0);
        p.drawText(QRectF(c.x() - 20, area.bottom() + 4, 40, 18), Qt::AlignCenter, m_labels[i]);
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
