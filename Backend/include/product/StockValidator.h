#pragma once
// Stock validation. The rule "is this quantity allowed?" is a swappable policy (strategy), so a
// different business rule never touches StockValidator.
#include "product/Product.h"
#include <memory>
#include <string>

class IStockPolicy
{
public:
    virtual ~IStockPolicy() = default;
    virtual bool allows(int requested, int available) const = 0;
    virtual std::string describe() const = 0;
};

// requested <= available  (default: you may sell everything you have)
class UpToAvailablePolicy : public IStockPolicy
{
public:
    bool allows(int requested, int available) const override { return requested <= available; }
    std::string describe() const override { return "quantity must not exceed the available stock"; }
};

// requested < available (literal reading of the requirement: always keep at least one in stock)
class StrictlyBelowAvailablePolicy : public IStockPolicy
{
public:
    bool allows(int requested, int available) const override { return requested < available; }
    std::string describe() const override { return "quantity must be less than the available stock"; }
};

enum class StockStatus { Ok, InvalidQuantity, OutOfStock, InsufficientStock };

struct StockCheck
{
    StockStatus status = StockStatus::Ok;
    int requested = 0;
    int available = 0;
    std::string message;
    bool ok() const { return status == StockStatus::Ok; }
};

class StockValidator
{
public:
    StockValidator() : policy_(std::make_shared<UpToAvailablePolicy>()) {}
    explicit StockValidator(std::shared_ptr<IStockPolicy> policy) : policy_(std::move(policy)) {}

    // Is `requestedTotal` units of this product allowed?
    StockCheck check(const Product &product, int requestedTotal) const;
    // The order already holds `alreadyInOrder`; can `toAdd` more be added?
    StockCheck checkAdd(const Product &product, int alreadyInOrder, int toAdd) const;

private:
    std::shared_ptr<IStockPolicy> policy_;
};
