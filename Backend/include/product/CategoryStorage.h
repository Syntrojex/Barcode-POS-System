#pragma once
// CategoryStorage: PCategory.txt (Increment 3, requirement 1).
// One product category per line, space separated, ending with '#':
//     categoryId categoryName categoryAcronym#        e.g.  10 Dairy & Milk Products DAI#
// The Category Id (2 digits, 01-99) is the first two digits of every barcode in that category.
// The name may contain spaces (it is everything between the id and the acronym); the acronym is one word.
#include <string>
#include <vector>

struct ProductCategory
{
    std::string id;      // "10"
    std::string name;    // "Dairy & Milk Products"
    std::string acronym; // "DAI"
};

class CategoryRepository
{
public:
    explicit CategoryRepository(std::string filePath);

    static std::string encode(const ProductCategory &c);
    static bool decode(const std::string &line, ProductCategory &out, std::string *error = nullptr);
    // Empty when the category can be stored; otherwise why not (id format, empty name, acronym with spaces...).
    static std::string validate(const ProductCategory &c);

    bool exists() const;
    // Reads every valid line; bad or duplicate lines are skipped and described in `errors`.
    std::vector<ProductCategory> load(std::vector<std::string> *errors = nullptr) const;
    // Writes "<file>.tmp" first and then replaces the file, so a crash never leaves half a file.
    bool save(const std::vector<ProductCategory> &categories, std::string *error = nullptr) const;

private:
    std::string m_path;
};
