#include "Pricing.h"
#include "UiStore.h"
#include <QtMath>

namespace
{
    double round2(double v) { return qRound64(v * 100.0) / 100.0; }

    // Category id of the product behind a bill line (first two digits of its barcode); empty for manual lines.
    QString categoryOfLine(const ReceiptLine &l)
    {
        if (l.productId <= 0)
            return QString();
        for (const auto &p : AppState::instance().products())
            if (p.id == l.productId)
                return p.ean.left(2);
        return QString();
    }

    bool seasonCovers(const Season &s, const ReceiptLine &l)
    {
        if (s.categories.isEmpty())
            return true;
        return s.categories.contains(categoryOfLine(l));
    }
}

namespace Pricing
{
    double clampSpecial(double percent)
    {
        if (percent < 0)
            return 0;
        return qMin(percent, UiStore::kSpecialDiscountCapPercent);
    }

    Quote quote(const QVector<ReceiptLine> &lines, double specialPercent, const QDate &on)
    {
        Quote q;
        q.specialPercent = clampSpecial(specialPercent);
        const Season *season = UiStore::instance().activeSeason(on);
        if (season)
        {
            q.seasonalPercent = season->percent;
            q.seasonName = season->name;
        }
        for (const auto &l : lines)
        {
            const double base = l.price - l.discount; // per unit, after the product's own discount
            double sUnit = 0;
            if (season && seasonCovers(*season, l))
                sUnit = round2(base * season->percent / 100.0);
            const double xUnit = round2((base - sUnit) * q.specialPercent / 100.0);
            q.gross += l.price * l.qty;
            q.itemDiscount += l.discount * l.qty;
            q.seasonal += sUnit * l.qty;
            q.special += xUnit * l.qty;
            q.tax += l.tax * l.qty;
            q.units += l.qty;
            ++q.lines;
        }
        q.payable = q.gross - q.itemDiscount - q.seasonal - q.special + q.tax;
        return q;
    }

    QVector<ReceiptLine> fold(const QVector<ReceiptLine> &lines, double specialPercent, const QDate &on)
    {
        const double sp = clampSpecial(specialPercent);
        const Season *season = UiStore::instance().activeSeason(on);
        QVector<ReceiptLine> out = lines;
        for (auto &l : out)
        {
            const double base = l.price - l.discount;
            double sUnit = 0;
            if (season && seasonCovers(*season, l))
                sUnit = round2(base * season->percent / 100.0);
            const double xUnit = round2((base - sUnit) * sp / 100.0);
            l.discount = round2(l.discount + sUnit + xUnit);
        }
        return out;
    }

    void apply(OrderRecord &o, const Quote &q)
    {
        o.subtotal = q.gross;
        o.discount = q.totalDiscount();
        o.tax = q.tax;
        o.total = q.payable;
        o.seasonal = q.seasonal;
        o.special = q.special;
        o.seasonalPercent = q.seasonalPercent;
        o.specialPercent = q.specialPercent;
        o.seasonName = q.seasonName;
    }
}
