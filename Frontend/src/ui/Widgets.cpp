#include "Widgets.h"
#include "AppState.h"
#include "Theme.h"
#ifdef WITH_CAMERA
#include "decoder.h"
#ifdef WITH_CAMERA_OPENCV
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#endif
#ifdef WITH_CAMERA_QT
#include <QCamera>
#include <QCameraDevice>
#include <QCoreApplication>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QVideoFrame>
#include <QVideoSink>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QPermission>
#endif
#endif // WITH_CAMERA_QT
#endif // WITH_CAMERA
#include <QHBoxLayout>
#include <QTimer>
#include <cstdlib>
#include <cmath>
#include <QApplication>
#include <QIconEngine>
#include <QLinearGradient>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <algorithm>

namespace
{
    class ThemeButtonIcon final : public QIconEngine
    {
    public:
        ThemeButtonIcon(const QString &name, const QColor &color) : m_name(name), m_color(color) {}

        QIconEngine *clone() const override { return new ThemeButtonIcon(m_name, m_color); }

        QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
        {
            QPixmap result(size);
            result.fill(Qt::transparent);
            QPainter painter(&result);
            paint(&painter, result.rect(), mode, state);
            return result;
        }

        void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State) override
        {
            painter->save();
            if (mode == QIcon::Disabled)
                painter->setOpacity(painter->opacity() * 0.5);
            const int size = qMin(rect.width(), rect.height());
            painter->drawPixmap(rect, Theme::iconPixmap(m_name, Theme::readable(m_color), size));
            painter->restore();
        }

    private:
        QString m_name;
        QColor m_color;
    };
}

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
            b->setIcon(QIcon(new ThemeButtonIcon(icon, c)));
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

void Card::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF body = QRectF(rect()).adjusted(8.5, 4.5, -8.5, -10.5);
    const bool dark = Theme::isDark();

    p.setPen(Qt::NoPen);
    p.setBrush(dark ? QColor(0, 0, 0, 32) : QColor(47, 61, 114, 16));
    p.drawRoundedRect(body.translated(0, 3).adjusted(-2, 0, 2, 2), 15, 15);
    const auto fill = m_alert ? (dark ? QColor("#352434") : QColor("#fff5f6"))
                              : (dark ? Theme::darkPanel : Theme::white);
    const auto border = m_alert ? (dark ? QColor("#765064") : QColor("#e7b9c1"))
                                : (dark ? Theme::darkDivider : Theme::outlineVariant);
    p.setBrush(fill);
    p.setPen(border);
    p.drawRoundedRect(body, 14, 14);
}

// ------------------------------------------------------------------ CameraFeedWidget
#ifdef WITH_CAMERA_QT
struct CameraFeedWidget::CameraRig
{
    explicit CameraRig(const QCameraDevice &dev) : camera(dev) {}
    QCamera camera;
    QMediaCaptureSession session;
    QVideoSink sink;
    Decoder decoder;
};
#endif
#ifdef WITH_CAMERA_OPENCV
struct CameraFeedWidget::CameraRig
{
    cv::VideoCapture capture;
    QTimer timer;
    Decoder decoder;
};
#endif

CameraFeedWidget::CameraFeedWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(150);
    setMouseTracking(true);
    m_resultClock.invalidate();
    // repaint while the confirmation banner is visible so it disappears on time
    auto *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [this] { if (m_resultClock.isValid()) update(); });
    t->start(300);
    // the red scan line sweeps up and down by itself while the camera runs
    auto *sweep = new QTimer(this);
    connect(sweep, &QTimer::timeout, this, [this] {
        if (!m_running || m_frame.isNull()) return;
        m_sweepPhase += 0.045;
        m_lineFrac = 0.5 + 0.4 * std::sin(m_sweepPhase);
        update();
    });
    sweep->start(30);
}
CameraFeedWidget::~CameraFeedWidget() = default;

// The picture is letter-boxed inside the widget; these map the aiming line to widget coordinates.
static QRect pictureRect(const QWidget *w, const QImage &img)
{
    if (img.isNull()) return w->rect();
    const QSize s = img.size().scaled(w->rect().adjusted(1, 1, -1, -1).size(), Qt::KeepAspectRatio);
    return QRect(QPoint((w->width() - s.width()) / 2, (w->height() - s.height()) / 2), s);
}

