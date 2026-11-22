#include "UiKit.h"
#include "Theme.h"
#include "Widgets.h"
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QShowEvent>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

namespace Ui
{
    QTableWidget *makeTable(const QStringList &headers)
    {
        auto *t = new QTableWidget(0, headers.size());
        t->setHorizontalHeaderLabels(headers);
        t->verticalHeader()->hide();
        t->setShowGrid(false);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setSelectionMode(QAbstractItemView::SingleSelection);
        t->setFocusPolicy(Qt::NoFocus);
        t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        t->horizontalHeader()->setHighlightSections(false);
        t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        t->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        return t;
    }

    QTableWidgetItem *item(const QString &s, Qt::Alignment a, const QString &color, bool bold)
    {
        auto *i = new QTableWidgetItem(s);
        i->setTextAlignment(a);
        if (!color.isEmpty())
            i->setForeground(Theme::readable(QColor(color)));
        if (bold)
        {
            QFont f = i->font();
            f.setBold(true);
            i->setFont(f);
        }
        return i;
    }

    QWidget *wrapW(QWidget *w, Qt::Alignment a)
    {
        auto *c = new QWidget;
        auto *h = new QHBoxLayout(c);
        h->setContentsMargins(6, 0, 6, 0);
        h->setAlignment(a | Qt::AlignVCenter);
        h->addWidget(w);
        return c;
    }

    QString csvField(const QString &s)
    {
        if (!s.contains(',') && !s.contains('"') && !s.contains('\n'))
            return s;
        QString q = s;
        q.replace('"', "\"\"");
        return '"' + q + '"';
    }

    bool writeCsv(QWidget *parent, const QString &suggested, const QStringList &rows)
    {
        const QString path = QFileDialog::getSaveFileName(parent, "Export CSV", suggested, "CSV (*.csv)");
        if (path.isEmpty())
            return false;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;
        QTextStream out(&f);
        for (const auto &r : rows)
            out << r << '\n';
        return true;
    }

    QWidget *pageShell(QWidget *page, QVBoxLayout *&body)
    {
        body = W::pageLayout(page);
        return page;
    }

    QWidget *sectionHeader(const QString &eyebrow, const QString &title, const QString &subtitle,
                           const QList<QWidget *> &actions)
    {
        auto *w = new QWidget;
        w->setAttribute(Qt::WA_StyledBackground, false);
        auto *h = new QHBoxLayout(w);
        h->setContentsMargins(4, 4, 4, 4);
        h->setSpacing(10);
        auto *v = new QVBoxLayout;
        v->setSpacing(1);
        if (!eyebrow.isEmpty())
            v->addWidget(W::label(eyebrow, "eyebrow"));
        v->addWidget(W::label(title, "pageH1"));
        if (!subtitle.isEmpty())
        {
            auto *s = W::label(subtitle, "pageDesc");
            s->setWordWrap(true);
            v->addWidget(s);
        }
        h->addLayout(v, 1);
        for (QWidget *a : actions)
            h->addWidget(a, 0, Qt::AlignVCenter);
        return w;
    }

    QFrame *statTile(const QString &caption, const QString &icon, QLabel *&value, QLabel *&sub, const QString &tint)
    {
        auto *c = W::card(nullptr);
        auto *h = new QHBoxLayout(c);
        h->setContentsMargins(16, 12, 16, 12);
        h->setSpacing(10);
        auto *v = new QVBoxLayout;
        v->setSpacing(2);
        v->addWidget(W::label(caption, "caption"));
        value = W::label("", "bigValue");
        sub = W::label("", "muted");
        v->addWidget(value);
        v->addWidget(sub);
        h->addLayout(v, 1);
        QString top = "#eaf1ff", bottom = "#d9e6ff";
        QColor fg = Theme::primary;
        if (tint == "green") { top = "#e2fbec"; bottom = "#c9f3d9"; fg = Theme::tertiary; }
        else if (tint == "amber") { top = "#fff4d6"; bottom = "#ffe8a8"; fg = QColor("#8a5a00"); }
        else if (tint == "red") { top = "#ffe9e6"; bottom = "#ffd0ca"; fg = Theme::error; }
        h->addWidget(W::iconTile(icon, fg, top, bottom, 38), 0, Qt::AlignTop);
        return c;
    }

