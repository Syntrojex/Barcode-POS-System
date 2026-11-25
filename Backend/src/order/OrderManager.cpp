#include "order/OrderManager.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>

namespace barcodehub::order {
namespace {

std::string currentDate() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d");
    return out.str();
}

std::string currentTime() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%H:%M:%S");
    return out.str();
}

bool isWholeNumber(double value) {
    return std::isfinite(value) && std::fabs(value - std::round(value)) < 1e-9;
}

bool hasWhitespace(const std::string& text) {
    return std::any_of(text.begin(), text.end(),
                       [](unsigned char c) { return std::isspace(c) != 0; });
}

double roundMoney(double value) {
    return std::round(value * 100.0) / 100.0;
}

void clearError(std::string* error) {
    if (error) error->clear();
}

bool ensureParentDirectory(const std::filesystem::path& file, std::string* error) {
    std::error_code ec;
    if (!file.parent_path().empty()) {
        std::filesystem::create_directories(file.parent_path(), ec);
    }
    if (ec) {
        if (error) *error = "Cannot create data directory: " + ec.message();
        return false;
    }
    return true;
}

// ---- Field encoding -------------------------------------------------------
// Records are one line each, fields separated by single spaces, line ended by '#'
// (same style as Product.txt). To keep that rule true for names containing spaces,
// text fields are escaped: ' ' -> '_', '_' -> %5F, '%' -> %25, '#' -> %23,
// and an empty (optional) field is written as '-'.
std::string encodeField(const std::string& text) {
    if (text.empty()) return "-";
    if (text == "-") return "%2D";
    std::string out;
    for (const unsigned char c : text) {
        if (c == '%') out += "%25";
        else if (c == '_') out += "%5F";
        else if (c == '#') out += "%23";
        else if (std::isspace(c)) out += '_';
        else out += static_cast<char>(c);
    }
    return out;
}

bool decodeField(const std::string& token, std::string& out) {
    out.clear();
    if (token == "-") return true;
    for (std::size_t i = 0; i < token.size(); ++i) {
        const char c = token[i];
        if (c == '_') {
            out += ' ';
        } else if (c == '%') {
            if (i + 2 >= token.size()) return false;   // truncated %XX
            const std::string hex = token.substr(i + 1, 2);
            if (hex.size() != 2 || !std::isxdigit(static_cast<unsigned char>(hex[0])) ||
                !std::isxdigit(static_cast<unsigned char>(hex[1]))) return false;
            out += static_cast<char>(std::stoi(hex, nullptr, 16));
            i += 2;
        } else {
            out += c;
        }
    }
    return true;
}

std::string formatQuantity(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << value;
    std::string text = out.str();
    while (!text.empty() && text.back() == '0') text.pop_back();
    if (!text.empty() && text.back() == '.') text.pop_back();
    return text.empty() ? "0" : text;
}

bool parseNumber(const std::string& token, double& value) {
    try {
        std::size_t used = 0;
        value = std::stod(token, &used);
        return used == token.size() && std::isfinite(value);
    } catch (...) {
        return false;
    }
}

// Splits one record line into tokens. The line must end with '#'.
bool splitRecord(const std::string& rawLine, std::vector<std::string>& tokens) {
    tokens.clear();
    std::string line = rawLine;
    while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back()))) line.pop_back();
    if (line.empty() || line.back() != '#') return false;
    line.pop_back();
    std::istringstream in(line);
    std::string token;
    while (in >> token) tokens.push_back(token);
    return true;
}

std::string lineError(const std::filesystem::path& path, int lineNo, const std::string& why) {
    return path.filename().string() + " line " + std::to_string(lineNo) + ": " + why;
}

bool replaceFile(const std::filesystem::path& temp,
                 const std::filesystem::path& destination,
                 std::string* error) {
    std::error_code ec;
    std::filesystem::rename(temp, destination, ec);
    if (!ec) return true;

    // Some platforms refuse to rename over an existing file: overwrite by copying instead
    // (the destination is never deleted first, so a failure cannot lose the old data).
    ec.clear();
    std::filesystem::copy_file(temp, destination,
                               std::filesystem::copy_options::overwrite_existing, ec);
    std::error_code ignored;
    std::filesystem::remove(temp, ignored);
    if (ec) {
        if (error) *error = "Cannot replace data file: " + ec.message();
        return false;
    }
    return true;
}

