#include "product/ProductFileLoader.h"
#include <fstream>
#include <istream>
#include <set>

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
