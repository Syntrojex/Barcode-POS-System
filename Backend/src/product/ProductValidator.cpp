#include "product/ProductValidator.h"
#include "product/Ean8.h"

void ProductIdRule::check(const Product &p, ValidationReport &r) const
{
    if (p.getProductID() <= 0)
        r.errors.push_back("product id must be a positive number");
}

void BarcodeRule::check(const Product &p, ValidationReport &r) const
{
    const std::string &b = p.getBarcode();
    if (b.empty())
    {
        r.errors.push_back("barcode is empty");
        return;
    }
    if (!ean8::isEightDigits(b))
    {
        r.errors.push_back("barcode '" + b + "' must be exactly 8 digits (EAN-8)");
        return;
    }
    if (!ean8::hasValidCheckDigit(b))
        r.warnings.push_back("barcode '" + b + "' has an invalid EAN-8 check digit");
}

void AcronymRule::check(const Product &p, ValidationReport &r) const
{
    if (p.getAcronym().empty())
        r.errors.push_back("acronym is empty");
    else if (p.getAcronym().size() > maxLength_)
        r.errors.push_back("acronym '" + p.getAcronym() + "' is longer than " + std::to_string(maxLength_) +
                           " characters");
}

void ProductNameRule::check(const Product &p, ValidationReport &r) const
{
    if (p.getProductName().empty())
        r.errors.push_back("product name is empty");
}

void PriceRule::check(const Product &p, ValidationReport &r) const
{
    if (p.getUnitPrice() < 0)
        r.errors.push_back("unit price cannot be negative");
    if (p.getTax() < 0)
        r.errors.push_back("tax cannot be negative");
    if (p.getDiscount() < 0)
        r.errors.push_back("discount cannot be negative");
    if (p.getDiscount() > p.getUnitPrice())
        r.errors.push_back("discount cannot be greater than the unit price");
}

void QuantityRule::check(const Product &p, ValidationReport &r) const
{
    if (p.getAvailableQuantity() < 0)
        r.errors.push_back("available quantity cannot be negative");
}

void BarcodePathRule::check(const Product &p, ValidationReport &r) const
{
    if (p.getBarcodeFilePath().empty())
        r.errors.push_back("barcode file path is empty");
}

ProductValidator ProductValidator::standard()
{
    ProductValidator v;
    v.addRule(std::make_unique<ProductIdRule>())
        .addRule(std::make_unique<BarcodeRule>())
        .addRule(std::make_unique<AcronymRule>())
        .addRule(std::make_unique<ProductNameRule>())
        .addRule(std::make_unique<PriceRule>())
        .addRule(std::make_unique<QuantityRule>())
        .addRule(std::make_unique<BarcodePathRule>());
    return v;
}

ProductValidator &ProductValidator::addRule(std::unique_ptr<IProductRule> rule)
{
    rules_.push_back(std::move(rule));
    return *this;
}

ValidationReport ProductValidator::validate(const Product &product) const
{
    ValidationReport report;
    for (const auto &rule : rules_)
        rule->check(product, report);
    return report;
}
