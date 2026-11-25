#pragma once
// Seasonal discounts (manager), requirements 13-15: seasons in Seasons.txt (start date, end date, discount
// percentage, isActive), added through this page and switched active / inactive.
#include <QWidget>

class QCheckBox;
class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QFrame;

class DiscountsPage : public QWidget
{
    Q_OBJECT
public:
    explicit DiscountsPage(QWidget *parent = nullptr);
    void refresh();
signals:
    void toast(const QString &message);

private:
    void rebuild();
    void resetForm();
    void edit(const QString &id);
    void submit();
    QFrame *m_banner = nullptr;
    QLabel *m_bannerTitle = nullptr, *m_bannerSub = nullptr, *m_rate = nullptr, *m_rateSub = nullptr,
           *m_upcoming = nullptr, *m_upSub = nullptr, *m_formTitle = nullptr, *m_error = nullptr;
    QTableWidget *m_table = nullptr;
    QString m_filter = "All";
    QVector<QPushButton *> m_tabs;
    QDateEdit *m_start = nullptr, *m_end = nullptr;
    QDoubleSpinBox *m_percent = nullptr;
    QCheckBox *m_active = nullptr;
    QPushButton *m_save = nullptr;
    QString m_editing;
};
