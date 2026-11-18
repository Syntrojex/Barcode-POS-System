#pragma once
// Currency formatting shared by the description and (later) bills: "PKR 1,234.50".
#include <cmath>
#include <cstdio>
#include <string>

inline std::string formatPKR(double amount)
{
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.2f", std::fabs(amount));
    std::string s = buf;
    const std::size_t dot = s.find('.');
    for (int i = static_cast<int>(dot) - 3; i > 0; i -= 3)
        s.insert(static_cast<std::size_t>(i), ",");
    return std::string(amount < 0 ? "-" : "") + "PKR " + s;
}
