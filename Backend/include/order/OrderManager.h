#pragma once

#include "order/Order.h"
#include "order/OrderDetail.h"

#include <filesystem>
#include <string>
#include <vector>

namespace barcodehub::order {

// A small transfer object. Product Management supplies this data after barcode lookup.
// This keeps Order Management independent from Farzam's internal Product class.
struct ProductSnapshot {
    std::string productId;
    std::string barcode;
    std::string acronym;
    std::string name;
    std::string measurementUnit; // "count" or "weight"
    double unitPrice{0.0};
    double taxPerUnit{0.0};
    double discountPerUnit{0.0};
    double availableQuantity{0.0};
};

struct CartItem {
    ProductSnapshot product;
    double quantity{0.0};
};

class OrderManager {
public:
    OrderManager(std::filesystem::path orderFile,
                 std::filesystem::path orderDetailFile);

    // Adds a product returned by Product Management. Repeated products are combined.
    bool addToCart(const ProductSnapshot& product, double quantity,
                   std::string* error = nullptr);

    bool setCartQuantity(const std::string& productId, double newQuantity,
                         std::string* error = nullptr);
    bool removeFromCart(const std::string& productId);
    void clearCart();

    const std::vector<CartItem>& cart() const noexcept;
    double subtotalBeforeTax() const noexcept;
    double totalTax() const noexcept;
    double totalDiscount() const noexcept;
    double totalPayable() const noexcept;

    // Creates and persists an in-progress order and its detail rows.
    // Returns the saved Order through createdOrder when successful.
    bool createOrder(const std::string& customerName,
                     const std::string& customerContact,
                     Order& createdOrder,
                     std::string* error = nullptr);

    // Update payment/status after the cashier completes payment or holds the order.
    bool updateOrder(const Order& order, std::string* error = nullptr);

    // The next order ID (preview only; createOrder allocates it at save time).
    std::string nextOrderId(std::string* error = nullptr) const;
    // Remove a newly created, unpaid order if checkout fails.
    bool discardOrder(const std::string& orderId, std::string* error = nullptr);

    // Read persisted data for UI/history screens.
    bool loadOrders(std::vector<Order>& orders, std::string* error = nullptr) const;
    bool loadOrderDetails(std::vector<OrderDetail>& details,
                          std::string* error = nullptr) const;
    // Only the detail rows that belong to one order (for Billing / order history).
    bool loadOrderDetails(const std::string& orderId,
                          std::vector<OrderDetail>& details,
                          std::string* error = nullptr) const;
    bool findOrder(const std::string& orderId, Order& order,
                   std::string* error = nullptr) const;

private:
    std::filesystem::path orderFile_;
    std::filesystem::path orderDetailFile_;
    std::vector<CartItem> cart_;

    bool validateProduct(const ProductSnapshot& product, double quantity,
                         std::string* error) const;
    std::string generateOrderId(const std::string& date,
                                const std::vector<Order>& existing,
                                std::string* error) const;
};

} // namespace barcodehub::order
