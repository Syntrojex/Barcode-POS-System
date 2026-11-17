#ifndef PRODUCT_VALIDATOR_H
#define PRODUCT_VALIDATOR_H

#include "Product.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

// One business rule about a Product. Add a new rule by writing a new class: ProductValidator
// itself never has to change (open/closed).
class IProductRule
{
public:
    virtual ~IProductRule() = default;
    // Returns an error message, or std::nullopt when the product satisfies the rule.
    virtual std::optional<std::string> check(const Product &p) const = 0;
};

// Built-in rules ----------------------------------------------------------------------
class PositiveIdRule : public IProductRule
{
public:
    std::optional<std::string> check(const Product &p) const override;
};
class BarcodeRequiredRule : public IProductRule
{
public:
    std::optional<std::string> check(const Product &p) const override;
};
// Optional (not in the default set): barcode must be exactly `length` digits, e.g. 8 for the
// Increment 1 generator format.
class BarcodeDigitsRule : public IProductRule
{
public:
    explicit BarcodeDigitsRule(std::size_t length) : length_(length) {}
    std::optional<std::string> check(const Product &p) const override;

private:
    std::size_t length_;
};
class AcronymRule : public IProductRule // 1..8 characters
{
public:
    explicit AcronymRule(std::size_t maxLength = 8) : max_(maxLength) {}
    std::optional<std::string> check(const Product &p) const override;

private:
    std::size_t max_;
};
class NameRequiredRule : public IProductRule
{
public:
    std::optional<std::string> check(const Product &p) const override;
};
class MeasurementUnitRule : public IProductRule // "count" or "weight"
{
public:
    std::optional<std::string> check(const Product &p) const override;
};
class AmountsRule : public IProductRule // price/tax/discount >= 0, discount <= price
{
public:
    std::optional<std::string> check(const Product &p) const override;
};
class QuantityRule : public IProductRule // available quantity >= 0
{
public:
    std::optional<std::string> check(const Product &p) const override;
};
class BarcodeFilePathRule : public IProductRule
{
public:
    std::optional<std::string> check(const Product &p) const override;
};

// Runs a list of rules and reports every violation.
class ProductValidator
{
public:
    // The rules required by the Increment 2 specification.
    static ProductValidator withDefaultRules();

    void addRule(std::unique_ptr<IProductRule> rule);
    std::vector<std::string> validate(const Product &p) const;
    bool isValid(const Product &p) const { return validate(p).empty(); }

private:
    std::vector<std::shared_ptr<const IProductRule>> rules_; // shared: validators are cheap to copy
};

#endif
