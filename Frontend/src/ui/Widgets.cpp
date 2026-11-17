#include "Widgets.h"
#include "Theme.h"
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QTableWidget>
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

    QLabel *iconTile(const QString &icon, const QColor &accent, int size)
    {
        auto *l = new QLabel;
        l->setFixedSize(size, size);
        l->setAlignment(Qt::AlignCenter);
        QColor tint = accent;
        tint.setAlpha(30);
        l->setStyleSheet(QString("background: rgba(%1,%2,%3,%4); border-radius:%5px;")
                             .arg(tint.red()).arg(tint.green()).arg(tint.blue()).arg(tint.alpha())
                             .arg(size / 3));
        l->setPixmap(Theme::iconPixmap(icon, accent, size / 2));
        return l;
    }

    void alignHeaders(QTableWidget *t, const QList<int> &cols, Qt::Alignment a)
    {
        for (int c : cols)
            if (auto *h = t->horizontalHeaderItem(c))
                h->setTextAlignment(a | Qt::AlignVCenter);
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
        f->setObjectName("hline");
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
    rebuildShadow();
}

void Card::rebuildShadow()
{
    m_shadowDark = Theme::isDark();
    const qreal dpr = devicePixelRatioF();
    m_shadow = QPixmap(QSize(width(), height()) * dpr);
    m_shadow.setDevicePixelRatio(dpr);
    m_shadow.fill(Qt::transparent);
    QPainter p(&m_shadow);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    // three faint, tight layers: a subtle lift rather than a heavy drop shadow
    const QRectF body = QRectF(rect()).adjusted(8, 4, -8, -10);
    for (int i = 5; i >= 1; --i)
    {
        p.setBrush(Theme::palette().shadow);
        p.drawRoundedRect(body.translated(0, 2.5).adjusted(-i, -i + 1, i, i + 1), 12 + i, 12 + i);
    }
}

void Card::paintEvent(QPaintEvent *)
{
    if (m_shadowDark != Theme::isDark())
        rebuildShadow(); // the mode changed since the last draw
    const Theme::Palette &pal = Theme::palette();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.drawPixmap(0, 0, m_shadow);
    const QRectF body = QRectF(rect()).adjusted(8, 4, -8, -10);
    p.setBrush(m_alert ? pal.alertBg : pal.cardBg);
    p.setPen(QPen(m_alert ? pal.alertBorder : pal.cardBorder, 1));
    p.drawRoundedRect(body.adjusted(.5, .5, -.5, -.5), 12, 12);
}

