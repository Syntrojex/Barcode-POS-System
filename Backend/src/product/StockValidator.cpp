#include "product/StockValidator.h"

StockCheck StockValidator::check(const Product &p, int requested) const
{
    StockCheck c;
    c.requested = requested;
    c.available = p.getAvailableQuantity();
    if (requested <= 0)
    {
        c.status = StockStatus::InvalidQuantity;
        c.message = "quantity must be at least 1";
    }
    else if (c.available <= 0)
    {
        c.status = StockStatus::OutOfStock;
        c.message = p.getProductName() + " is out of stock";
    }
    else if (!policy_->allows(requested, c.available))
    {
        c.status = StockStatus::InsufficientStock;
        c.message = "only " + std::to_string(c.available) + " of " + p.getProductName() + " available (" +
                    policy_->describe() + ")";
    }
    return c;
}

StockCheck StockValidator::checkAdd(const Product &p, int alreadyInOrder, int toAdd) const
{
    if (toAdd <= 0)
        return check(p, toAdd);
    return check(p, alreadyInOrder + toAdd);
}
