#pragma once
// Product Categories (manager): 2-digit category id = first two digits of every barcode.
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QHBoxLayout;

class CategoriesPage : public QWidget
{
    Q_OBJECT
public:
    explicit CategoriesPage(QWidget *parent = nullptr);
    void refresh();
signals:
    void toast(const QString &message);

private:
    void rebuild();
    void validate();
    void resetForm();
    void submit();
    void edit(const QString &id);
    QTableWidget *m_table = nullptr;
    QLineEdit *m_id = nullptr, *m_name = nullptr, *m_acr = nullptr, *m_filter = nullptr;
    QLabel *m_error = nullptr, *m_suggest = nullptr, *m_formTitle = nullptr, *m_range = nullptr, *m_summary = nullptr;
    QWidget *m_errorBox = nullptr;
    QPushButton *m_submit = nullptr;
    QString m_editing; // id of the category being edited, empty = adding
};
