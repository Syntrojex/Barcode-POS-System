#pragma once
// Inventory Management (manager): stock levels for every product, stage quantity changes, commit to Product.txt.
#include "AppState.h"
#include <QMap>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

class InventoryPage : public QWidget
{
    Q_OBJECT
public:
    explicit InventoryPage(QWidget *parent = nullptr);
    void refresh();
signals:
    void toast(const QString &message);
    void newProduct(); // "New SKU Entry"

private:
    void rebuild();
    void updateBar();
    void save();
    void discard();
    QLineEdit *m_search = nullptr;
    QComboBox *m_category = nullptr;
    QCheckBox *m_lowOnly = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_total = nullptr, *m_totalSub = nullptr, *m_low = nullptr, *m_lowSub = nullptr, *m_out = nullptr,
           *m_outSub = nullptr, *m_footer = nullptr, *m_barText = nullptr;
    QFrame *m_bar = nullptr;
    QPushButton *m_saveTop = nullptr;
    QMap<int, int> m_edits; // product id -> staged quantity
    bool m_building = false;
};
