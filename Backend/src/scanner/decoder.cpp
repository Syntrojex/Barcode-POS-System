#include "decoder.h"

#include <algorithm>
#include <cmath>

const char *const Decoder::patterns[10] = {"0001101", "0011001", "0010011", "0111101", "0100011",
                                           "0110001", "0101111", "0111011", "0110111", "0001011"};

std::vector<double> Decoder::lineRuns(const std::vector<float> &line, double &firstEdge) const
{
    firstEdge = 0;
    if (line.size() < 3)
        return {};
    const auto mm = std::minmax_element(line.begin(), line.end());
    if (*mm.second - *mm.first < 25.0f) // flat picture: no bars here
        return {};
    const float T = (*mm.first + *mm.second) / 2.0f;
    std::vector<double> edges;
    for (std::size_t i = 0; i + 1 < line.size(); ++i)
    {
        const float a = line[i], b = line[i + 1];
        if ((a < T) != (b < T))
            edges.push_back(static_cast<double>(i) + (T - a) / (b - a));
    }
    if (edges.size() < 2)
        return {};
    firstEdge = edges.front();
    std::vector<double> runs;
    for (std::size_t j = 0; j + 1 < edges.size(); ++j)
        runs.push_back(edges[j + 1] - edges[j]);
    return runs;
}

std::string Decoder::runsToModules(const std::vector<double> &runs) const
{
    if (runs.size() != static_cast<std::size_t>(stripes))
        return "";
    double total = 0;
    for (double r : runs)
        total += r;
    const double module = total / modules;
    std::string bits;
    for (std::size_t i = 0; i < runs.size(); ++i)
    {
        const int n = static_cast<int>(std::lround(runs[i] / module));
        if (n < 1 || n > 4)
            return "";
        bits += std::string(static_cast<std::size_t>(n), i % 2 == 0 ? '1' : '0');
    }
    return bits.size() == static_cast<std::size_t>(modules) ? bits : "";
}

int Decoder::digitMatch(const std::string &seven, bool right)
{
    for (int i = 0; i < 10; ++i)
    {
        std::string pattern = patterns[i];
        if (right)
            for (char &c : pattern)
                c = (c == '1') ? '0' : '1';
        if (pattern == seven)
            return i;
    }
    return -1;
}

std::string Decoder::modulesToDigits(const std::string &bits) const
{
    if (bits.size() != static_cast<std::size_t>(modules) || bits.compare(0, 3, "101") != 0 ||
        bits.compare(64, 3, "101") != 0 || bits.compare(31, 5, "01010") != 0)
        return "";
    std::string digits;
    for (int i = 0; i < 4; ++i)
    {
        const int d = digitMatch(bits.substr(3 + 7 * i, 7), false);
        if (d < 0)
            return "";
        digits += static_cast<char>('0' + d);
    }
    for (int j = 0; j < 4; ++j)
    {
        const int d = digitMatch(bits.substr(36 + 7 * j, 7), true);
        if (d < 0)
            return "";
        digits += static_cast<char>('0' + d);
    }
    return digits;
}

bool Decoder::hasValidCheckDigit(const std::string &d)
{
    if (d.size() != 8)
        return false;
    int sum = 0;
    for (int i = 0; i < 7; ++i)
        sum += (d[i] - '0') * ((i % 2 == 0) ? 3 : 1);
    return (10 - sum % 10) % 10 == d[7] - '0';
}

bool Decoder::decodeModules(const std::string &bits, std::string &ean8) const
{
    ean8.clear();
    for (int attempt = 0; attempt < 2; ++attempt) // straight, then upside down
    {
        const std::string b = attempt == 0 ? bits : std::string(bits.rbegin(), bits.rend());
        const std::string d = modulesToDigits(b);
        if (d.size() != 8)
            continue;
        if (!hasValidCheckDigit(d))
        {
            badCheckDigit = true;
            continue;
        }
        ean8 = d;
        badCheckDigit = false;
        return true;
    }
    return false;
}

// Average a thin band of lines (tolerates slightly tilted bars), pad with white, find the runs.
bool Decoder::scanLines(const GrayImage &g, bool vertical, std::string &ean8, int preferredPercent) const
{
    const int across = vertical ? g.height : g.width; // length of the scan line
    const int lines = vertical ? g.width : g.height;  // how many lines are available
    constexpr int pad = 12;
    std::vector<int> order{preferredPercent, preferredPercent - 4, preferredPercent + 4, preferredPercent - 8,
                           preferredPercent + 8, 50, 40, 60, 30, 70, 20, 80, 10, 90};
    for (int percent : order)
    {
        if (percent < 2 || percent > 98)
            continue;
        const int centre = lines * percent / 100;
        const int from = std::max(0, centre - 3), to = std::min(lines, centre + 4);
        if (to <= from)
            continue;
        std::vector<float> line(static_cast<std::size_t>(across) + 2 * pad, 255.0f);
        for (int i = 0; i < across; ++i)
        {
            float sum = 0;
            for (int k = from; k < to; ++k)
                sum += vertical ? g.at(k, i) : g.at(i, k);
            line[static_cast<std::size_t>(i) + pad] = sum / static_cast<float>(to - from);
        }
        // light smoothing against camera noise
        std::vector<float> smooth(line);
        for (std::size_t i = 1; i + 1 < line.size(); ++i)
            smooth[i] = (line[i - 1] + 2 * line[i] + line[i + 1]) / 4.0f;

        double firstEdge = 0;
        const auto runs = lineRuns(smooth, firstEdge);
        // other dark objects may share the line: try every starting dark stripe
        for (std::size_t start = 0; start + stripes <= runs.size(); start += 2)
        {
            const std::vector<double> candidate(runs.begin() + static_cast<long>(start),
                                                runs.begin() + static_cast<long>(start) + stripes);
            const std::string bits = runsToModules(candidate);
            if (!bits.empty() && decodeModules(bits, ean8))
            {
                double before = 0, len = 0;
                for (std::size_t k = 0; k < start; ++k) before += runs[k];
                for (double r : candidate) len += r;
                const double a = firstEdge + before - pad, b = a + len; // along the scan line
                const double mid = (from + to) / 2.0, half = len * 0.28; // box height ~ barcode aspect
                hit_.valid = true;
                if (vertical) { hit_.x0 = mid - half; hit_.x1 = mid + half; hit_.y0 = a; hit_.y1 = b; }
                else { hit_.x0 = a; hit_.x1 = b; hit_.y0 = mid - half; hit_.y1 = mid + half; }
                return true;
            }
        }
    }
    return false;
}

bool Decoder::decodeCropping(const GrayImage &gray, std::string &ean8, double lineFraction) const
{
    ean8.clear();
    badCheckDigit = false;
    hit_ = Hit{};
    if (gray.empty())
        return false;
    bool badCheck = false;
    for (bool vertical : {false, true})
    {
        if (scanLines(gray, vertical, ean8, vertical ? 50 : static_cast<int>(lineFraction * 100.0 + 0.5)))
            return true;
        badCheck = badCheck || badCheckDigit;
    }
    badCheckDigit = badCheck;
    return false;
}
