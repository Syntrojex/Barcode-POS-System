#pragma once

#include <string>

// Product is a plain C++ domain model. It has no dependency on Qt or the GUI.
struct Product {
    int productId{0};
    std::string barcode;
    std::string acronym;          // Maximum 8 characters
    std::string name;             // One token in Product.txt (spaces are not supported)
    std::string measurementUnit;  // "count" or "weight"
    double unitPrice{0.0};        // PKR, excluding tax
    double tax{0.0};              // PKR per unit
    double discount{0.0};         // PKR per unit
    int availableQuantity{0};
    std::string barcodeFilePath;

    bool isValid(std::string* reason = nullptr) const;
};
