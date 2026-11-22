#pragma once
// Draws EAN-8 barcodes (bars + digits) with QPainter. Used to preview a barcode in the product editor and to write
// the data/barcodes/<code>.png file that Product.txt points to. Pure frontend code: the check digit rule is the
// same one the Increment 2 backend validates (Backend/include/product/Ean8.h).
#include <QImage>
#include <QString>

namespace BarcodeArt
{
    // "01001020" -> the 67-module bit pattern ('1' = bar). Empty if `code` is not 8 digits.
    QString modules(const QString &code);
    // 7 digits -> the EAN-8 check digit character ("0".."9").
    QString checkDigit(const QString &sevenDigits);
    // Full 8-digit code from category (2 digits) + serial (up to 5 digits).
    QString makeCode(const QString &categoryId, int serial);
    // Same look as the existing data/barcodes PNGs: white background, black bars, digits underneath.
    QImage render(const QString &code, int moduleWidth = 3, int barHeight = 100, bool withDigits = true);
    bool savePng(const QString &code, const QString &path, QString *error = nullptr);
}
