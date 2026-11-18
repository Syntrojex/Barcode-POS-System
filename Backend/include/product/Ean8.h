#pragma once
// EAN-8 helpers (the project's barcode symbology). Header-only, no dependencies.
#include <string>

namespace ean8
{
    inline bool isEightDigits(const std::string &s)
    {
        if (s.size() != 8)
            return false;
        for (char c : s)
            if (c < '0' || c > '9')
                return false;
        return true;
    }

    // Check digit for the first 7 digits (weights 3,1,3,1,3,1,3).
    inline int checkDigit(const std::string &sevenDigits)
    {
        int sum = 0;
        for (int i = 0; i < 7; ++i)
            sum += (sevenDigits[i] - '0') * (i % 2 == 0 ? 3 : 1);
        return (10 - sum % 10) % 10;
    }

    inline bool hasValidCheckDigit(const std::string &code)
    {
        return isEightDigits(code) && (code[7] - '0') == checkDigit(code.substr(0, 7));
    }
}