bool CameraFeedWidget::nearLine(const QPoint &p) const
{
    if (m_frame.isNull()) return false;
    const QRect pic = pictureRect(this, m_frame);
    const int y = pic.top() + static_cast<int>(pic.height() * m_lineFrac);
    return pic.contains(p) && std::abs(p.y() - y) <= 14;
}

void CameraFeedWidget::moveLine(const QPoint &p)
{
    const QRect pic = pictureRect(this, m_frame);
    m_lineFrac = qBound(0.05, double(p.y() - pic.top()) / qMax(1, pic.height()), 0.95);
    update();
}

void CameraFeedWidget::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e); // the line moves by itself now
}
void CameraFeedWidget::mouseMoveEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
}
void CameraFeedWidget::mouseReleaseEvent(QMouseEvent *) { m_drag = false; }

void CameraFeedWidget::showResult(const QString &text, bool ok)
{
    m_result = text;
    m_resultOk = ok;
    m_resultClock.start();
    update();
}

void CameraFeedWidget::setRunning(bool on)
{
#ifdef WITH_CAMERA
    if (!on) {
        m_rig.reset(); // stops the camera and releases the device
        m_running = false;
        m_frame = QImage();
        update();
        return;
    }
    if (m_running) return;

#ifdef WITH_CAMERA_QT
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // Needed on macOS/mobile; on Windows it is simply granted.
    QCameraPermission permission;
    const auto status = qApp->checkPermission(permission);
    if (status == Qt::PermissionStatus::Denied) {
        emit scanError("Camera access is blocked. Allow it in Windows Settings > Privacy > Camera.");
        return;
    }
    if (status == Qt::PermissionStatus::Undetermined) {
        qApp->requestPermission(permission, this, [this](const QPermission &p) {
            if (p.status() == Qt::PermissionStatus::Granted) setRunning(true);
            else emit scanError("Camera permission was not granted.");
        });
        return;
    }
#endif
    const QCameraDevice device = QMediaDevices::defaultVideoInput();
    if (device.isNull()) {
        emit scanError("No camera found. Plug in a webcam (and close other apps that use it).");
        return;
    }
    m_rig = std::make_unique<CameraRig>(device);
    connect(&m_rig->sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &f) { onImage(f.toImage()); });
    connect(&m_rig->camera, &QCamera::errorOccurred, this, [this](QCamera::Error, const QString &why) {
        setRunning(false);
        emit scanError(why.isEmpty() ? QString("The camera stopped working.") : why);
    });
    m_rig->session.setCamera(&m_rig->camera);
    m_rig->session.setVideoSink(&m_rig->sink);
    m_lastDetected.clear();
    m_lastEmit.invalidate();
    m_armed = true;
    m_decodeClock.start();
    m_running = true;
    m_rig->camera.start();
#endif
#ifdef WITH_CAMERA_OPENCV
    m_rig = std::make_unique<CameraRig>();
#ifdef _WIN32
    m_rig->capture.open(0, cv::CAP_DSHOW); // DirectShow: fastest to open on Windows
    if (!m_rig->capture.isOpened()) m_rig->capture.open(0);
#else
    m_rig->capture.open(0);
#endif
    if (!m_rig->capture.isOpened()) {
        m_rig.reset();
        emit scanError("Cannot open the camera. Plug in a webcam and close other apps that use it.");
        return;
    }
    m_rig->capture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    m_rig->capture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    connect(&m_rig->timer, &QTimer::timeout, this, [this] {
        if (!m_rig) return;
        cv::Mat bgr;
        if (!m_rig->capture.read(bgr) || bgr.empty()) {
            setRunning(false);
            emit scanError("The camera stopped sending pictures.");
            return;
        }
        cv::Mat rgb;
        cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
        onImage(QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888).copy());
    });
    m_lastDetected.clear();
    m_lastEmit.invalidate();
    m_armed = true;
    m_decodeClock.start();
    m_running = true;
    m_rig->timer.start(33);
#endif
    update();
#else
    if (on)
        emit scanError("Camera support is not built in. Install the Qt Multimedia component and reconfigure CMake.");
