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
#include <thread>
#include <QApplication>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX // keep std::min/std::max usable
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Short "scanner" beep after a successful scan (if enabled in Settings).
// Windows Beep() blocks while it plays, so it runs on its own short-lived thread.
static void scanBeep()
{
    if (!AppState::instance().settings().beepOnScan)
        return;
#ifdef Q_OS_WIN
    std::thread([] { Beep(1800, 120); }).detach(); // 1800 Hz for 120 ms
#else
    QApplication::beep();
#endif
}
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
    const bool dark = Theme::isDark(); // card colours follow light / dark mode
    QLinearGradient g(body.topLeft(), body.bottomLeft());
    if (dark)
    {
        g.setColorAt(0, m_alert ? Theme::themed("#3a2230") : Theme::themed("#1f2a47"));
        g.setColorAt(1, m_alert ? Theme::themed("#2e1c28") : Theme::themed("#1a233d"));
    }
    else
    {
        g.setColorAt(0, Theme::themed("#ffffff"));
        g.setColorAt(1, m_alert ? Theme::themed("#fff2f0") : Theme::themed("#f4f7ff"));
    }
    p.setBrush(g);
    p.setPen(QPen(m_alert ? QColor(255, 140, 130, 190) : (dark ? QColor(90, 120, 200, 70) : QColor(40, 70, 150, 34)), 1));
    p.drawRoundedRect(body.adjusted(.5, .5, -.5, -.5), 14, 14);
    // lit top edge for the "raised" look
    p.setPen(QPen(dark ? QColor(140, 170, 255, 60) : QColor(255, 255, 255, 240), 1.2));
    p.drawLine(QPointF(body.left() + 14, body.top() + 1.6), QPointF(body.right() - 14, body.top() + 1.6));
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
            scanBeep();
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
    p.fillRect(r, Theme::themed("#162644"));
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
        p.setPen(Theme::themed("#c2d0ef"));
#ifdef WITH_CAMERA
        p.drawText(r, Qt::AlignCenter, "Camera stopped — press Start camera");
#else
        p.drawText(r.adjusted(15, 0, -15, 0), Qt::AlignCenter | Qt::TextWordWrap,
                   "Camera unavailable in this build.\nInstall the Qt Multimedia component, then re-run CMake.\nTyped barcode lookup still works.");
#endif
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
    const QRectF area(66, 12 + depth, width() - 66 - 10 - depth, height() - 12 - depth - 24);
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
        p.setPen(QPen(Theme::themed("#e1e8f8"), 1, g == 0 ? Qt::SolidLine : Qt::DashLine));
        p.drawLine(QPointF(area.left(), y), QPointF(area.right() + depth, y));
        p.setPen(Theme::outline);
        p.drawText(QRectF(0, y - 8, 60, 16), Qt::AlignRight | Qt::AlignVCenter, QString("PKR %1").arg(int(maxV * g / 4)));
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
        const QColor c1 = hot ? Theme::themed("#7fb0ff") : Theme::themed("#7aa6f7");
        const QColor c2 = hot ? Theme::themed("#2457e6") : Theme::themed("#3f6fe0");
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
            p.drawText(QRectF(x - 30, y - depth - 18, bw + depth + 60, 16), Qt::AlignCenter, QString("PKR %1").arg(int(m_actual[i])));
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
        p.setPen(QPen(Theme::themed("#f08a24"), 2, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
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
