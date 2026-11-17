#include "product/ProductManager.h"
#include "product/ProductFileWriter.h"
#include <algorithm>
#include <cctype>

namespace
{
    std::string lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }
}

bool ProductManager::loadProducts(const std::string &filename)
{
    lastLoad_ = loader_.load(filename);
    path_ = filename;
    products_ = lastLoad_.products;
    rebuildIndex();
    return !lastLoad_.hasErrors();
}

bool ProductManager::saveProducts(std::string *error) const
{
    if (path_.empty())
    {
        if (error)
            *error = "no product file has been loaded";
        return false;
    }
    return ProductFileWriter().save(path_, products_, error);
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

void ProductManager::clearProducts()
{
    products_.clear();
    rebuildIndex();
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

const Product *ProductManager::searchByCode(const std::string &codeIn) const
{
    const std::string code = lower(codeIn);
    if (const Product *p = searchByBarcode(codeIn))
        return p;
    if (!code.empty() && std::all_of(code.begin(), code.end(), [](unsigned char c) { return std::isdigit(c); }) &&
        code.size() < 8)
        if (const Product *p = searchByID(std::atoi(code.c_str())))
            return p;
    for (const auto &p : products_)
        if (lower(p.getAcronym()) == code)
            return &p;
    return nullptr;
}

std::vector<const Product *> ProductManager::searchByText(const std::string &query) const
{
    std::vector<const Product *> out;
    const std::string q = lower(query);
    for (const auto &p : products_)
        if (q.empty() || lower(p.getProductName()).find(q) != std::string::npos ||
            lower(p.getAcronym()).find(q) != std::string::npos || p.getBarcode().find(q) != std::string::npos)
            out.push_back(&p);
    return out;
}

Product *ProductManager::mutableById(int id)
{
    const auto it = byId_.find(id);
    return it == byId_.end() ? nullptr : &products_[it->second];
}

bool ProductManager::deductStock(int productId, int quantity)
{
    Product *p = mutableById(productId);
    if (!p || quantity < 0 || quantity > p->getAvailableQuantity())
        return false;
    p->deductStock(quantity);
    return true;
}
