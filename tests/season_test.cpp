// Tests for Seasons.txt reading/writing (SeasonRepository).
#include "billing/SeasonRepository.h"

#include <cstdio>
#include <fstream>
#include <iostream>

using barcodehub::billing::SeasonRecord;
using barcodehub::billing::SeasonRepository;

static int failures = 0;
#define CHECK(cond)                                                       \
    do                                                                    \
    {                                                                     \
        if (!(cond))                                                      \
        {                                                                 \
            std::cerr << "FAILED line " << __LINE__ << ": " #cond "\n";   \
            ++failures;                                                   \
        }                                                                 \
    } while (0)

int main()
{
    // encode: exact Seasons.txt line
    CHECK(SeasonRepository::encode({"2026-11-20", "2026-11-30", 10, true}) == "2026-11-20 2026-11-30 10.00 true#");

    // decode: a good line
    SeasonRecord s;
    CHECK(SeasonRepository::decode("2026-12-01 2026-12-31 12.5 false#", s));
    CHECK(s.startDate == "2026-12-01" && s.endDate == "2026-12-31" && s.percent == 12.5 && !s.active);

    // decode: bad lines are rejected
    std::string err;
    CHECK(!SeasonRepository::decode("2026-12-01 2026-12-31 10 true", s, &err));      // no '#'
    CHECK(!SeasonRepository::decode("2026-12-31 2026-12-01 10 true#", s, &err));     // end before start
    CHECK(!SeasonRepository::decode("2026-12-01 2026-12-31 0 true#", s, &err));      // 0 percent
    CHECK(!SeasonRepository::decode("2026-12-01 2026-12-31 120 true#", s, &err));    // over 100
    CHECK(!SeasonRepository::decode("2026-12-01 2026-12-31 10 yes#", s, &err));      // not true/false
    CHECK(!SeasonRepository::decode("01-12-2026 2026-12-31 10 true#", s, &err));     // wrong date format
    CHECK(!SeasonRepository::decode("2026-12-01 2026-12-31 10 true extra#", s, &err)); // too many fields

    // save + load round trip; a missing file loads as empty
    const std::string path = "season_test_Seasons.txt";
    std::remove(path.c_str());
    SeasonRepository repo(path);
    CHECK(repo.load().empty());
    CHECK(repo.save({{"2026-11-20", "2026-11-30", 10, true}, {"2027-03-20", "2027-03-27", 15, false}}));
    std::vector<std::string> errors;
    auto loaded = repo.load(&errors);
    CHECK(loaded.size() == 2 && errors.empty());
    CHECK(loaded.size() == 2 && loaded[1].percent == 15 && !loaded[1].active);

    // a bad line in the file is skipped and reported
    {
        std::ofstream f(path, std::ios::app);
        f << "garbage line#\n";
    }
    errors.clear();
    CHECK(repo.load(&errors).size() == 2 && errors.size() == 1);
    std::remove(path.c_str());

    if (failures == 0)
        std::cout << "season_test: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
