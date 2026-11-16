#pragma once
// Orders, Products, Bills, Barcode Scanner and Settings screens.
#include "AppState.h"
#include <QWidget>

class QLabel;
class QLineEdit;
class QTableWidget;
class QPushButton;
class CameraFeedWidget;
class QListWidget;

class OrdersPage : public QWidget
{
    Q_OBJECT
public:
    explicit OrdersPage(QWidget *parent = nullptr);
    void setSearch(const QString &text);
signals:
    void toast(const QString &message);
    void resumeOrder(const QString &orderId);

private:
    void rebuild();
    QLineEdit *m_orderSearch, *m_custSearch;
    QTableWidget *m_table;
    QString m_filter = "All";
    QVector<QPushButton *> m_tabs;
    QLabel *m_showing;
};

class ProductsPage : public QWidget
{
    Q_OBJECT
public:
    explicit ProductsPage(QWidget *parent = nullptr);
    void setSearch(const QString &text);
signals:
    void toast(const QString &message);
    void addToOrder(const QString &ean);

private:
    void rebuild();
    QLineEdit *m_search;
    QTableWidget *m_table;
    QLabel *m_count;
};

class BillsPage : public QWidget
{
    Q_OBJECT
public:
    explicit BillsPage(QWidget *parent = nullptr);
signals:
    void toast(const QString &message);

private:
    void rebuild();
    QLineEdit *m_search;
    QTableWidget *m_table;
};

class ScannerPage : public QWidget
{
    Q_OBJECT
public:
    explicit ScannerPage(QWidget *parent = nullptr);
signals:
    void toast(const QString &message);
    void sendToOrder(const QString &ean);

private:
    void scanNext();
    CameraFeedWidget *m_feed;
    QListWidget *m_history;
    QLabel *m_last;
    QPushButton *m_toggle, *m_send;
    QLineEdit *m_manual;
    int m_index = 3;
    QString m_lastEan;
};

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(QWidget *parent = nullptr);
signals:
    void toast(const QString &message);
};
