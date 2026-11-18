#pragma once
#include <opencv2/core.hpp>
#include <string>
#include <vector>
class Decoder
{
    // from a thin strip of the picture, measure every black and white stripe
    std::vector<double> scanLineRuns(const cv::Mat &barcode) const;
    // turn the 43 stripe widths into 67 modules
    std::string runsToModules(const std::vector<double> &runs) const;
    // match a module group with the digit table
    static int digitMatch(const std::string &seven, bool right);
    // read all 8 digits from the 67 modules
    std::string modulesToDigits(const std::string &bits) const;
    // is the check digit valid?
    bool isCheckDigit(const std::string &d) const;
    static constexpr int stripes = 43;     // (3 + 4*4 + 5 + 4*4 + 3)=43 black/white stripes
    static constexpr int modules = 67;     // 3 + 28 + 5 + 28 + 3 modules
    static const char *const patterns[10]; // pattern of each digit 0..9
    mutable bool badCheckDigit = false;    // check for a badcheckdigit
public:
    // Historic name: returns the FULL 8-digit EAN used as Product.txt lookup key.
    bool decodeSix(const std::string &bits, std::string &six) const;
    bool decodeCropping(const cv::Mat &gray, std::string &six) const;
    bool isBadCheck() const;
};