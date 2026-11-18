#pragma once
// Small reusable widgets and helpers shared by all pages.
#include <QFrame>
#include <QImage>
#include <QElapsedTimer>
#include <memory>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVector>
#include <QWidget>

class QHBoxLayout;
class QVBoxLayout;

// Raised panel: soft layered shadow, top-lit gradient and a highlight edge (pure painting, cheap).
// Its contents margins keep children inside the visible body, so layouts need no changes.
class Card : public QFrame
{
public:
    explicit Card(QWidget *parent = nullptr, bool alert = false);

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private:
    QPixmap m_shadow;
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

// Actual OpenCV camera capture when built with CAMERA=ON; otherwise an honest disabled view.
class CameraFeedWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CameraFeedWidget(QWidget *parent = nullptr);
    ~CameraFeedWidget() override;
    bool running() const { return m_running; }
    void setRunning(bool on);
    void setCode(const QString &code); // barcode digits shown in the viewfinder
signals:
    void barcodeDetected(const QString &ean);
    void scanError(const QString &error);

public:
    QSize sizeHint() const override { return {420, 236}; }
    QSize minimumSizeHint() const override { return {260, 150}; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    bool m_running = false;
    QString m_code;
    QString m_lastDetected;
    QElapsedTimer m_lastEmit;
    QImage m_frame;
    QTimer m_timer;
#ifdef WITH_CAMERA
    std::unique_ptr<class cameraScanner> m_camera;
#endif
};

// 3D bar chart: hourly revenue as extruded columns against a dashed target line.
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
