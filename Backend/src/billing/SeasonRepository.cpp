#include "billing/SeasonRepository.h"

#include <cctype>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace barcodehub::billing
{
    namespace
    {
        // YYYY-MM-DD with a real month (01-12) and day (01-31).
        bool isDate(const std::string &s)
        {
            if (s.size() != 10 || s[4] != '-' || s[7] != '-')
                return false;
            for (int i : {0, 1, 2, 3, 5, 6, 8, 9})
                if (!std::isdigit(static_cast<unsigned char>(s[i])))
                    return false;
            const int month = std::stoi(s.substr(5, 2));
            const int day = std::stoi(s.substr(8, 2));
            return month >= 1 && month <= 12 && day >= 1 && day <= 31;
        }

        std::string trim(const std::string &s)
        {
            const auto a = s.find_first_not_of(" \t\r\n");
            if (a == std::string::npos)
                return std::string();
            const auto b = s.find_last_not_of(" \t\r\n");
            return s.substr(a, b - a + 1);
        }
    } // namespace

    SeasonRepository::SeasonRepository(std::string filePath) : m_path(std::move(filePath)) {}

    std::string SeasonRepository::encode(const SeasonRecord &s)
    {
        std::ostringstream out;
        out << s.startDate << ' ' << s.endDate << ' ' << std::fixed << std::setprecision(2) << s.percent << ' '
            << (s.active ? "true" : "false") << '#';
        return out.str();
    }

    bool SeasonRepository::decode(const std::string &rawLine, SeasonRecord &out, std::string *error)
    {
        auto fail = [error](const std::string &msg)
        {
            if (error)
                *error = msg;
            return false;
        };
        std::string line = trim(rawLine);
        if (line.empty() || line.back() != '#')
            return fail("line must end with '#'");
        line.pop_back();

        std::istringstream in(line);
        std::string start, end, percent, active, extra;
        if (!(in >> start >> end >> percent >> active))
            return fail("expected 4 fields: startDate endDate percent isActive");
        if (in >> extra)
            return fail("too many fields");
        if (!isDate(start) || !isDate(end))
            return fail("dates must be YYYY-MM-DD");
        if (end < start) // ISO dates compare correctly as text
            return fail("end date is before start date");
        double pct = 0;
        try
        {
            size_t used = 0;
            pct = std::stod(percent, &used);
            if (used != percent.size())
                return fail("percent is not a number");
        }
        catch (...)
        {
            return fail("percent is not a number");
        }
        if (pct <= 0 || pct > 100)
            return fail("percent must be greater than 0 and at most 100");
        if (active != "true" && active != "false")
            return fail("isActive must be true or false");

        out = SeasonRecord{start, end, pct, active == "true"};
        return true;
    }

    std::vector<SeasonRecord> SeasonRepository::load(std::vector<std::string> *errors) const
    {
        std::vector<SeasonRecord> seasons;
        std::ifstream file(m_path);
        if (!file)
            return seasons; // no file yet: no seasons
        std::string line;
        int number = 0;
        while (std::getline(file, line))
        {
            ++number;
            if (trim(line).empty())
                continue;
            SeasonRecord s;
            std::string err;
            if (decode(line, s, &err))
                seasons.push_back(s);
            else if (errors)
                errors->push_back("Seasons.txt line " + std::to_string(number) + ": " + err);
        }
        return seasons;
    }

    bool SeasonRepository::save(const std::vector<SeasonRecord> &seasons, std::string *error) const
    {
        const std::string tmp = m_path + ".tmp";
        {
            std::ofstream out(tmp, std::ios::trunc);
            if (!out)
            {
                if (error)
                    *error = "cannot write " + tmp;
                return false;
            }
            for (const auto &s : seasons)
                out << encode(s) << '\n';
            if (!out)
            {
                if (error)
                    *error = "write failed for " + tmp;
                return false;
            }
        }
        std::remove(m_path.c_str()); // Windows rename() will not overwrite an existing file
        if (std::rename(tmp.c_str(), m_path.c_str()) != 0)
        {
            if (error)
                *error = "cannot replace " + m_path;
            return false;
        }
        return true;
    }
} // namespace barcodehub::billing
