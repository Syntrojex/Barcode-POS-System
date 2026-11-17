#include "ProductManager.h"

#include "ProductRecordCodec.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <utility>

ProductManager::ProductManager() : validator_(ProductValidator::withDefaultRules()) {}

ProductManager::ProductManager(ProductValidator validator) : validator_(std::move(validator)) {}

bool ProductManager::loadProducts(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        errors_ = {"cannot open '" + filename + "'"};
        return false;
    }
    return loadProducts(file);
}

bool ProductManager::loadProducts(std::istream &in)
{
    errors_.clear();
    ProductManager staging(validator_); // built on the side so a bad file never half-loads
    staging.errors_.clear();

    std::string line;
    for (int lineNo = 1; std::getline(in, line); ++lineNo)
    {
        if (line.find_first_not_of(" \t\r\n") == std::string::npos)
            continue; // blank line

        const auto parsed = ProductRecordCodec::parse(line);
        if (!parsed.ok)
        {
            errors_.push_back("line " + std::to_string(lineNo) + ": " + parsed.error);
            continue;
        }
        if (!staging.addProduct(parsed.product))
            for (const auto &e : staging.errors_)
                errors_.push_back("line " + std::to_string(lineNo) + ": " + e);
    }

    if (!errors_.empty())
        return false;

    products_ = std::move(staging.products_);
    rebuildIndex();
    return true;
}

bool ProductManager::saveProducts(const std::string &filename) const
{
    // Write to a temp file first so a crash never leaves a half-written Product.txt.
    const std::string tmp = filename + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out)
            return false;
        for (const auto &p : products_)
            out << ProductRecordCodec::format(p) << '\n';
        if (!out)
            return false;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, filename, ec);
    if (ec) // some platforms refuse to rename over an existing file
    {
        std::filesystem::remove(filename, ec);
        std::filesystem::rename(tmp, filename, ec);
    }
    return !ec;
}

bool ProductManager::addProduct(const Product &p)
{
    errors_ = validator_.validate(p);
    if (!errors_.empty())
        return false;
    if (byId_.count(p.getProductID()))
    {
        errors_ = {"duplicate product id " + std::to_string(p.getProductID())};
        return false;
    }
    if (byBarcode_.count(p.getBarcode()))
    {
        errors_ = {"duplicate barcode " + p.getBarcode()};
        return false;
    }
    byId_[p.getProductID()] = products_.size();
    byBarcode_[p.getBarcode()] = products_.size();
    products_.push_back(p);
    return true;
}

Product *ProductManager::searchByBarcode(const std::string &barcode)
{
    return const_cast<Product *>(std::as_const(*this).searchByBarcode(barcode));
}

Product *ProductManager::searchByID(int id)
{
    return const_cast<Product *>(std::as_const(*this).searchByID(id));
}

const Product *ProductManager::searchByBarcode(const std::string &barcode) const
{
    const auto it = byBarcode_.find(barcode);
    return it == byBarcode_.end() ? nullptr : &products_[it->second];
}

const Product *ProductManager::searchByID(int id) const
{
    const auto it = byId_.find(id);
    return it == byId_.end() ? nullptr : &products_[it->second];
}

void ProductManager::clearProducts()
{
    products_.clear();
    byBarcode_.clear();
    byId_.clear();
}

StockStatus ProductManager::deductStock(int productId, int quantity)
{
    Product *p = searchByID(productId);
    if (!p)
        return StockStatus::UnknownProduct;
    const StockStatus status = StockValidator::check(*p, quantity);
    if (status == StockStatus::Ok)
        p->setAvailableQuantity(p->getAvailableQuantity() - quantity);
    return status;
}

void ProductManager::rebuildIndex()
{
    byBarcode_.clear();
    byId_.clear();
    for (std::size_t i = 0; i < products_.size(); ++i)
    {
        byBarcode_[products_[i].getBarcode()] = i;
        byId_[products_[i].getProductID()] = i;
    }
}
