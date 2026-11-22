#pragma once
// Bills & Invoices: ledger of bills, live receipt inspection, print as PDF.
#include "AppState.h"
#include <QWidget>

class QLineEdit;
class QTableWidget;

class BillsPage : public QWidget
{
    Q_OBJECT
public:
    explicit BillsPage(QWidget *parent = nullptr);
    void showOrder(const QString &orderId); // select this order's bill in the ledger / show its preview
signals:
    void toast(const QString &message);
    void newOrderRequested(); // "New Order [Space]" on the receipt screen

private:
    void rebuild();
    QLineEdit *m_search = nullptr;
    QTableWidget *m_table = nullptr;
};
