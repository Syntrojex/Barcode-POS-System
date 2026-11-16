#pragma once

#include "product/Models.h"
#include <string>
#include <vector>

// Persistence only: no GUI, Qt, or product business rules in this class.
class FileIO {
public:
    explicit FileIO(std::string dataDirectory);

    std::vector<Product> loadProducts() const;
    bool saveProducts(const std::vector<Product>& products) const;
    bool appendOrder(const Order& order) const;
    bool appendOrderDetail(const OrderDetail& detail) const;
    bool appendBill(const Bill& bill) const;

private:
    std::string directory;
    std::string pathFor(const std::string& filename) const;
};
