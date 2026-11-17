#pragma once
// Data validation for a Product. Open/closed: every check is an IProductRule; new checks are added
// with addRule() and never require editing ProductValidator.
#include "product/Product.h"
#include <memory>
#include <string>
#include <vector>

struct ValidationReport
{
    std::vector<std::string> errors;   // the product must be rejected
    std::vector<std::string> warnings; // the product is usable but suspicious
    bool ok() const { return errors.empty(); }
};

class IProductRule
{
public:
    virtual ~IProductRule() = default;
    virtual void check(const Product &product, ValidationReport &report) const = 0;
};

// ---- the standard rules (one responsibility each) ----
class ProductIdRule : public IProductRule      { public: void check(const Product &, ValidationReport &) const override; };
class BarcodeRule : public IProductRule        { public: void check(const Product &, ValidationReport &) const override; };
class AcronymRule : public IProductRule        // 1..8 characters
{
public:
    explicit AcronymRule(std::size_t maxLength = 8) : maxLength_(maxLength) {}
    void check(const Product &, ValidationReport &) const override;
private:
    std::size_t maxLength_;
};
class ProductNameRule : public IProductRule    { public: void check(const Product &, ValidationReport &) const override; };
class PriceRule : public IProductRule          { public: void check(const Product &, ValidationReport &) const override; };
class QuantityRule : public IProductRule       { public: void check(const Product &, ValidationReport &) const override; };
class BarcodePathRule : public IProductRule    { public: void check(const Product &, ValidationReport &) const override; };

class ProductValidator
{
public:
    static ProductValidator standard();                       // all rules above
    ProductValidator &addRule(std::unique_ptr<IProductRule> rule);
    ValidationReport validate(const Product &product) const;

private:
    std::vector<std::unique_ptr<IProductRule>> rules_;
};
