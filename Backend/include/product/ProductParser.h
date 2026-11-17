#pragma once
// Turns ONE line of Product.txt into a Product. Knows the file format, nothing else
// (no file access, no business validation, no duplicates).
//
// Line format (space separated, '#' ends the line):
//   id barcode acronym name... unit price tax discount quantity barcodeFilePath #
// The name may contain spaces: the last six fields are fixed, so everything between the acronym and
// the unit belongs to the name.
#include "product/Product.h"
#include <optional>
#include <string>

struct ParsedProduct
{
    std::optional<Product> product;
    std::string error; // set when product is empty
    bool ok() const { return product.has_value(); }
};

class ProductParser
{
public:
    static constexpr char kEndMarker = '#';
    ParsedProduct parseLine(const std::string &line) const;
};