#endif
}

#ifdef WITH_CAMERA
void CameraFeedWidget::onImage(const QImage &img)
{
    if (!m_running || !m_rig) return;
    if (img.isNull()) return;
    m_frame = img;
    update();
    if (m_decodeClock.elapsed() < 120) return; // decode ~8 times a second, show every frame
    m_decodeClock.restart();

    QImage g = img.convertToFormat(QImage::Format_Grayscale8);
    if (g.width() > 960) g = g.scaledToWidth(960, Qt::FastTransformation);
    GrayImage gray;
    gray.width = g.width();
    gray.height = g.height();
    gray.pixels.resize(static_cast<std::size_t>(gray.width) * gray.height);
    for (int y = 0; y < gray.height; ++y)
        std::copy(g.constScanLine(y), g.constScanLine(y) + gray.width, gray.pixels.begin() + static_cast<long>(y) * gray.width);

    std::string code;
    if (m_rig->decoder.decodeCropping(gray, code, m_lineFrac)) {
        const QString value = QString::fromStdString(code);
        const auto h = m_rig->decoder.lastHit();
        if (h.valid) {
            const double k = static_cast<double>(img.width()) / gray.width; // back to camera-picture pixels
            m_hitBox = QRectF(QPointF(h.x0 * k, h.y0 * k), QPointF(h.x1 * k, h.y1 * k)).normalized();
        } else {
            m_hitBox = QRectF();
        }
        m_hitText = value;
        m_hitClock.start();
        // Scan once: report a barcode a single time, then wait until it leaves the picture.
        m_lastEmit.restart();
        if (m_armed) {
            m_armed = false;
            m_lastDetected = value;
            showResult(QString(), true); // green flash only, no text on the camera
            emit barcodeDetected(value);
        }
    } else if (!m_armed && m_lastEmit.isValid() && m_lastEmit.elapsed() > 1000) {
        m_armed = true; // no barcode for 1 second: ready for the next scan
    }
}
#endif

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
        const QPoint origin((width() - scaled.width()) / 2, (height() - scaled.height()) / 2);
        p.drawImage(origin, scaled);
        // aiming line (drag it up/down onto the barcode) + hint
        const int ly = origin.y() + static_cast<int>(scaled.height() * m_lineFrac);
        p.setPen(QPen(QColor(255, 60, 60, 220), 2));
        p.drawLine(origin.x() + 24, ly, origin.x() + scaled.width() - 24, ly);
        // soft glow trail behind the line
        for (int k = 1; k <= 10; ++k) {
            p.setPen(QPen(QColor(255, 60, 60, 24 - k * 2), 2));
            p.drawLine(origin.x() + 24, ly - k * 2, origin.x() + scaled.width() - 24, ly - k * 2);
        }
        p.setPen(Qt::NoPen);
        p.setPen(Qt::white);
        // short hint on the live picture (elided so it never gets cut off)
        const QRect hint(origin.x() + 10, origin.y() + 8, scaled.width() - 20, 20);
        p.drawText(hint, Qt::AlignTop | Qt::AlignHCenter,
                   p.fontMetrics().elidedText("Hold the barcode on the red line", Qt::ElideRight, hint.width()));
        // "barcode read" confirmation
        if (m_resultClock.isValid() && m_resultClock.elapsed() < 3000) {
            const QColor c = m_resultOk ? QColor(34, 197, 94) : QColor(239, 68, 68);
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(c, 4));
            p.drawRect(QRect(origin, scaled.size()).adjusted(2, 2, -2, -2));
        }
        // the barcode that was just read: green box for ~2.5 s (no text)
        if (m_hitClock.isValid() && m_hitClock.elapsed() < 2500 && !m_hitText.isEmpty()) {
            const double k = static_cast<double>(scaled.width()) / m_frame.width();
            if (m_hitBox.isValid() && !m_hitBox.isEmpty()) {
                const QRectF box(origin.x() + m_hitBox.x() * k, origin.y() + m_hitBox.y() * k,
                                 m_hitBox.width() * k, m_hitBox.height() * k);
                p.setBrush(QColor(40, 200, 90, 45));
                p.setPen(QPen(QColor(40, 220, 100), 3));
                p.drawRoundedRect(box.adjusted(-6, -6, 6, 6), 6, 6);
            }
        }
    } else {
        p.setPen(QColor("#c2d0ef"));
#ifdef WITH_CAMERA
        p.drawText(r, Qt::AlignCenter, "Camera stopped — press Start camera");
#else
        p.drawText(r.adjusted(15, 0, -15, 0), Qt::AlignCenter | Qt::TextWordWrap,
                   "Camera unavailable in this build.\nInstall the Qt Multimedia component, then re-run CMake.\nTyped barcode lookup still works.");
#endif
    }
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
    const int count = m_actual.size();
    if (count == 0)
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const bool dark = Theme::isDark();
    const QColor muted = dark ? Theme::darkMuted : Theme::onSurfaceVariant;
    const QColor grid = dark ? Theme::darkDivider : Theme::outlineVariant;
    const QLocale locale;
    QFont labels = font();
    labels.setPixelSize(11);
    p.setFont(labels);

    const double peak = *std::max_element(m_actual.cbegin(), m_actual.cend());
    const double targetPeak = m_target.isEmpty() ? 0.0 : *std::max_element(m_target.cbegin(), m_target.cend());
    const double maximum = std::ceil(qMax(1.0, qMax(peak, targetPeak)) * 1.1 / 200.0) * 200.0;
    const QString maxLabel = locale.toString(maximum, 'f', 0);
    const int axisWidth = p.fontMetrics().horizontalAdvance(maxLabel) + 12;
    const QRectF area = QRectF(rect()).adjusted(axisWidth, 12, -8, -26);
    if (area.isEmpty())
        return;

    for (int step = 0; step <= 4; ++step)
    {
        const qreal y = area.bottom() - area.height() * step / 4.0;
        p.setPen(QPen(grid, 1, step == 0 ? Qt::SolidLine : Qt::DashLine));
        p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
        p.setPen(muted);
        p.drawText(QRectF(0, y - 8, axisWidth - 8, 16), Qt::AlignRight | Qt::AlignVCenter,
                   locale.toString(maximum * step / 4.0, 'f', 0));
    }

    const qreal slot = area.width() / count;
    const qreal barWidth = qMin(32.0, slot * 0.62);
    for (int i = 0; i < count; ++i)
    {
        const qreal x = area.left() + slot * i;
        if (m_actual[i] > 0)
        {
            const qreal barHeight = area.height() * m_actual[i] / maximum;
            const qreal y = area.bottom() - barHeight;
            QLinearGradient fill(x, y, x, area.bottom());
            fill.setColorAt(0, dark ? QColor("#9aaaff") : QColor("#426de6"));
            fill.setColorAt(1, dark ? QColor("#5064cd") : QColor("#9bb2f7"));
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawRoundedRect(QRectF(x + (slot - barWidth) / 2, y, barWidth, barHeight), 4, 4);
        }
        if (i < m_labels.size())
        {
            p.setPen(muted);
            p.drawText(QRectF(x, area.bottom() + 5, slot, 16), Qt::AlignCenter, m_labels[i]);
        }
    }

    if (targetPeak > 0 && m_target.size() == count)
    {
        QPainterPath path;
        path.moveTo(area.left() + slot / 2, area.bottom() - area.height() * m_target[0] / maximum);
        for (int i = 1; i < count; ++i)
            path.lineTo(area.left() + slot * (i + 0.5), area.bottom() - area.height() * m_target[i] / maximum);
        p.strokePath(path, QPen(dark ? QColor("#f6bf87") : QColor("#ad5d21"), 2, Qt::DashLine));
    }

    if (peak <= 0 && targetPeak <= 0)
    {
        p.setPen(muted);
        p.drawText(area, Qt::AlignCenter, "No paid sales in these hours");
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
    auto *v = W::label(QString::number(value), "quantityValue");
    v->setAlignment(Qt::AlignCenter);
    v->setMinimumWidth(22);
    h->addWidget(m);
    h->addWidget(v);
    h->addWidget(pl);
    connect(m, &QPushButton::clicked, this, &QtyStepper::minus);
    connect(pl, &QPushButton::clicked, this, &QtyStepper::plus);
}