bool readOrdersFrom(const std::filesystem::path& path,
                    std::vector<Order>& orders,
                    std::string* error) {
    orders.clear();
    std::error_code existsError;
    if (!std::filesystem::exists(path, existsError)) return true;   // no file yet = no orders

    std::ifstream in(path);
    if (!in) {
        if (error) *error = "Cannot open order file: " + path.string();
        return false;
    }

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        std::vector<std::string> t;
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;  // blank line
        if (!splitRecord(line, t)) {
            if (error) *error = lineError(path, lineNo, "record must end with '#'.");
            return false;
        }
        if (t.size() != 9) {
            if (error) *error = lineError(path, lineNo, "expected 9 fields, found " + std::to_string(t.size()) + ".");
            return false;
        }

        Order order;
        order.id = t[0];
        order.date = t[1];
        order.time = t[2];
        if (!decodeField(t[3], order.customerName) ||
            !decodeField(t[4], order.customerContact) ||
            !decodeField(t[8], order.paymentMerchant)) {
            if (error) *error = lineError(path, lineNo, "bad text encoding.");
            return false;
        }
        if (!orderStatusFromString(t[5], order.status)) {
            if (error) *error = lineError(path, lineNo, "unknown order status '" + t[5] + "'.");
            return false;
        }
        if (!parseNumber(t[6], order.paymentAmount)) {
            if (error) *error = lineError(path, lineNo, "payment amount is not a number.");
            return false;
        }
        if (!paymentModeFromString(t[7], order.paymentMode)) {
            if (error) *error = lineError(path, lineNo, "unknown payment mode '" + t[7] + "'.");
            return false;
        }
        std::string why;
        if (!order.isValid(&why)) {
            if (error) *error = lineError(path, lineNo, why);
            return false;
        }
        for (const auto& existing : orders) {
            if (existing.id == order.id) {
                if (error) *error = lineError(path, lineNo, "duplicate Order ID " + order.id + ".");
                return false;
            }
        }
        orders.push_back(std::move(order));
    }
    return true;
}

bool readDetailsFrom(const std::filesystem::path& path,
                     std::vector<OrderDetail>& details,
                     std::string* error) {
    details.clear();
    std::error_code existsError;
    if (!std::filesystem::exists(path, existsError)) return true;

    std::ifstream in(path);
    if (!in) {
        if (error) *error = "Cannot open order-detail file: " + path.string();
        return false;
    }

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        std::vector<std::string> t;
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
        if (!splitRecord(line, t)) {
            if (error) *error = lineError(path, lineNo, "record must end with '#'.");
            return false;
        }
        if (t.size() != 4) {
            if (error) *error = lineError(path, lineNo, "expected 4 fields, found " + std::to_string(t.size()) + ".");
            return false;
        }
        OrderDetail detail;
        detail.orderId = t[0];
        detail.productId = t[1];
        detail.productBarcode = t[2];
        if (!parseNumber(t[3], detail.quantity)) {
            if (error) *error = lineError(path, lineNo, "quantity is not a number.");
            return false;
        }
        std::string why;
        if (!detail.isValid(&why)) {
            if (error) *error = lineError(path, lineNo, why);
            return false;
        }
        details.push_back(std::move(detail));
    }
    return true;
}

bool writeOrdersTo(const std::filesystem::path& path,
                   const std::vector<Order>& orders,
                   std::string* error) {
    if (!ensureParentDirectory(path, error)) return false;
    const auto temp = path.string() + ".tmp";
    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out) {
            if (error) *error = "Cannot write temporary order file: " + temp;
            return false;
        }
        out << std::fixed << std::setprecision(2);
        for (const auto& order : orders) {
            out << order.id << ' '
                << order.date << ' '
                << order.time << ' '
                << encodeField(order.customerName) << ' '
                << encodeField(order.customerContact) << ' '
                << toString(order.status) << ' '
                << order.paymentAmount << ' '
                << toString(order.paymentMode) << ' '
                << encodeField(order.paymentMerchant) << " #\n";
        }
        out.flush();
        if (!out) {
            std::error_code ignored;
            std::filesystem::remove(temp, ignored);
            if (error) *error = "Failed while writing order data (disk full or no permission?).";
            return false;
        }
    }
    return replaceFile(temp, path, error);
}

