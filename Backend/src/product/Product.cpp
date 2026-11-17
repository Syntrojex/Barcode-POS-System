#include "product/Product.h"
#include <stdexcept>

std::string toString(MeasurementUnit unit)
{
    return unit == MeasurementUnit::Count ? "count" : "weight";
}

bool parseMeasurementUnit(const std::string &text, MeasurementUnit &out)
{
    if (text == "count")
        out = MeasurementUnit::Count;
    else if (text == "weight")
        out = MeasurementUnit::Weight;
    else
        return false;
    return true;
}

Product::Product(int id, std::string barcode, std::string acronym, std::string name, MeasurementUnit unit,
                 double unitPrice, double tax, double discount, int availableQuantity,
                 std::string barcodeFilePath)
    : id_(id), barcode_(std::move(barcode)), acronym_(std::move(acronym)), name_(std::move(name)),
      unit_(unit), unitPrice_(unitPrice), tax_(tax), discount_(discount),
      availableQuantity_(availableQuantity), barcodeFilePath_(std::move(barcodeFilePath))
{
}

void Product::deductStock(int qty)
{
    if (qty < 0 || qty > availableQuantity_)
        throw std::invalid_argument("cannot deduct " + std::to_string(qty) + " from stock of " +
                                    std::to_string(availableQuantity_));
    availableQuantity_ -= qty;
}
