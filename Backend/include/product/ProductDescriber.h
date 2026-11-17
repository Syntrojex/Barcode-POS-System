#ifndef PRODUCT_DESCRIBER_H
#define PRODUCT_DESCRIBER_H

#include "Product.h"

#include <string>
#include <utility>
#include <vector>

// Turns a Product into cashier-readable specifications (barcode scan result, Description button).
// Returns plain data, so the Qt GUI can lay it out as a table and a console can print it as text.
class ProductDescriber
{
public:
    using Field = std::pair<std::string, std::string>; // label, value

    static std::vector<Field> fields(const Product &p);
    static std::string text(const Product &p); // "Label: value" lines
    // Product.txt names are single tokens, so '_' stands for a space ("Whole_Milk" -> "Whole Milk").
    static std::string displayName(const Product &p);
};

#endif
