#pragma once
// ProductValidation: rules that check a Product's data is sane (ProductValidator)
// and checks that an order quantity can be met from stock (StockValidator).
#include "ProductModel.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// ---------- Product data validation ----------
struct ValidationReport
{
    std::vector<std::string> errors;   // line is rejected
    std::vector<std::string> warnings; // line is kept, but flagged
    bool ok() const { return errors.empty(); }
};

class IProductRule
{
public:
    virtual ~IProductRule() = default;
    virtual void check(const Product &p, ValidationReport &r) const = 0;
};

class ProductIdRule : public IProductRule
{
public:
    void check(const Product &p, ValidationReport &r) const override;
};

class BarcodeRule : public IProductRule
{
public:
    void check(const Product &p, ValidationReport &r) const override;
};

class AcronymRule : public IProductRule
{
public:
    explicit AcronymRule(std::size_t maxLength = 10) : maxLength_(maxLength) {}
    void check(const Product &p, ValidationReport &r) const override;

private:
    std::size_t maxLength_;
};

class ProductNameRule : public IProductRule
{
public:
    void check(const Product &p, ValidationReport &r) const override;
};

class PriceRule : public IProductRule
{
public:
    void check(const Product &p, ValidationReport &r) const override;
};

class QuantityRule : public IProductRule
{
public:
    void check(const Product &p, ValidationReport &r) const override;
};

class BarcodePathRule : public IProductRule
{
public:
    void check(const Product &p, ValidationReport &r) const override;
};

class ProductValidator
{
public:
    static ProductValidator standard(); // all built-in rules
    ProductValidator &addRule(std::unique_ptr<IProductRule> rule);
    ValidationReport validate(const Product &product) const;

private:
    std::vector<std::unique_ptr<IProductRule>> rules_;
};

// ---------- Stock checks ----------
enum class StockStatus
{
    Ok,
    InvalidQuantity,
    OutOfStock,
    InsufficientStock
};

struct StockCheck
{
    StockStatus status = StockStatus::Ok;
    int requested = 0;
    int available = 0;
    std::string message;
    bool ok() const { return status == StockStatus::Ok; }
};

// Decides whether a requested quantity is allowed given what's in stock.
class IStockPolicy
{
public:
    virtual ~IStockPolicy() = default;
    virtual bool allows(int requested, int available) const = 0;
    virtual std::string describe() const = 0;
};

// Default policy: you can't order more than is in stock.
class StrictStockPolicy : public IStockPolicy
{
public:
    bool allows(int requested, int available) const override { return requested <= available; }
    std::string describe() const override { return "cannot exceed available stock"; }
};

class StockValidator
{
public:
    explicit StockValidator(std::shared_ptr<const IStockPolicy> policy = std::make_shared<StrictStockPolicy>())
        : policy_(std::move(policy)) {}

    StockCheck check(const Product &p, int requested) const;
    StockCheck checkAdd(const Product &p, int alreadyInOrder, int toAdd) const;

private:
    std::shared_ptr<const IStockPolicy> policy_;
};
