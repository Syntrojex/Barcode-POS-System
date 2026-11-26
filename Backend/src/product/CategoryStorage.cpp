#include "CategoryStorage.h"

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

namespace
{
    std::string trim(const std::string &s)
    {
        const auto a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos)
            return {};
        return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
    }

    bool twoDigits(const std::string &s)
    {
        return s.size() == 2 && std::isdigit(static_cast<unsigned char>(s[0])) &&
               std::isdigit(static_cast<unsigned char>(s[1]));
    }
} // namespace

CategoryRepository::CategoryRepository(std::string filePath) : m_path(std::move(filePath)) {}

std::string CategoryRepository::validate(const ProductCategory &c)
{
    if (!twoDigits(c.id) || c.id == "00")
        return "Category ID must be 2 digits from 01 to 99.";
    if (trim(c.name).empty())
        return "Category name is required.";
    if (c.name.find('#') != std::string::npos)
        return "Category name cannot contain '#'.";
    if (c.acronym.empty() || c.acronym.find_first_of(" \t#") != std::string::npos)
        return "Category acronym must be one word without spaces or '#'.";
    return {};
}

std::string CategoryRepository::encode(const ProductCategory &c)
{
    return c.id + ' ' + trim(c.name) + ' ' + c.acronym + '#';
}

bool CategoryRepository::decode(const std::string &rawLine, ProductCategory &out, std::string *error)
{
    auto fail = [error](const std::string &message) {
        if (error)
            *error = message;
        return false;
    };
    std::string line = trim(rawLine);
    if (line.empty() || line.back() != '#')
        return fail("line must end with '#'");
    line.pop_back();
    std::istringstream in(line);
    std::vector<std::string> t;
    for (std::string tok; in >> tok;)
        t.push_back(tok);
    if (t.size() < 3)
        return fail("expected 3 fields: categoryId categoryName categoryAcronym");
    ProductCategory c;
    c.id = t.front();
    c.acronym = t.back();
    for (std::size_t i = 1; i + 1 < t.size(); ++i)
        c.name += (c.name.empty() ? "" : " ") + t[i];
    if (const std::string why = validate(c); !why.empty())
        return fail(why);
    out = c;
    return true;
}

bool CategoryRepository::exists() const
{
    std::error_code ec;
    return std::filesystem::exists(m_path, ec);
}

std::vector<ProductCategory> CategoryRepository::load(std::vector<std::string> *errors) const
{
    std::vector<ProductCategory> categories;
    std::ifstream file(m_path);
    if (!file)
        return categories;
    std::set<std::string> ids;
    std::string line;
    int number = 0;
    while (std::getline(file, line))
    {
        ++number;
        if (number == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0)
            line.erase(0, 3); // UTF-8 byte-order mark
        if (trim(line).empty())
            continue;
        ProductCategory c;
        std::string why;
        if (!decode(line, c, &why))
        {
            if (errors)
                errors->push_back("PCategory.txt line " + std::to_string(number) + ": " + why);
            continue;
        }
        if (!ids.insert(c.id).second)
        {
            if (errors)
                errors->push_back("PCategory.txt line " + std::to_string(number) + ": category id " + c.id +
                                  " appears twice");
            continue;
        }
        categories.push_back(c);
    }
    return categories;
}

bool CategoryRepository::save(const std::vector<ProductCategory> &categories, std::string *error) const
{
    for (const auto &c : categories)
        if (const std::string why = validate(c); !why.empty())
        {
            if (error)
                *error = "category " + c.id + ": " + why;
            return false;
        }
    const std::string tmp = m_path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out)
        {
            if (error)
                *error = "cannot write " + tmp;
            return false;
        }
        for (const auto &c : categories)
            out << encode(c) << '\n';
        if (!out)
        {
            if (error)
                *error = "write failed for " + tmp;
            return false;
        }
    }
    std::error_code ec;
    std::filesystem::rename(tmp, m_path, ec);
    if (ec)
    {
        ec.clear();
        std::filesystem::copy_file(tmp, m_path, std::filesystem::copy_options::overwrite_existing, ec);
        std::error_code ignored;
        std::filesystem::remove(tmp, ignored);
        if (ec)
        {
            if (error)
                *error = "cannot replace " + m_path + ": " + ec.message();
            return false;
        }
    }
    return true;
}
