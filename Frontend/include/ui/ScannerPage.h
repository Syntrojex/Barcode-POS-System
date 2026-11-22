#pragma once
#include "AppState.h"
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QListWidget;
class CameraFeedWidget;

class ScannerPage : public QWidget
{
    Q_OBJECT
public:
    explicit ScannerPage(QWidget *parent = nullptr);
    void setViewOnly(bool on); // hide "Send to order", keep Description
signals:
    void toast(const QString &message);
    void sendToOrder(const QString &ean);
    void described(int productId);

private:
    void scanNext();
    void show(const ProductInfo &p);
    CameraFeedWidget *m_feed = nullptr;
    QListWidget *m_history = nullptr;
    QLabel *m_last = nullptr;
    QPushButton *m_toggle = nullptr, *m_send = nullptr, *m_describe = nullptr;
    QLineEdit *m_manual = nullptr;
    int m_index = 3;
    QString m_lastEan;
    int m_lastId = 0;
};
