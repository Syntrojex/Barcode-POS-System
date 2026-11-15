#pragma once
// Small reusable widgets and helpers shared by all pages.
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVector>
#include <QWidget>

class QHBoxLayout;
class QVBoxLayout;

namespace W
{
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

// Simulated optical scanner feed. Draws an animated placeholder; no camera or OpenCV needed.
class CameraFeedWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CameraFeedWidget(QWidget *parent = nullptr);
    bool running() const { return m_running; }
    void setRunning(bool on);
    void setCode(const QString &code); // barcode digits shown in the viewfinder
    QSize sizeHint() const override { return {420, 236}; }
    QSize minimumSizeHint() const override { return {260, 150}; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    bool m_running = true;
    qreal m_phase = 0;
    QString m_code = "40088404";
    QTimer m_timer;
};

// Line chart: actual revenue (area + line) against a dashed target.
class RevenueChart : public QWidget
{
    Q_OBJECT
public:
    explicit RevenueChart(QWidget *parent = nullptr);
    void setData(const QStringList &labels, const QVector<double> &actual, const QVector<double> &target);
    QSize minimumSizeHint() const override { return {300, 170}; }

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
