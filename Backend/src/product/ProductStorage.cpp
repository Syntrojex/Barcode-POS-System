#include "ProductStorage.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <istream>
#include <locale>
#include <set>
#include <sstream>

// Shared helpers used only inside this file
namespace
{
    std::string trim(const std::string &s)
    {
        const char *ws = " \t\r\n";
        const auto b = s.find_first_not_of(ws);
        if (b == std::string::npos)
            return {};
        return s.substr(b, s.find_last_not_of(ws) - b + 1);
    }

    std::string lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    bool toInt(const std::string &s, int &out)
    {
        char *end = nullptr;
        errno = 0;
        const long v = std::strtol(s.c_str(), &end, 10);
        if (s.empty() || *end != '\0' || errno == ERANGE || v < -2147483647L || v > 2147483647L)
            return false;
        out = static_cast<int>(v);
        return true;
    }

    bool toDouble(const std::string &s, double &out)
    {
        char *end = nullptr;
        errno = 0;
        const double v = std::strtod(s.c_str(), &end);
        if (s.empty() || *end != '\0' || errno == ERANGE)
            return false;
        out = v;
        return true;
    }

    // Whole-token numeric conversion in the "C" locale: "12abc" and "1,5" are rejected.
    template <typename T>
    bool toNumber(const std::string &token, T &out)
    {
        std::istringstream in(token);
        in.imbue(std::locale::classic());
        in >> out;
        return !in.fail() && in.peek() == std::char_traits<char>::eof();
    }
}

// ======================= ProductParser =======================
ParsedProduct ProductParser::parseLine(const std::string &rawLine) const
{
    ParsedProduct out;
    std::string line = trim(rawLine);
    if (line.empty() || line.back() != kEndMarker)
    {
        out.error = "missing '#' end-of-line marker";
        return out;
    }
    line.pop_back();

    std::vector<std::string> t;
    std::istringstream in(line);
    for (std::string tok; in >> tok;)
        t.push_back(tok);

    constexpr std::size_t kMinFields = 10; // name needs at least one word
    if (t.size() < kMinFields)
    {
        out.error = "expected 10 fields but found " + std::to_string(t.size());
        return out;
    }

    const std::size_t n = t.size();
    int id = 0, quantity = 0;
    double price = 0, tax = 0, discount = 0;
    MeasurementUnit unit = MeasurementUnit::Count;

    if (!toInt(t[0], id))
        out.error = "product id '" + t[0] + "' is not a whole number";
    else if (!parseMeasurementUnit(t[n - 6], unit))
        out.error = "measurement unit '" + t[n - 6] + "' must be 'count' or 'weight'";
    else if (!toDouble(t[n - 5], price))
        out.error = "unit price '" + t[n - 5] + "' is not a number";
    else if (!toDouble(t[n - 4], tax))
        out.error = "tax '" + t[n - 4] + "' is not a number";
    else if (!toDouble(t[n - 3], discount))
        out.error = "discount '" + t[n - 3] + "' is not a number";
    else if (!toInt(t[n - 2], quantity))
        out.error = "available quantity '" + t[n - 2] + "' is not a whole number";
    if (!out.error.empty())
        return out;

    std::string name;
    for (std::size_t i = 3; i < n - 6; ++i)
        name += (name.empty() ? "" : " ") + t[i];

    out.product = Product(id, t[1], t[2], name, unit, price, tax, discount, quantity, t[n - 1]);
    return out;
}

// ======================= ProductRecordCodec =======================
ProductRecordCodec::ParseResult ProductRecordCodec::parse(const std::string &rawLine)
{
    ParseResult r;
    std::string line = trim(rawLine); // also strips the '\r' of Windows line endings

    if (line.empty() || line.back() != '#')
    {
        r.error = "missing '#' end-of-line marker";
        return r;
    }
    line.pop_back();

    std::vector<std::string> t;
    std::istringstream in(line);
    for (std::string tok; in >> tok;)
        t.push_back(tok);

    if (t.size() != 10)
    {
        r.error = "expected 10 fields but found " + std::to_string(t.size());
        return r;
    }

    int id = 0, quantity = 0;
    double price = 0, tax = 0, discount = 0;
    MeasurementUnit unit = MeasurementUnit::Count;
    if (!toNumber(t[0], id))
        r.error = "product id '" + t[0] + "' is not a whole number";
    else if (!parseMeasurementUnit(t[4], unit))
        r.error = "measurement unit '" + t[4] + "' must be 'count' or 'weight'";
    else if (!toNumber(t[5], price))
        r.error = "unit price '" + t[5] + "' is not a number";
    else if (!toNumber(t[6], tax))
        r.error = "tax '" + t[6] + "' is not a number";
    else if (!toNumber(t[7], discount))
        r.error = "discount '" + t[7] + "' is not a number";
    else if (!toNumber(t[8], quantity))
        r.error = "available quantity '" + t[8] + "' is not a whole number";
    if (!r.error.empty())
        return r;

    r.product = Product(id, t[1], t[2], t[3], unit, price, tax, discount, quantity, t[9]);
    r.ok = true;
    return r;
}

