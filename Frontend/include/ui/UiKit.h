#pragma once
// Shared building blocks for the redesigned screens (tables, page headers, stat tiles, design-style modal).
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QStringList>

class QHBoxLayout;
class QTableWidget;
class QTableWidgetItem;
class QVBoxLayout;
class QWidget;
class QDate;
class QDateEdit;

namespace Ui
{
    // ---- tables ----
    QTableWidget *makeTable(const QStringList &headers);
    QTableWidgetItem *item(const QString &s, Qt::Alignment a = Qt::AlignLeft | Qt::AlignVCenter,
                           const QString &color = "", bool bold = false);
    QWidget *wrapW(QWidget *w, Qt::Alignment a = Qt::AlignLeft); // put a widget in a table cell
    bool writeCsv(QWidget *parent, const QString &suggested, const QStringList &rows);
    QString csvField(const QString &s); // quotes a value when it needs it

    // ---- page scaffolding ----
    QWidget *pageShell(QWidget *page, QVBoxLayout *&body, bool scrollable = false);
    QDateEdit *dateEdit(const QDate &date, const QString &displayFormat, int minimumHeight);
    // EYEBROW caption (blue, optional) / big title / description on the left, action widgets on the right.
    QWidget *sectionHeader(const QString &eyebrow, const QString &title, const QString &subtitle,
                           const QList<QWidget *> &actions = {});
    // Stat tile: CAPTION, big value, small sub-line, round icon on the right.
    QFrame *statTile(const QString &caption, const QString &icon, QLabel *&value, QLabel *&sub,
                     const QString &tint = "blue"); // tint: blue | green | amber | red
    QLabel *kbd(const QString &text); // little "Ctrl+S" key hint chip
    QLabel *dotLabel(const QString &text, const QColor &dot); // "● Text" status
    // "Prev  Page 1 of 61  Next" footers are page-specific; this makes just the pill buttons.
    QPushButton *pageButton(const QString &text, bool current = false);

    // ---- modal in the style of the Stitch dialogs ----
    // Frameless rounded card, icon tile + title (+ tag chip) + subtitle + close button, body, footer.
    // Dims the window behind it. Esc closes (rejects).
    class Modal : public QDialog
    {
        Q_OBJECT
    public:
        Modal(QWidget *parent, const QString &icon, const QString &title, const QString &subtitle = QString(),
              const QString &tag = QString(), int width = 720);
        QVBoxLayout *body() { return m_body; }
        QHBoxLayout *footer() { return m_footer; } // add buttons here (left side: addWidget, then addStretch)
        QWidget *footerBar() { return m_footerBar; }
        void setTitle(const QString &t);
        void hideFooter();

    protected:
        void showEvent(QShowEvent *e) override;
        void hideEvent(QHideEvent *e) override;

    private:
        QVBoxLayout *m_body;
        QHBoxLayout *m_footer;
        QWidget *m_footerBar;
        QLabel *m_title;
        QWidget *m_dim = nullptr;
    };
}
