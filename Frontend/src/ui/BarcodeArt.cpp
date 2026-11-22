#include "BarcodeArt.h"
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QPainter>

namespace
{
    // EAN-8: left half uses the "L" codes, right half the "R" codes (R = bitwise complement of L).
    const char *kL[10] = {"0001101", "0011001", "0010011", "0111101", "0100011",
                          "0110001", "0101111", "0111011", "0110111", "0001011"};
    const char *kR[10] = {"1110010", "1100110", "1101100", "1000010", "1011100",
                          "1001110", "1010000", "1000100", "1001000", "1110100"};
    bool eightDigits(const QString &s)
    {
        if (s.size() != 8)
            return false;
        for (QChar c : s)
            if (!c.isDigit())
                return false;
        return true;
    }
}

namespace BarcodeArt
{
    QString checkDigit(const QString &seven)
    {
        if (seven.size() != 7)
            return QString();
        int sum = 0;
        for (int i = 0; i < 7; ++i)
            sum += seven[i].digitValue() * (i % 2 == 0 ? 3 : 1);
        return QString::number((10 - sum % 10) % 10);
    }

    QString makeCode(const QString &categoryId, int serial)
    {
        if (categoryId.size() != 2 || serial < 0 || serial > 99999)
            return QString();
        const QString seven = categoryId + QString::number(serial).rightJustified(5, '0');
        return seven + checkDigit(seven);
    }

    QString modules(const QString &code)
    {
        if (!eightDigits(code))
            return QString();
        QString m = "101";
        for (int i = 0; i < 4; ++i)
            m += kL[code[i].digitValue()];
        m += "01010";
        for (int i = 4; i < 8; ++i)
            m += kR[code[i].digitValue()];
        m += "101";
        return m;
    }

    QImage render(const QString &code, int moduleWidth, int barHeight, bool withDigits)
    {
        const QString m = modules(code);
        if (m.isEmpty())
            return QImage();
        const int quiet = moduleWidth * 7;
        const int digitsH = withDigits ? moduleWidth * 8 : 0;
        const int w = quiet * 2 + m.size() * moduleWidth;
        const int h = quiet / 2 + barHeight + digitsH + quiet / 2;
        QImage img(w, h, QImage::Format_RGB32);
        img.fill(Qt::white);
        QPainter p(&img);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::black);
        for (int i = 0; i < m.size(); ++i)
            if (m[i] == '1')
                p.drawRect(quiet + i * moduleWidth, quiet / 2, moduleWidth, barHeight);
        if (withDigits)
        {
            QFont f("Courier New");
            f.setStyleHint(QFont::Monospace);
            f.setPixelSize(moduleWidth * 6);
            p.setFont(f);
            p.setPen(Qt::black);
            p.drawText(QRect(0, quiet / 2 + barHeight, w, digitsH + quiet / 3), Qt::AlignHCenter | Qt::AlignVCenter, code);
        }
        return img;
    }

    bool savePng(const QString &code, const QString &path, QString *error)
    {
        const QImage img = render(code);
        if (img.isNull())
        {
            if (error) *error = "Barcode must be exactly 8 digits.";
            return false;
        }
        QDir().mkpath(QFileInfo(path).absolutePath());
        if (!img.save(path, "PNG"))
        {
            if (error) *error = "Could not write " + path;
            return false;
        }
        return true;
    }
}
