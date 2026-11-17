#include "ProductValidator.h"

#include <algorithm>
#include <cctype>

using Msg = std::optional<std::string>;

Msg PositiveIdRule::check(const Product &p) const
{
    return p.getProductID() > 0 ? Msg{} : Msg{"product id must be greater than 0"};
}

Msg BarcodeRequiredRule::check(const Product &p) const
{
    return p.getBarcode().empty() ? Msg{"barcode is empty"} : Msg{};
}

Msg BarcodeDigitsRule::check(const Product &p) const
{
    const std::string &b = p.getBarcode();
    const bool digits = std::all_of(b.begin(), b.end(), [](unsigned char c)
                                    { return std::isdigit(c) != 0; });
    if (b.size() != length_ || !digits)
        return "barcode '" + b + "' must be exactly " + std::to_string(length_) + " digits";
    return {};
}

Msg AcronymRule::check(const Product &p) const
{
    if (p.getAcronym().empty())
        return "acronym is empty";
    if (p.getAcronym().size() > max_)
        return "acronym '" + p.getAcronym() + "' is longer than " + std::to_string(max_) + " characters";
    return {};
}

Msg NameRequiredRule::check(const Product &p) const
{
    return p.getProductName().empty() ? Msg{"product name is empty"} : Msg{};
}

Msg MeasurementUnitRule::check(const Product &p) const
{
    const std::string &u = p.getMeasurementUnit();
    if (u != "count" && u != "weight")
        return "measurement unit '" + u + "' must be 'count' or 'weight'";
    return {};
}

Msg AmountsRule::check(const Product &p) const
{
    if (p.getUnitPrice() < 0)
        return "unit price cannot be negative";
    if (p.getTax() < 0)
        return "tax cannot be negative";
    if (p.getDiscount() < 0)
        return "discount cannot be negative";
    if (p.getDiscount() > p.getUnitPrice())
        return "discount cannot be greater than the unit price";
    return {};
}

Msg QuantityRule::check(const Product &p) const
{
    return p.getAvailableQuantity() < 0 ? Msg{"available quantity cannot be negative"} : Msg{};
}

Msg BarcodeFilePathRule::check(const Product &p) const
{
    return p.getBarcodeFilePath().empty() ? Msg{"barcode file path is empty"} : Msg{};
}

ProductValidator ProductValidator::withDefaultRules()
{
    ProductValidator v;
    v.addRule(std::make_unique<PositiveIdRule>());
    v.addRule(std::make_unique<BarcodeRequiredRule>());
    v.addRule(std::make_unique<AcronymRule>());
    v.addRule(std::make_unique<NameRequiredRule>());
    v.addRule(std::make_unique<MeasurementUnitRule>());
    v.addRule(std::make_unique<AmountsRule>());
    v.addRule(std::make_unique<QuantityRule>());
    v.addRule(std::make_unique<BarcodeFilePathRule>());
    return v;
}

void ProductValidator::addRule(std::unique_ptr<IProductRule> rule)
{
    rules_.push_back(std::shared_ptr<const IProductRule>(std::move(rule)));
}

std::vector<std::string> ProductValidator::validate(const Product &p) const
{
    std::vector<std::string> errors;
    for (const auto &rule : rules_)
        if (auto e = rule->check(p))
            errors.push_back(*e);
    return errors;
}
