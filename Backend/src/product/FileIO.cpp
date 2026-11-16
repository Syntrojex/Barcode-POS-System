#include "product/FileIO.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
template <typename T>
bool appendRecord(const std::string& path, const T& record) {
    std::ofstream out(path, std::ios::app);
    if (!out) return false;
    record(out);
    out << "#\n";
    return static_cast<bool>(out);
}
}

FileIO::FileIO(std::string dataDirectory) : directory(std::move(dataDirectory)) {
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
}

std::string FileIO::pathFor(const std::string& filename) const {
    return (std::filesystem::path(directory) / filename).string();
}

std::vector<Product> FileIO::loadProducts() const {
    std::vector<Product> products;
    std::ifstream in(pathFor("Product.txt"));
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (line.back() != '#') return {};
        line.pop_back();
        std::istringstream row(line);
        Product p;
        std::string extra;
        if (!(row >> p.productId >> p.barcode >> p.acronym >> p.name >> p.measurementUnit
                  >> p.unitPrice >> p.tax >> p.discount >> p.availableQuantity >> p.barcodeFilePath)
            || (row >> extra) || !p.isValid()) return {};
        products.push_back(std::move(p));
    }
    return products;
}

bool FileIO::saveProducts(const std::vector<Product>& products) const {
    std::ofstream out(pathFor("Product.txt"), std::ios::trunc);
    if (!out) return false;
    out << std::fixed << std::setprecision(2);
    for (const auto& p : products) {
        if (!p.isValid()) return false;
        out << p.productId << ' ' << p.barcode << ' ' << p.acronym << ' ' << p.name << ' '
            << p.measurementUnit << ' ' << p.unitPrice << ' ' << p.tax << ' ' << p.discount << ' '
            << p.availableQuantity << ' ' << p.barcodeFilePath << "#\n";
    }
    return static_cast<bool>(out);
}

bool FileIO::appendOrder(const Order& o) const {
    return appendRecord(pathFor("Order.txt"), [&](std::ostream& out) {
        out << o.orderId << ' ' << o.date << ' ' << o.time << ' ' << o.customerName << ' '
            << (o.contact.empty() ? "-" : o.contact) << ' ' << o.status << ' '
            << std::fixed << std::setprecision(2) << o.paymentAmount << ' '
            << o.paymentMode << ' ' << (o.merchant.empty() ? "-" : o.merchant);
    });
}

bool FileIO::appendOrderDetail(const OrderDetail& d) const {
    return appendRecord(pathFor("OrderDetail.txt"), [&](std::ostream& out) {
        out << d.orderId << ' ' << d.productId << ' ' << d.barcode << ' ' << d.quantity;
    });
}

bool FileIO::appendBill(const Bill& b) const {
    return appendRecord(pathFor("Bill.txt"), [&](std::ostream& out) {
        out << b.billId << ' ' << b.customerName << ' ' << b.orderId << ' ' << b.productAcronym << ' '
            << std::fixed << std::setprecision(2) << b.unitPrice << ' ' << b.discountPerUnit << ' '
            << b.quantity << ' ' << b.totalWithoutTax << ' ' << b.totalTax << ' '
            << b.totalDiscount << ' ' << b.payable << ' ' << b.paymentStatus;
    });
}
