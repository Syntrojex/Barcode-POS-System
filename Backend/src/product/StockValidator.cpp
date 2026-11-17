#include "StockValidator.h"

StockStatus StockValidator::check(const Product &p, int requested)
{
    if (requested <= 0)
        return StockStatus::InvalidQuantity;
    if (p.getAvailableQuantity() <= 0)
        return StockStatus::OutOfStock;
    if (requested > p.getAvailableQuantity())
        return StockStatus::Insufficient;
    return StockStatus::Ok;
}

std::string StockValidator::message(const Product &p, int requested)
{
    switch (check(p, requested))
    {
    case StockStatus::Ok:
        return "In stock";
    case StockStatus::InvalidQuantity:
        return "Quantity must be at least 1";
    case StockStatus::OutOfStock:
        return p.getProductName() + " is out of stock";
    case StockStatus::UnknownProduct:
        return "Product not found";
    case StockStatus::Insufficient:
        return "Only " + std::to_string(p.getAvailableQuantity()) + " available (requested " +
               std::to_string(requested) + ")";
    }
    return {};
}
