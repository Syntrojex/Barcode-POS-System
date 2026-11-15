
#include "ProductManager.h"

#include <fstream>
#include <sstream>

bool ProductManager::loadProducts(string filename)
{
    ifstream file(filename);

    if (!file.is_open())
    {
        cout << "Unable to open Product.txt\n";
        return false;
    }

    vector<Product> tempProducts;

    string line;
    int lineNumber = 0;

    while (getline(file, line))
    {
        lineNumber++;

        if (line.empty())
            continue;

        // Check end marker
        if (line.back() != '#')
        {
            cout << "Missing # at line " << lineNumber << endl;
            file.close();
            return false;
        }

        // Remove #
        line.pop_back();

        stringstream ss(line);

        int id;
        string barcode, acronym, name, unit;
        double price, tax, discount;
        int quantity;
        string barcodePath;

        // Read all 10 fields
        if (!(ss >> id >> barcode >> acronym >> name
                 >> unit >> price >> tax >> discount
                 >> quantity >> barcodePath))
        {
            cout << "Invalid record at line "
                 << lineNumber << endl;

            file.close();
            return false;
        }

        // Check for extra data
        string extra;

        if (ss >> extra)
        {
            cout << "Extra data at line "
                 << lineNumber << endl;

            file.close();
            return false;
        }

        // Validate product data
        if (id <= 0 || price < 0 || tax < 0 ||
            discount < 0 || quantity < 0 ||
            acronym.length() > 8 ||
            barcode.empty() || name.empty() ||
            barcodePath.empty() ||
            (unit != "count" && unit != "weight"))
        {
            cout << "Invalid product information at line "
                 << lineNumber << endl;

            file.close();
            return false;
        }

        // Check duplicate ID and barcode
        bool duplicate = false;

        for (int i = 0; i < tempProducts.size(); i++)
        {
            if (tempProducts[i].getProductID() == id ||
                tempProducts[i].getBarcode() == barcode)
            {
                duplicate = true;
                break;
            }
        }

        if (duplicate)
        {
            cout << "Duplicate product at line "
                 << lineNumber << endl;

            file.close();
            return false;
        }

        // Create Product object
        Product p(id, barcode, acronym, name, unit,
                  price, tax, discount, quantity, barcodePath);

        tempProducts.push_back(p);
    }

    file.close();

    // Store products after successful parsing
    products = tempProducts;

    cout << products.size() << " products loaded successfully.\n";

    return true;
}

// Search using barcode
Product* ProductManager::searchByBarcode(string barcode)
{
    for (int i = 0; i < products.size(); i++)
    {
        if (products[i].getBarcode() == barcode)
        {
            return &products[i];
        }
    }

    return nullptr;
}

// Search using product ID
Product* ProductManager::searchByID(int id)
{
    for (int i = 0; i < products.size(); i++)
    {
        if (products[i].getProductID() == id)
        {
            return &products[i];
        }
    }

    return nullptr;
}

// Return all products
vector<Product>& ProductManager::getAllProducts()
{
    return products;
}

// Return number of products
int ProductManager::getProductCount()
{
    return products.size();
}

// Clear product collection
void ProductManager::clearProducts()
{
    products.clear();
}
