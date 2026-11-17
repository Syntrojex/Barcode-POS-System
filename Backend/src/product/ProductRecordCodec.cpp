#include "ProductRecordCodec.h"

#include <iomanip>
#include <locale>
#include <sstream>
#include <vector>

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
    if (!toNumber(t[0], id))
        r.error = "product id '" + t[0] + "' is not a whole number";
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

    r.product = Product(id, t[1], t[2], t[3], t[4], price, tax, discount, quantity, t[9]);
    r.ok = true;
    return r;
}

std::string ProductRecordCodec::format(const Product &p)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(2);
    out << p.getProductID() << ' ' << p.getBarcode() << ' ' << p.getAcronym() << ' '
        << p.getProductName() << ' ' << p.getMeasurementUnit() << ' '
        << p.getUnitPrice() << ' ' << p.getTax() << ' ' << p.getDiscount() << ' '
        << p.getAvailableQuantity() << ' ' << p.getBarcodeFilePath() << '#';
    return out.str();
}
