#ifndef STOCK_VALIDATOR_H
#define STOCK_VALIDATOR_H

#include "Product.h"

#include <string>

enum class StockStatus
{
    Ok,
    InvalidQuantity, // requested quantity is zero or negative
    OutOfStock,      // nothing available
    Insufficient,    // requested more than is available
    UnknownProduct   // no such product (used by ProductManager::deductStock)
};

// Decides whether a requested quantity can be sold. Pure function of (product, quantity):
// the Order module calls this before adding a line to a bill.
class StockValidator
{
public:
    static StockStatus check(const Product &p, int requestedQuantity);
    static bool canFulfil(const Product &p, int requestedQuantity)
    {
        return check(p, requestedQuantity) == StockStatus::Ok;
    }
    // Cashier-facing text, e.g. "Only 3 available".
    static std::string message(const Product &p, int requestedQuantity);
};

#endif
