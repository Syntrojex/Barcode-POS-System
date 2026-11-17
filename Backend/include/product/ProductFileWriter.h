#pragma once
// Writes products back to Product.txt in the exact format the parser reads (round-trips).
#include "product/Product.h"
#include <string>
#include <vector>

class ProductFileWriter
{
public:
    std::string serialize(const Product &product) const; // one line, ends with '#'
    // Writes to "<path>.tmp" then renames, so a crash never leaves a half-written file.
    bool save(const std::string &path, const std::vector<Product> &products, std::string *error = nullptr) const;
};