bool writeDetailsTo(const std::filesystem::path& path,
                    const std::vector<OrderDetail>& details,
                    std::string* error) {
    if (!ensureParentDirectory(path, error)) return false;
    const auto temp = path.string() + ".tmp";
    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out) {
            if (error) *error = "Cannot write temporary order-detail file: " + temp;
            return false;
        }
        for (const auto& detail : details) {
            out << detail.orderId << ' '
                << detail.productId << ' '
                << detail.productBarcode << ' '
                << formatQuantity(detail.quantity) << " #\n";
        }
        out.flush();
        if (!out) {
            std::error_code ignored;
            std::filesystem::remove(temp, ignored);
            if (error) *error = "Failed while writing order-detail data (disk full or no permission?).";
            return false;
        }
    }
    return replaceFile(temp, path, error);
}

} // namespace

OrderManager::OrderManager(std::filesystem::path orderFile,
                           std::filesystem::path orderDetailFile)
    : orderFile_(std::move(orderFile)),
      orderDetailFile_(std::move(orderDetailFile)) {}

bool OrderManager::validateProduct(const ProductSnapshot& product, double quantity,
                                   std::string* error) const {
    clearError(error);
    if (product.productId.empty() || product.barcode.empty() || product.name.empty()) {
        if (error) *error = "Product ID, barcode, and name are required.";
        return false;
    }
    if (hasWhitespace(product.productId) || hasWhitespace(product.barcode)) {
        if (error) *error = "Product ID and barcode must not contain spaces.";
        return false;
    }
    if (!std::isfinite(quantity) || quantity <= 0.0 ||
        !std::isfinite(product.availableQuantity) || product.availableQuantity < 0.0) {
        if (error) *error = "Quantity and available stock must be valid positive values.";
        return false;
    }
    if (product.measurementUnit == "count" && !isWholeNumber(quantity)) {
        if (error) *error = "Count-based products require a whole-number quantity.";
        return false;
    }
    if (quantity > product.availableQuantity + 1e-9) {
        if (error) {
            std::ostringstream msg;
            msg << "Requested quantity exceeds available stock for " << product.name
                << " (available: " << formatQuantity(product.availableQuantity) << ").";
            *error = msg.str();
        }
        return false;
    }
    if (!std::isfinite(product.unitPrice) || product.unitPrice < 0.0 ||
        !std::isfinite(product.taxPerUnit) || product.taxPerUnit < 0.0 ||
        !std::isfinite(product.discountPerUnit) || product.discountPerUnit < 0.0) {
        if (error) *error = "Product price, tax, and discount must be non-negative.";
        return false;
    }
    if (product.measurementUnit != "count" && product.measurementUnit != "weight") {
        if (error) *error = "Measurement unit must be 'count' or 'weight'.";
        return false;
    }
    if (product.discountPerUnit > product.unitPrice + product.taxPerUnit + 1e-9) {
        if (error) *error = "Discount per unit cannot exceed the unit price plus tax for " + product.name + ".";
        return false;
    }
    return true;
}

bool OrderManager::addToCart(const ProductSnapshot& product, double quantity,
                             std::string* error) {
    clearError(error);
    if (!validateProduct(product, quantity, error)) return false;

    const auto found = std::find_if(cart_.begin(), cart_.end(), [&](const CartItem& item) {
        return item.product.productId == product.productId;
    });

    if (found != cart_.end()) {
        const double combined = found->quantity + quantity;
        if (!validateProduct(product, combined, error)) return false;
        found->product = product;
        found->quantity = combined;
    } else {
        cart_.push_back(CartItem{product, quantity});
    }
    return true;
}

bool OrderManager::setCartQuantity(const std::string& productId, double newQuantity,
                                   std::string* error) {
    clearError(error);
    const auto found = std::find_if(cart_.begin(), cart_.end(), [&](const CartItem& item) {
        return item.product.productId == productId;
    });
    if (found == cart_.end()) {
        if (error) *error = "Product is not present in the cart.";
        return false;
    }
    if (!validateProduct(found->product, newQuantity, error)) return false;
    found->quantity = newQuantity;
    return true;
}

bool OrderManager::removeFromCart(const std::string& productId) {
    const auto oldSize = cart_.size();
    cart_.erase(std::remove_if(cart_.begin(), cart_.end(), [&](const CartItem& item) {
        return item.product.productId == productId;
    }), cart_.end());
    return cart_.size() != oldSize;
}

void OrderManager::clearCart() {
    cart_.clear();
}

const std::vector<CartItem>& OrderManager::cart() const noexcept {
    return cart_;
}

