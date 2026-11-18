# Product module (standard C++17)

This module uses only the C++ standard library. It does not include Qt headers and can be compiled/tested independently of the GUI.

- `Product`: product data and validation rules.
- `ProductManager`: catalogue loading, lookup, uniqueness checks, and stock reservation.
- `FileIO`: text-file persistence for product, order, order-detail, and bill records.

## Product.txt format

Each record is exactly ten space-separated fields followed by `#`:

`productId barcode acronym name measurementUnit unitPrice tax discount availableQuantity barcodeFilePath#`

`acronym` is limited to 8 characters. `measurementUnit` must be `count` or `weight`. Because the assignment specifies space-separated fields without quoting/escaping, `name` and `barcodeFilePath` must not contain spaces. Use a relative path where practical, for example `assets/barcodes/12345678.png`.

The project archive's `data/Product.txt` was empty, so no sample product rows have been fabricated. Add the actual product records supplied by your team/instructor.

## Standalone compile check

From the repository root (G++):

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -I Backend/include \
  Backend/src/product/Product.cpp \
  Backend/src/product/ProductManager.cpp \
  Backend/src/product/FileIO.cpp \
  path/to/your_product_test.cpp -o product_test
```
