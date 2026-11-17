#include "ProductDescriber.h"

#include "StockValidator.h"

#include <algorithm>
#include <iomanip>
#include <locale>
#include <sstream>

namespace
{
    std::string pkr(double v)
    {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << "PKR " << std::fixed << std::setprecision(2) << v;
        return out.str();
    }
}

std::string ProductDescriber::displayName(const Product &p)
{
    std::string n = p.getProductName();
    std::replace(n.begin(), n.end(), '_', ' ');
    return n;
}

std::vector<ProductDescriber::Field> ProductDescriber::fields(const Product &p)
{
    const bool inStock = StockValidator::canFulfil(p, 1);
    return {
        {"Product ID", std::to_string(p.getProductID())},
        {"Barcode", p.getBarcode()},
        {"Acronym", p.getAcronym()},
        {"Name", displayName(p)},
        {"Measurement unit", p.getMeasurementUnit()},
        {"Unit price", pkr(p.getUnitPrice())},
        {"Tax per unit", pkr(p.getTax())},
        {"Discount per unit", pkr(p.getDiscount())},
        {"Available quantity", std::to_string(p.getAvailableQuantity())},
        {"Stock status", inStock ? "In stock" : "Out of stock"},
        {"Barcode file", p.getBarcodeFilePath()},
    };
}

std::string ProductDescriber::text(const Product &p)
{
    std::string out;
    for (const auto &f : fields(p))
        out += f.first + ": " + f.second + "\n";
    return out;
}