double OrderManager::subtotalBeforeTax() const noexcept {
    double total = 0.0;
    for (const auto& item : cart_) {
        total += item.product.unitPrice * item.quantity;
    }
    return roundMoney(total);
}

double OrderManager::totalTax() const noexcept {
    double total = 0.0;
    for (const auto& item : cart_) {
        total += item.product.taxPerUnit * item.quantity;
    }
    return roundMoney(total);
}

double OrderManager::totalDiscount() const noexcept {
    double total = 0.0;
    for (const auto& item : cart_) {
        total += item.product.discountPerUnit * item.quantity;
    }
    return roundMoney(total);
}

double OrderManager::totalPayable() const noexcept {
    return roundMoney(subtotalBeforeTax() + totalTax() - totalDiscount());
}

std::string OrderManager::generateOrderId(const std::string& date,
                                          const std::vector<Order>& existing,
                                          std::string* error) const {
    // ID layout: YYMM + daily sequence (01-99) + DD.
    // Example: 26110115 = November 2026, order 01 of the 15th.
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') {
        if (error) *error = "Cannot generate Order ID from invalid date.";
        return {};
    }
    const std::string yearMonth = date.substr(2, 2) + date.substr(5, 2);
    const std::string day = date.substr(8, 2);
    int highestSequence = 0;

    for (const auto& order : existing) {
        if (order.id.size() == 8 &&
            order.id.substr(0, 4) == yearMonth &&
            order.id.substr(6, 2) == day) {
            try {
                highestSequence = std::max(highestSequence, std::stoi(order.id.substr(4, 2)));
            } catch (...) {
                // Ignore malformed legacy IDs; valid records are checked elsewhere.
            }
        }
    }

    if (highestSequence >= 99) {
        if (error) *error = "Daily Order ID limit reached (99 orders for this date).";
        return {};
    }

    std::ostringstream id;
    id << yearMonth << std::setw(2) << std::setfill('0')
       << (highestSequence + 1) << day;
    return id.str();
}

bool OrderManager::createOrder(const std::string& customerName,
                               const std::string& customerContact,
                               Order& createdOrder,
                               std::string* error) {
    clearError(error);
    if (cart_.empty()) {
        if (error) *error = "Cannot create an order with an empty cart.";
        return false;
    }
    if (customerName.empty()) {
        if (error) *error = "Customer name is required.";
        return false;
    }

    std::vector<Order> orders;
    std::vector<OrderDetail> details;
    if (!loadOrders(orders, error) || !loadOrderDetails(details, error)) return false;

    Order order;
    order.date = currentDate();
    order.time = currentTime();
    order.id = generateOrderId(order.date, orders, error);
    if (order.id.empty()) return false;
    order.customerName = customerName;
    order.customerContact = customerContact;
    order.status = OrderStatus::InProgress;
    order.paymentAmount = 0.0;
    order.paymentMode = PaymentMode::Cash;
    order.paymentMerchant.clear();

    if (!order.isValid(error)) return false;

    std::vector<OrderDetail> newDetails = details;
    for (const auto& item : cart_) {
        newDetails.push_back(OrderDetail{
            order.id,
            item.product.productId,
            item.product.barcode,
            item.quantity
        });
    }

    orders.push_back(order);

    // Save detail rows first. If order saving fails, the extra detail rows are harmless
    // only if the application does not expose orphan details; rollback them immediately.
    if (!writeDetailsTo(orderDetailFile_, newDetails, error)) return false;
    if (!writeOrdersTo(orderFile_, orders, error)) {
        std::string rollbackError;
        writeDetailsTo(orderDetailFile_, details, &rollbackError);
        return false;
    }

    createdOrder = order;
    clearCart();
    return true;
}

std::string OrderManager::nextOrderId(std::string* error) const {
    std::vector<Order> existing;
    if (!loadOrders(existing, error)) return {};
    return generateOrderId(currentDate(), existing, error);
}

