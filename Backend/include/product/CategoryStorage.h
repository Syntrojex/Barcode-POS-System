#pragma once
// CategoryStorage: reads and writes PCategory.txt (Increment 3, requirement 1).
//
// Each line of PCategory.txt is one category, with the fields separated by spaces and a '#' at the end:
//
//     categoryId categoryName categoryAcronym#
//     10 Dairy & Milk Products DAI#
//
// - categoryId : 2 digits (01 to 99). It becomes the first two digits of every barcode in this category.
// - categoryName : may contain spaces (it is every word between the id and the acronym).
// - categoryAcronym : one word.
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

    // One category -> one line of text ("10 Dairy & Milk Products DAI#").
    static std::string encode(const ProductCategory &c);

    // One line of text -> one category. Returns false (and the reason in `error`) if the line is not valid.
    static bool decode(const std::string &line, ProductCategory &out, std::string *error = nullptr);

    // Returns "" when the category is valid, otherwise a message saying what is wrong.
    static std::string validate(const ProductCategory &c);

    // True if PCategory.txt exists.
    bool exists() const;

    // Reads all categories from the file. Invalid lines and repeated ids are skipped; the reasons go into `errors`.
    std::vector<ProductCategory> load(std::vector<std::string> *errors = nullptr) const;

    // Writes all categories to the file (one per line). Returns false if a category is invalid or the file
    // cannot be written.
    bool save(const std::vector<ProductCategory> &categories, std::string *error = nullptr) const;

private:
    std::string m_path; // path of PCategory.txt
};
