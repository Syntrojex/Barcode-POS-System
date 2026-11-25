#pragma once
// Small reusable widgets and helpers shared by all pages.
#include <QFrame>
#include <QImage>
#include <QMouseEvent>
#include <QElapsedTimer>
#include <memory>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVector>
#include <QWidget>

class QHBoxLayout;
class QVBoxLayout;

// Rounded, theme-aware panel with an inset surface and a subtle shadow.
class Card : public QFrame
{
public:
    explicit Card(QWidget *parent = nullptr, bool alert = false);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    bool m_alert;
};

namespace W
{
    // Re-usable page layout: consistent outer margins, no gaps (cards carry their own shadow space).
    QVBoxLayout *pageLayout(QWidget *page);
    // Round gradient icon tile used in stat cards.
    QLabel *iconTile(const QString &icon, const QColor &fg, const QString &gradTop, const QString &gradBottom,
                     int size = 38);
    QFrame *card(QWidget *parent = nullptr, const char *objectName = "card");
    QLabel *label(const QString &text, const char *objectName = nullptr, QWidget *parent = nullptr);
    QLabel *badge(const QString &text, const QString &kind, QWidget *parent = nullptr);
    void setBadge(QLabel *l, const QString &text, const QString &kind);
    // kind: primary | soft | danger | flat | tab | mini | step | icon
    QPushButton *button(const QString &text, const QString &kind = "", const QString &icon = "",
                        QWidget *parent = nullptr);
    QLabel *iconLabel(const QString &icon, const QColor &color, int px = 18, QWidget *parent = nullptr);
    QLabel *dot(const QColor &color, QWidget *parent = nullptr);
    QFrame *hline(QWidget *parent = nullptr);
    QString money(double v, bool sign = false);
    QString statusBadgeKind(const QString &status);
    // Section heading row: [icon] Title  ......  right-widget
    QHBoxLayout *headerRow(QVBoxLayout *into, const QString &icon, const QString &title,
                           QWidget *rightWidget = nullptr);
}

class QVideoFrame;

// Live camera feed (Qt Multimedia) with barcode detection by the pure-C++ Decoder.
// Without Qt Multimedia the widget shows a clear "camera unavailable" message.
class CameraFeedWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CameraFeedWidget(QWidget *parent = nullptr);
    ~CameraFeedWidget() override;
    bool running() const { return m_running; }
    void setRunning(bool on);
    void setCode(const QString &code); // barcode digits shown in the viewfinder
    // Green confirmation banner on the live picture (e.g. "40001029 - Olpers Milk"); fades after ~3 s.
    void showResult(const QString &text, bool ok = true);
signals:
    void barcodeDetected(const QString &ean);
    void scanError(const QString &error);

public:
    QSize sizeHint() const override { return {420, 236}; }
    QSize minimumSizeHint() const override { return {260, 150}; }

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    bool nearLine(const QPoint &p) const;
    void moveLine(const QPoint &p);
    double m_lineFrac = 0.5; // aiming line height, 0..1 of the picture (drag it with the mouse)
    bool m_drag = false;
    double m_sweepPhase = 0;
    QString m_result;
    bool m_resultOk = true;
    QElapsedTimer m_resultClock;
    bool m_running = false;
    QString m_code;
    QString m_lastDetected;
    QElapsedTimer m_lastEmit; // last time any barcode was seen in the picture
    bool m_armed = true;      // false after a read, until the barcode leaves the picture
    QImage m_frame;
    QRectF m_hitBox;       // where the barcode was found, in camera-picture pixels
    QString m_hitText;     // the code that was read
    QElapsedTimer m_hitClock;
    QTimer m_timer;
#ifdef WITH_CAMERA
    struct CameraRig;
    void onImage(const QImage &image); // show + decode one camera picture
    std::unique_ptr<CameraRig> m_rig;
    QElapsedTimer m_decodeClock;
#endif
};

// Hourly revenue columns and an optional target line.
class RevenueChart : public QWidget
{
    Q_OBJECT
public:
    explicit RevenueChart(QWidget *parent = nullptr);
    void setData(const QStringList &labels, const QVector<double> &actual, const QVector<double> &target);
    QSize minimumSizeHint() const override { return {300, 190}; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QStringList m_labels;
    QVector<double> m_actual, m_target;
};

// "-  2  +" quantity control.
class QtyStepper : public QWidget
{
    Q_OBJECT
public:
    explicit QtyStepper(int value, QWidget *parent = nullptr);
signals:
    void minus();
    void plus();
};