std::string ProductRecordCodec::format(const Product &p)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(2);
    out << p.getProductID() << ' ' << p.getBarcode() << ' ' << p.getAcronym() << ' '
        << p.getProductName() << ' ' << p.getMeasurementUnitName() << ' '
        << p.getUnitPrice() << ' ' << p.getTax() << ' ' << p.getDiscount() << ' '
        << p.getAvailableQuantity() << ' ' << p.getBarcodeFilePath() << '#';
    return out.str();
}

// ======================= ProductFileWriter =======================
std::string ProductFileWriter::serialize(const Product &p) const
{
    char nums[96];
    std::snprintf(nums, sizeof nums, "%.2f %.2f %.2f", p.getUnitPrice(), p.getTax(), p.getDiscount());
    return std::to_string(p.getProductID()) + " " + p.getBarcode() + " " + p.getAcronym() + " " +
           p.getProductName() + " " + p.getMeasurementUnitName() + " " + nums + " " +
           std::to_string(p.getAvailableQuantity()) + " " + p.getBarcodeFilePath() + "#";
}

bool ProductFileWriter::save(const std::string &path, const std::vector<Product> &products, std::string *error) const
{
    namespace fs = std::filesystem;
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out)
        {
            if (error)
                *error = "cannot write '" + tmp + "'";
            return false;
        }
        for (const auto &p : products)
            out << serialize(p) << '\n';
        if (!out)
        {
            if (error)
                *error = "write failed for '" + tmp + "'";
            return false;
        }
    }
    std::error_code ec;
    fs::rename(tmp, path, ec);
    if (ec)
    {
        fs::remove(path, ec);
        fs::rename(tmp, path, ec);
        if (ec)
        {
            if (error)
                *error = "cannot replace '" + path + "': " + ec.message();
            return false;
        }
    }
    return true;
}

// ======================= ProductLoadResult =======================
int ProductLoadResult::errorCount() const
{
    int n = 0;
    for (const auto &i : issues)
        n += i.severity == IssueSeverity::Error;
    return n;
}

int ProductLoadResult::warningCount() const
{
    return static_cast<int>(issues.size()) - errorCount();
}

std::string ProductLoadResult::summary() const
{
    if (!fileOpened)
        return issues.empty() ? "Product file could not be opened" : issues.front().message;
    std::string s = std::to_string(products.size()) + " product(s) loaded";
    if (rejectedLines)
        s += ", " + std::to_string(rejectedLines) + " line(s) rejected";
    s += ".\n";
    for (const auto &i : issues)
        s += std::string(i.severity == IssueSeverity::Error ? "  error   " : "  warning ") + "line " +
             std::to_string(i.line) + ": " + i.message + "\n";
    return s;
}

// ======================= ProductFileLoader =======================
ProductLoadResult ProductFileLoader::load(const std::string &path) const
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        ProductLoadResult r;
        r.issues.push_back({0, IssueSeverity::Error, "cannot open '" + path + "'"});
        return r;
    }
    return loadFrom(file);
}

ProductLoadResult ProductFileLoader::loadFrom(std::istream &in) const
{
    ProductLoadResult result;
    result.fileOpened = true;
    std::set<int> seenIds;
    std::set<std::string> seenBarcodes;

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line))
    {
        ++lineNo;
        if (lineNo == 1 && line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0)
            line.erase(0, 3); // UTF-8 byte-order mark
        if (line.find_first_not_of(" \t\r\n") == std::string::npos)
            continue; // blank line

        const ParsedProduct parsed = parser_.parseLine(line);
        if (!parsed.ok())
        {
            result.issues.push_back({lineNo, IssueSeverity::Error, parsed.error});
            ++result.rejectedLines;
            continue;
        }
        const Product &p = *parsed.product;

        const ValidationReport report = validator_.validate(p);
        for (const auto &w : report.warnings)
            result.issues.push_back({lineNo, IssueSeverity::Warning, w});
        if (!report.ok())
        {
            for (const auto &e : report.errors)
                result.issues.push_back({lineNo, IssueSeverity::Error, e});
            ++result.rejectedLines;
            continue;
        }
        if (seenIds.count(p.getProductID()) || seenBarcodes.count(p.getBarcode()))
        {
            result.issues.push_back({lineNo, IssueSeverity::Error,
                                     "duplicate product id " + std::to_string(p.getProductID()) +
                                         " or barcode " + p.getBarcode()});
            ++result.rejectedLines;
            continue;
        }
        seenIds.insert(p.getProductID());
        seenBarcodes.insert(p.getBarcode());
        result.products.push_back(p);
    }
    return result;
}

// ======================= ProductManager =======================
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
