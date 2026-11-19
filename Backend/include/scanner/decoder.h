#pragma once
#include "GrayImage.h"
#include <string>
#include <vector>

// Reads an EAN-8 barcode from a grayscale picture (pure C++17).
// Scans several horizontal lines, then several vertical lines (barcode held sideways),
// and accepts a barcode printed upside down.
class Decoder
{
public:
    // On success `ean8` is the full 8-digit code with a valid check digit - the same key
    // that Product.txt stores - and true is returned.
    // `lineFraction` (0..1) is the height of the aiming line: the rows around it are tried first.
    bool decodeCropping(const GrayImage &gray, std::string &ean8, double lineFraction = 0.5) const;
    // True when the last decode saw a complete barcode whose check digit was wrong.
    bool isBadCheck() const { return badCheckDigit; }

    // Where the last successful decode found the barcode, in picture pixels (for drawing a box).
    struct Hit { double x0 = 0, y0 = 0, x1 = 0, y1 = 0; bool valid = false; };
    Hit lastHit() const { return hit_; }

    // Pieces below are public so they can be unit-tested one by one.
    static const char *const patterns[10]; // 7-module left-hand pattern of digits 0..9
    bool decodeModules(const std::string &bits, std::string &ean8) const;
    static bool hasValidCheckDigit(const std::string &eight);

private:
    // measure every dark/light stripe along one line of pixels (sub-pixel edge positions)
    std::vector<double> lineRuns(const std::vector<float> &line, double &firstEdge) const;
    // turn the 43 stripe widths into 67 modules ('1' dark, '0' light)
    std::string runsToModules(const std::vector<double> &runs) const;
    static int digitMatch(const std::string &seven, bool right);
    std::string modulesToDigits(const std::string &bits) const;
    bool scanLines(const GrayImage &g, bool vertical, std::string &ean8, int preferredPercent) const;

    static constexpr int stripes = 43;
    static constexpr int modules = 67;
    mutable bool badCheckDigit = false;
    mutable Hit hit_;
};
