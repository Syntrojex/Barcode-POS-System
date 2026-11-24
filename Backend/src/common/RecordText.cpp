#include "common/RecordText.h"

#include <cctype>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>

namespace barcodehub::common {

std::string encodeField(const std::string& text) {
    if (text.empty()) return "-";
    if (text == "-") return "%2D";
    std::string out;
    for (const unsigned char c : text) {
        if (c == '%') out += "%25";
        else if (c == '_') out += "%5F";
        else if (c == '#') out += "%23";
        else if (std::isspace(c)) out += '_';
        else out += static_cast<char>(c);
    }
    return out;
}

bool decodeField(const std::string& token, std::string& out) {
    out.clear();
    if (token == "-") return true;
    for (std::size_t i = 0; i < token.size(); ++i) {
        const char c = token[i];
        if (c == '_') {
            out += ' ';
        } else if (c == '%') {
            if (i + 2 >= token.size()) return false;   // truncated %XX
            const std::string hex = token.substr(i + 1, 2);
            if (!std::isxdigit(static_cast<unsigned char>(hex[0])) ||
                !std::isxdigit(static_cast<unsigned char>(hex[1]))) return false;
            out += static_cast<char>(std::stoi(hex, nullptr, 16));
            i += 2;
        } else {
            out += c;
        }
    }
    return true;
}

bool splitRecord(const std::string& rawLine, std::vector<std::string>& tokens) {
    tokens.clear();
    std::string line = rawLine;
    while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back()))) line.pop_back();
    if (line.empty() || line.back() != '#') return false;
    line.pop_back();
    std::istringstream in(line);
    for (std::string token; in >> token;) tokens.push_back(token);
    return true;
}

bool parseNumber(const std::string& token, double& value) {
    try {
        std::size_t used = 0;
        value = std::stod(token, &used);
        return used == token.size() && std::isfinite(value);
    } catch (...) {
        return false;
    }
}

std::string formatMoney(double value) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(2) << value;
    return out.str();
}

std::string formatQuantity(double value) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(3) << value;
    std::string text = out.str();
    while (!text.empty() && text.back() == '0') text.pop_back();
    if (!text.empty() && text.back() == '.') text.pop_back();
    return text.empty() ? "0" : text;
}

double roundMoney(double value) {
    return std::round(value * 100.0) / 100.0;
}

std::string currentDate() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d");
    return out.str();
}

std::string currentTime() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%H:%M:%S");
    return out.str();
}

} // namespace barcodehub::common