// ------------------------------------------------------------------ CameraFeedWidget
CameraFeedWidget::CameraFeedWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(96);
    connect(&m_timer, &QTimer::timeout, this, [this]
            {
        if (!m_running) return;
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
    const QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1);
    QPainterPath clip;
    clip.addRoundedRect(r, 10, 10);
    p.setClipPath(clip);
    p.fillRect(r, QColor("#0f1b33")); // flat viewport

    if (!m_running)
    {
        p.setPen(QColor("#8d9abd"));
        QFont f = font();
        f.setPixelSize(13);
        f.setBold(true);
        p.setFont(f);
        p.drawText(r, Qt::AlignCenter, "Camera stopped");
    }
    else
    {
        // product label with barcode
        const QRectF label(r.width() * .24, r.height() * .2, r.width() * .52, r.height() * .56);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#ffffff"));
        p.drawRoundedRect(label, 8, 8);
        p.setBrush(QColor("#0f1b33"));
        const QString digits = m_code.isEmpty() ? QString("00000000") : m_code;
        qreal x = label.left() + label.width() * .08;
        const qreal barW = label.width() * .84 / (digits.size() * 7 + 6);
        const qreal top = label.top() + label.height() * .14, h = label.height() * .56;
        for (QChar ch : digits)
        {
            const int d = ch.isDigit() ? ch.digitValue() : 0;
            for (int i = 0; i < 4; ++i)
            {
                const int w = 1 + ((d + i * 3) % 3 == 0 ? 1 : 0) + (i == 1 ? 1 : 0);
                if (i % 2 == 0)
                    p.drawRect(QRectF(x, top, barW * w, h));
                x += barW * w;
            }
            x += barW * 0.2;
        }
        p.setPen(QColor("#0f1b33"));
        QFont mf("Consolas");
        mf.setStyleHint(QFont::Monospace);
        mf.setPixelSize(qMax(10, int(label.height() * .13)));
        p.setFont(mf);
        p.drawText(QRectF(label.left(), top + h, label.width(), label.bottom() - top - h), Qt::AlignCenter, digits);

        // corner brackets
        p.setPen(QPen(QColor("#4ade80"), 2.5, Qt::SolidLine, Qt::RoundCap));
        const QRectF b = r.adjusted(r.width() * .14, r.height() * .12, -r.width() * .14, -r.height() * .12);
        const qreal L = 20;
        auto corner = [&](QPointF c, int sx, int sy)
        {
            p.drawLine(c, c + QPointF(sx * L, 0));
            p.drawLine(c, c + QPointF(0, sy * L));
        };
        corner(b.topLeft(), 1, 1);
        corner(b.topRight(), -1, 1);
        corner(b.bottomLeft(), 1, -1);
        corner(b.bottomRight(), -1, -1);

        // sweeping laser line
        const qreal y = b.top() + b.height() * (0.5 + 0.5 * qSin(m_phase * 2 * 3.14159265358979));
        p.setPen(QPen(QColor(74, 222, 128, 230), 2));
        p.drawLine(QPointF(b.left(), y), QPointF(b.right(), y));
    }
    p.setClipping(false);
    p.setPen(QPen(QColor("#1c2b4d"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r, 10, 10);
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
    const int n = m_actual.size();
    if (n < 2)
        return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const Theme::Palette &pal = Theme::palette();
    const QRectF area(44, 16, width() - 44 - 8, height() - 16 - 26);
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
        p.setPen(QPen(pal.chartGrid, 1));
        p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
        p.setPen(pal.subtle);
        p.drawText(QRectF(0, y - 8, 38, 16), Qt::AlignRight | Qt::AlignVCenter, QString("$%1").arg(int(maxV * g / 4)));
    }

    const qreal slot = area.width() / n;
    const qreal bw = qMin<qreal>(slot * 0.56, 34);
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
        // flat bar, rounded on top only
        QPainterPath bar;
        bar.addRoundedRect(QRectF(x, y, bw, h), 4, 4);
        bar.addRect(QRectF(x, qMax(y, area.bottom() - 4), bw, qMin<qreal>(4, h)));
        p.setPen(Qt::NoPen);
        p.setBrush(hot ? pal.chartBarHot : pal.chartBar);
        p.drawPath(bar.simplified());

        if (hot)
        {
            p.setPen(pal.accentText);
            QFont bf = f;
            bf.setBold(true);
            p.setFont(bf);
            p.drawText(QRectF(x - 16, y - 20, bw + 32, 16), Qt::AlignCenter, QString("$%1").arg(int(m_actual[i])));
            p.setFont(f);
        }
        p.setPen(hot ? pal.accentText : pal.muted);
        if (i < m_labels.size())
        {
            QFont lf = f;
            lf.setBold(hot);
            p.setFont(lf);
            p.drawText(QRectF(x - 8, area.bottom() + 6, bw + 16, 16), Qt::AlignCenter, m_labels[i]);
            p.setFont(f);
        }
    }

    // target line
    if (m_target.size() == n)
    {
        QPainterPath path;
        for (int i = 0; i < n; ++i)
        {
            const qreal x = area.left() + slot * i + slot / 2;
            const qreal y = area.bottom() - area.height() * m_target[i] / maxV;
            if (i == 0)
                path.moveTo(x, y);
            else
                path.lineTo(x, y);
        }
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(pal.chartTarget, 1.8, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
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
    v->setStyleSheet("font-weight:700;");
    h->addWidget(m);
    h->addWidget(v);
    h->addWidget(pl);
    connect(m, &QPushButton::clicked, this, &QtyStepper::minus);
    connect(pl, &QPushButton::clicked, this, &QtyStepper::plus);
}
