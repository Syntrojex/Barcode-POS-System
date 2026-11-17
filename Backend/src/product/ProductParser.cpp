#include "product/ProductParser.h"
#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <vector>

namespace
{
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

    std::string trim(const std::string &s)
    {
        const char *ws = " \t\r\n";
        const auto b = s.find_first_not_of(ws);
        if (b == std::string::npos)
            return "";
        return s.substr(b, s.find_last_not_of(ws) - b + 1);
    }
}

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
