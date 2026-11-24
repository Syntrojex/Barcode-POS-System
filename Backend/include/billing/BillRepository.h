#pragma once

/**
 * @file BillRepository.h
 * @brief Data access object (DAO) managing atomic read and write operations for Bill.txt and BillDetail.txt.
 * @author Ali Abdullah (25L-3022)
 *
 * Provides safe flat-file persistence for finalized customer bills. Uses an atomic
 * write-through-temporary-file pattern to guard against partial writes or file corruption
 * during unexpected shutdowns or power loss.
 */

#include "billing/Bill.h"

#include <filesystem>
#include <string>
#include <vector>

namespace barcodehub::billing {

/**
 * @class BillRepository
 * @brief Manages storage, retrieval, and identity generation for Bill domain models.
 *
 * Responsibilities:
 * - Atomic persistence: Rewrites the full bill file through a temporary `.tmp` file and
 *   atomically renames/replaces it, preventing data loss.
 * - Multi-line reconstruction: Parses flat lines sharing the same 9-digit Bill ID and
 *   recombines them into unified Bill aggregates containing all line items.
 * - Duplicate prevention: Rejects append operations if a bill with the same ID is already stored.
 * - Sequential ID generation: Generates structured 9-digit identifiers composed of the date
 *   in YYMMDD format followed by a 3-digit zero-padded sequence counter (e.g. 261118001).
 */
class BillRepository {
public:
    /**
     * @brief Constructs a repository bound to Bill.txt; BillDetail.txt sits next to it unless given.
     * @param billFile Path to Bill.txt (one line per bill).
     * @param detailFile Path to BillDetail.txt (one line per bought product); empty = same folder as Bill.txt.
     */
    explicit BillRepository(std::filesystem::path billFile, std::filesystem::path detailFile = {});

    /**
     * @brief Reads all stored bills from disk, grouping multi-line records by Bill ID.
     *
     * If the target file does not yet exist, returns true with an empty result vector
     * (representing a clean database initialized without previous transactions).
     *
     * @param[out] bills Output vector populated with reconstructed Bill objects.
     * @param[out] error Optional pointer to store error description if file I/O or parsing fails.
     * @return True if read and parsed cleanly; false on I/O or syntax error.
     */
    bool loadAll(std::vector<Bill>& bills, std::string* error = nullptr) const;

    /**
     * @brief Atomically appends a new bill to disk.
     *
     * Validates that the bill has line items, loads existing bills to ensure ID uniqueness,
     * serializes all records to a temporary file, and performs an atomic replace.
     *
     * @param[in] bill The Bill aggregate to append.
     * @param[out] error Optional pointer to store failure description.
     * @return True if written successfully; false on duplication or I/O failure.
     */
    bool append(const Bill& bill, std::string* error = nullptr);

    /**
     * @brief Computes the next available 9-digit Bill ID for a given date.
     *
     * Scans existing bills matching the current YYMMDD prefix, identifies the highest
     * sequence number (001-999), and increments it by one.
     *
     * @param[in] date Date string formatted as YYYY-MM-DD.
     * @param[in] existing Current collection of recorded bills.
     * @param[out] error Optional pointer to store error message if date format is invalid or limit reached.
     * @return 9-digit Bill ID string (e.g., "261118001"), or empty string on error.
     */
    static std::string nextBillId(const std::string& date, const std::vector<Bill>& existing,
                                  std::string* error = nullptr);

private:
    std::filesystem::path file_;       ///< Bill.txt
    std::filesystem::path detailFile_; ///< BillDetail.txt
};

} // namespace barcodehub::billing
