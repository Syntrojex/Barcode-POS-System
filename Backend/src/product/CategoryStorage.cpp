#include "CategoryStorage.h"

#include <fstream>
#include <sstream>

using namespace std;

// ---------------------------------------------------------------- small helper functions

// Removes spaces, tabs and line-end characters from both ends of a string.
static string trim(const string &s)
{
    int start = 0;
    int end = (int)s.size() - 1;
    while (start <= end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n'))
        start++;
    while (end >= start && (s[end] == ' ' || s[end] == '\t' || s[end] == '\r' || s[end] == '\n'))
        end--;
    return s.substr(start, end - start + 1);
}

// True if s is exactly two digits, e.g. "10".
static bool isTwoDigits(const string &s)
{
    return s.size() == 2 && s[0] >= '0' && s[0] <= '9' && s[1] >= '0' && s[1] <= '9';
}

// True if s contains the character c.
static bool contains(const string &s, char c)
{
    return s.find(c) != string::npos;
}

// Splits a line into words (separated by one or more spaces).
static vector<string> splitWords(const string &line)
{
    vector<string> words;
    stringstream in(line);
    string word;
    while (in >> word)
        words.push_back(word);
    return words;
}

// ---------------------------------------------------------------- CategoryRepository

CategoryRepository::CategoryRepository(string filePath)
{
    m_path = filePath;
}

string CategoryRepository::validate(const ProductCategory &c)
{
    if (!isTwoDigits(c.id) || c.id == "00")
        return "Category ID must be 2 digits from 01 to 99.";
    if (trim(c.name) == "")
        return "Category name is required.";
    if (contains(c.name, '#'))
        return "Category name cannot contain '#'.";
    if (c.acronym == "" || contains(c.acronym, ' ') || contains(c.acronym, '#'))
        return "Category acronym must be one word without spaces or '#'.";
    return ""; // valid
}

string CategoryRepository::encode(const ProductCategory &c)
{
    return c.id + " " + trim(c.name) + " " + c.acronym + "#";
}

bool CategoryRepository::decode(const string &rawLine, ProductCategory &out, string *error)
{
    string line = trim(rawLine);

    // 1. the line must end with '#'
    if (line == "" || line[line.size() - 1] != '#')
    {
        if (error)
            *error = "line must end with '#'";
        return false;
    }
    line = line.substr(0, line.size() - 1); // remove the '#'

    // 2. we need at least 3 words: id, name (one or more words), acronym
    vector<string> words = splitWords(line);
    if (words.size() < 3)
    {
        if (error)
            *error = "expected 3 fields: categoryId categoryName categoryAcronym";
        return false;
    }

    // 3. first word = id, last word = acronym, everything in between = name
    ProductCategory c;
    c.id = words[0];
    c.acronym = words[words.size() - 1];
    for (int i = 1; i < (int)words.size() - 1; i++)
    {
        if (c.name != "")
            c.name += " ";
        c.name += words[i];
    }

    // 4. check the values
    string problem = validate(c);
    if (problem != "")
    {
        if (error)
            *error = problem;
        return false;
    }
    out = c;
    return true;
}

bool CategoryRepository::exists() const
{
    ifstream file(m_path);
    return file.is_open();
}

vector<ProductCategory> CategoryRepository::load(vector<string> *errors) const
{
    vector<ProductCategory> categories;
    ifstream file(m_path);
    if (!file.is_open())
        return categories; // no file yet: no categories

    string line;
    int lineNumber = 0;
    while (getline(file, line))
    {
        lineNumber++;
        if (trim(line) == "")
            continue; // skip empty lines

        ProductCategory c;
        string why;
        if (!decode(line, c, &why))
        {
            if (errors)
                errors->push_back("PCategory.txt line " + to_string(lineNumber) + ": " + why);
            continue;
        }

        // the same id may appear only once (linear search through what we have read so far)
        bool repeated = false;
        for (int i = 0; i < (int)categories.size(); i++)
            if (categories[i].id == c.id)
                repeated = true;
        if (repeated)
        {
            if (errors)
                errors->push_back("PCategory.txt line " + to_string(lineNumber) + ": category id " + c.id +
                                  " appears twice");
            continue;
        }
        categories.push_back(c);
    }
    return categories;
}

bool CategoryRepository::save(const vector<ProductCategory> &categories, string *error) const
{
    // check every category first, so a bad one never reaches the file
    for (int i = 0; i < (int)categories.size(); i++)
    {
        string problem = validate(categories[i]);
        if (problem != "")
        {
            if (error)
                *error = "category " + categories[i].id + ": " + problem;
            return false;
        }
    }

    ofstream file(m_path); // opening for writing empties the old file
    if (!file.is_open())
    {
        if (error)
            *error = "cannot write " + m_path;
        return false;
    }
    for (int i = 0; i < (int)categories.size(); i++)
        file << encode(categories[i]) << "\n";
    return true;
}