    QLabel *kbd(const QString &text)
    {
        auto *l = new QLabel(text);
        l->setObjectName("kbd");
        l->setAlignment(Qt::AlignCenter);
        return l;
    }

    QLabel *dotLabel(const QString &text, const QColor &dot)
    {
        auto *l = new QLabel(QString("<span style='color:%1'>&#9679;</span>&nbsp;%2").arg(dot.name(), text));
        l->setTextFormat(Qt::RichText);
        return l;
    }

    QPushButton *pageButton(const QString &text, bool current)
    {
        auto *b = W::button(text, current ? "primary" : "tab");
        b->setFixedHeight(32);
        return b;
    }

    // ------------------------------------------------------------------ Modal
    Modal::Modal(QWidget *parent, const QString &icon, const QString &title, const QString &subtitle,
                 const QString &tag, int width)
        : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setModal(true);
        auto *outer = new QVBoxLayout(this);
        outer->setContentsMargins(14, 10, 14, 18); // room for the card's shadow
        auto *card = new Card;
        card->setObjectName("modalCard");
        outer->addWidget(card);
        auto *cv = new QVBoxLayout(card);
        cv->setContentsMargins(0, 0, 0, 0);
        cv->setSpacing(0);

        auto *head = new QFrame;
        head->setObjectName("modalHead");
        auto *hh = new QHBoxLayout(head);
        hh->setContentsMargins(20, 16, 16, 14);
        hh->setSpacing(12);
        hh->addWidget(W::iconTile(icon, QColor("white"), "#4a86ff", "#2554d8", 44), 0, Qt::AlignTop);
        auto *tv = new QVBoxLayout;
        tv->setSpacing(1);
        auto *tr = new QHBoxLayout;
        tr->setSpacing(8);
        m_title = W::label(title, "modalTitle");
        tr->addWidget(m_title);
        if (!tag.isEmpty())
            tr->addWidget(W::badge(tag, "blue"));
        tr->addStretch();
        tv->addLayout(tr);
        if (!subtitle.isEmpty())
        {
            auto *s = W::label(subtitle, "muted");
            s->setWordWrap(true);
            tv->addWidget(s);
        }
        hh->addLayout(tv, 1);
        auto *x = W::button("", "icon", "close");
        x->setProperty("kind", "flat");
        x->setFixedSize(34, 34);
        x->setToolTip("Close (Esc)");
        connect(x, &QPushButton::clicked, this, &QDialog::reject);
        hh->addWidget(x, 0, Qt::AlignTop);
        cv->addWidget(head);

        auto *bodyW = new QWidget;
        m_body = new QVBoxLayout(bodyW);
        m_body->setContentsMargins(20, 16, 20, 16);
        m_body->setSpacing(12);
        cv->addWidget(bodyW, 1);

        m_footerBar = new QFrame;
        m_footerBar->setObjectName("modalFoot");
        m_footer = new QHBoxLayout(m_footerBar);
        m_footer->setContentsMargins(20, 12, 20, 12);
        m_footer->setSpacing(10);
        cv->addWidget(m_footerBar);

        setFixedWidth(width + 28);
    }

    void Modal::setTitle(const QString &t) { m_title->setText(t); }
    void Modal::hideFooter() { m_footerBar->hide(); }

    void Modal::showEvent(QShowEvent *e)
    {
        QDialog::showEvent(e);
        if (QWidget *host = parentWidget() ? parentWidget()->window() : nullptr)
        {
            if (!m_dim)
            {
                m_dim = new QWidget(host);
                m_dim->setObjectName("dim");
                m_dim->setAttribute(Qt::WA_StyledBackground, true);
                m_dim->setStyleSheet("background: rgba(15,27,51,46%);");
            }
            m_dim->setGeometry(host->rect());
            m_dim->show();
            m_dim->raise();
            raise();
            // centre over the host window
            const QPoint c = host->mapToGlobal(host->rect().center());
            move(c.x() - width() / 2, qMax(host->mapToGlobal(QPoint(0, 0)).y() + 16, c.y() - height() / 2));
        }
    }

    void Modal::hideEvent(QHideEvent *e)
    {
        QDialog::hideEvent(e);
        if (m_dim)
        {
            m_dim->hide();
            m_dim->deleteLater();
            m_dim = nullptr;
        }
    }
}
