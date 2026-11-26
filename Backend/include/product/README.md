# Product module (standard C++17)

This module uses only the C++ standard library. It does not include Qt headers and can be compiled and tested
independently of the GUI.

- `ProductModel.h/.cpp`: the `Product` class (including its Category Id), measurement units, EAN-8 checks,
  `ProductDescriber` and text formatting.
- `ProductValidation.h/.cpp`: the product rules (`ProductValidator`) and stock checks (`StockValidator`).
- `ProductStorage.h/.cpp`: reading and writing `Product.txt`, and the in-memory catalogue `ProductManager`.
- `CategoryStorage.h/.cpp`: reading and writing `PCategory.txt` (`CategoryRepository`).

## Product.txt format

One product per line, space separated, ending with `#`:

`productId barcode acronym name measurementUnit unitPrice tax discount availableQuantity barcodeFilePath categoryId#`

Example: `1 01001020 OLPMILK Olpers Full Cream Milk 1L count 340.00 0.00 10.00 42 data/barcodes/01001020.png 01#`

- `name` may contain spaces (it is everything between the acronym and the measurement unit).
- `measurementUnit` is `count` or `weight`.
- `categoryId` is 2 digits (01-99), must exist in `PCategory.txt`, and must equal the first two barcode digits.
- Lines written before Increment 3 have no `categoryId`; they are still read and take it from the barcode.

## PCategory.txt format

One category per line, space separated, ending with `#`:

`categoryId categoryName categoryAcronym#`, e.g. `10 Dairy & Milk Products DAI#`

The Category Id is the first two digits of every barcode generated for that category.

## Tests

`tests/product_tests.cpp` (CTest name `product`).
