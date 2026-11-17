#ifndef PRODUCT_RECORD_CODEC_H
#define PRODUCT_RECORD_CODEC_H

#include "Product.h"

#include <string>

// Converts ONE Product.txt line <-> Product. It knows the file layout and nothing else:
// no file I/O, no business rules (see ProductValidator), no duplicate checks (ProductManager).
//
// Layout (space separated, '#' ends the line):
//   id barcode acronym name unit unitPrice tax discount quantity barcodeFilePath #
class ProductRecordCodec
{
public:
    struct ParseResult
    {
        bool ok = false;
        Product product;
        std::string error; // empty when ok
    };

    static ParseResult parse(const std::string &line);
    static std::string format(const Product &product);
};

#endif