bool OrderManager::discardOrder(const std::string& orderId, std::string* error) {
    clearError(error);
    std::vector<Order> orders;
    std::vector<OrderDetail> details;
    if (!loadOrders(orders, error) || !loadOrderDetails(details, error)) return false;
    const auto it = std::find_if(orders.begin(), orders.end(), [&](const Order& order) {
        return order.id == orderId;
    });
    if (it == orders.end() ||
        (it->status != OrderStatus::InProgress && it->status != OrderStatus::OnHold)) {
        if (error) *error = "Only existing unpaid (in-progress or on-hold) orders may be discarded.";
        return false;
    }
    const auto oldDetails = details;
    orders.erase(it);
    details.erase(std::remove_if(details.begin(), details.end(), [&](const OrderDetail& detail) {
        return detail.orderId == orderId;
    }), details.end());
    if (!writeDetailsTo(orderDetailFile_, details, error)) return false;
    if (!writeOrdersTo(orderFile_, orders, error)) {
        std::string ignored;
        writeDetailsTo(orderDetailFile_, oldDetails, &ignored);
        return false;
    }
    return true;
}

bool OrderManager::updateOrder(const Order& order, std::string* error) {
    clearError(error);
    if (!order.isValid(error)) return false;

    std::vector<Order> orders;
    if (!loadOrders(orders, error)) return false;

    const auto found = std::find_if(orders.begin(), orders.end(), [&](const Order& existing) {
        return existing.id == order.id;
    });
    if (found == orders.end()) {
        if (error) *error = "Order ID was not found.";
        return false;
    }

    if (found->status == OrderStatus::Archived && order.status != OrderStatus::Archived) {
        if (error) *error = "An archived order cannot be reopened.";
        return false;
    }
    // Date and time belong to the moment of creation and must not be rewritten.
    Order updated = order;
    updated.date = found->date;
    updated.time = found->time;
    *found = updated;
    return writeOrdersTo(orderFile_, orders, error);
}

bool OrderManager::saveCartToOrder(const std::string& orderId,
                                   const std::string& customerName,
                                   const std::string& customerContact,
                                   std::string* error) {
    clearError(error);
    if (cart_.empty()) {
        if (error) *error = "Cannot save an order with an empty cart.";
        return false;
    }
    if (customerName.empty()) {
        if (error) *error = "Customer name is required.";
        return false;
    }
    std::vector<Order> orders;
    std::vector<OrderDetail> details;
    if (!loadOrders(orders, error) || !loadOrderDetails(details, error)) return false;
    const auto found = std::find_if(orders.begin(), orders.end(), [&](const Order& o) {
        return o.id == orderId;
    });
    if (found == orders.end()) {
        if (error) *error = "Order ID was not found.";
        return false;
    }
    if (found->status == OrderStatus::Archived) {
        if (error) *error = "An archived order cannot be changed.";
        return false;
    }
    found->customerName = customerName;
    found->customerContact = customerContact;
    if (!found->isValid(error)) return false;

    const auto oldDetails = details;
    details.erase(std::remove_if(details.begin(), details.end(), [&](const OrderDetail& d) {
        return d.orderId == orderId;
    }), details.end());
    for (const auto& item : cart_) {
        details.push_back(OrderDetail{orderId, item.product.productId, item.product.barcode, item.quantity});
    }
    if (!writeDetailsTo(orderDetailFile_, details, error)) return false;
    if (!writeOrdersTo(orderFile_, orders, error)) {
        std::string ignored;
        writeDetailsTo(orderDetailFile_, oldDetails, &ignored);
        return false;
    }
    clearCart();
    return true;
}

bool OrderManager::loadOrders(std::vector<Order>& orders, std::string* error) const {
    clearError(error);
    return readOrdersFrom(orderFile_, orders, error);
}

bool OrderManager::loadOrderDetails(std::vector<OrderDetail>& details,
                                    std::string* error) const {
    clearError(error);
    return readDetailsFrom(orderDetailFile_, details, error);
}

bool OrderManager::loadOrderDetails(const std::string& orderId,
                                    std::vector<OrderDetail>& details,
                                    std::string* error) const {
    std::vector<OrderDetail> all;
    if (!loadOrderDetails(all, error)) return false;
    details.clear();
    for (auto& detail : all) {
        if (detail.orderId == orderId) details.push_back(std::move(detail));
    }
    return true;
}

bool OrderManager::findOrder(const std::string& orderId, Order& order,
                             std::string* error) const {
    std::vector<Order> orders;
    if (!loadOrders(orders, error)) return false;
    const auto found = std::find_if(orders.begin(), orders.end(), [&](const Order& o) {
        return o.id == orderId;
    });
    if (found == orders.end()) {
        if (error) *error = "Order ID was not found.";
        return false;
    }
    order = *found;
    return true;
}

} // namespace barcodehub::order
