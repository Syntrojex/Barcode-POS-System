#pragma once
/**
 * @file SeasonRepository.h
 * @brief Reads and writes Seasons.txt (seasonal discounts set by the manager).
 *
 * One season per line, space separated, ending with '#':
 *   startDate endDate discountPercent isActive#
 *   2026-11-20 2026-11-30 10.00 true#
 *
 * Dates are YYYY-MM-DD, the percentage is 0-100 (two decimals) and isActive is true / false.
 * This class only handles the file (SRP); deciding which season applies to a bill is done by the caller.
 */
#include <string>
#include <vector>

namespace barcodehub::billing
{
    /** @brief One line of Seasons.txt. */
    struct SeasonRecord
    {
        std::string startDate; // YYYY-MM-DD
        std::string endDate;   // YYYY-MM-DD, not before startDate
        double percent = 0;    // discount percentage, greater than 0 and at most 100
        bool active = false;   // isActive
    };

    class SeasonRepository
    {
    public:
        explicit SeasonRepository(std::string filePath);

        /**
         * @brief Loads all valid seasons. A missing file means "no seasons" (not an error).
         * @param errors Optional: one message per bad line (the bad line is skipped).
         */
        std::vector<SeasonRecord> load(std::vector<std::string> *errors = nullptr) const;

        /** @brief Replaces the file with these seasons (written to a temp file first, then swapped in). */
        bool save(const std::vector<SeasonRecord> &seasons, std::string *error = nullptr) const;

        /** @brief "2026-11-20 2026-11-30 10.00 true#" */
        static std::string encode(const SeasonRecord &season);

        /** @brief Parses one line; returns false and sets error if a field is missing or invalid. */
        static bool decode(const std::string &line, SeasonRecord &out, std::string *error = nullptr);

    private:
        std::string m_path;
    };
} // namespace barcodehub::billing
