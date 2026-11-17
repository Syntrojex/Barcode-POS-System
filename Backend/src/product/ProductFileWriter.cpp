#include "product/ProductFileWriter.h"
#include <cstdio>
#include <filesystem>
#include <fstream>

std::string ProductFileWriter::serialize(const Product &p) const
{
    char nums[96];
    std::snprintf(nums, sizeof nums, "%.2f %.2f %.2f", p.getUnitPrice(), p.getTax(), p.getDiscount());
    return std::to_string(p.getProductID()) + " " + p.getBarcode() + " " + p.getAcronym() + " " +
           p.getProductName() + " " + p.getMeasurementUnitName() + " " + nums + " " +
           std::to_string(p.getAvailableQuantity()) + " " + p.getBarcodeFilePath() + "#";
}

bool ProductFileWriter::save(const std::string &path, const std::vector<Product> &products, std::string *error) const
{
    namespace fs = std::filesystem;
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out)
        {
            if (error)
                *error = "cannot write '" + tmp + "'";
            return false;
        }
        for (const auto &p : products)
            out << serialize(p) << '\n';
        if (!out)
        {
            if (error)
                *error = "write failed for '" + tmp + "'";
            return false;
        }
    }
    std::error_code ec;
    fs::rename(tmp, path, ec);
    if (ec)
    {
        fs::remove(path, ec);
        fs::rename(tmp, path, ec);
        if (ec)
        {
            if (error)
                *error = "cannot replace '" + path + "': " + ec.message();
            return false;
        }
    }
    return true;
}
