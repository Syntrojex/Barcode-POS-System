# Product catalogue reference

This reference describes behavior found by inspecting the Increment 2 source.
It does not establish that the GUI or camera has been run successfully.

## Responsibilities and source map

| Component | Responsibility |
| --- | --- |
| [ProductModel](../../Backend/include/product/ProductModel.h) | Product values, EAN-8 helpers, PKR text, descriptions |
| [ProductValidation](../../Backend/src/product/ProductValidation.cpp) | Composable validation rules and stock policies |
| [ProductStorage](../../Backend/src/product/ProductStorage.cpp) | Parsing, loading, catalogue searches, saving |
| [ProductAdapter](../../Backend/src/integration/ProductAdapter.cpp) | Copy product values into an order snapshot |

The product module uses standard C++17 and does not depend on Qt.
`ProductManager` owns catalogue products and indexes them by barcode and ID.
Its stock deduction changes memory; `saveProducts()` persists those values.

## Product.txt records

The active parser expects the following logical fields, ending in `#`:

```text
id barcode acronym name unit price tax discount stock barcodePath#
```

Names may contain spaces: the parser reads the final six fields from the right.
Barcode paths remain single tokens; paths containing spaces are unsupported.
Units are the lowercase words `count` and `weight`; stock is an integer.
Tax and discount are PKR amounts per unit, not percentage rates.
The first-line UTF-8 BOM and blank lines are handled by the file loader.

Malformed records are rejected individually, with line-numbered issues.
Valid records survive other rejected rows; duplicate IDs/barcodes are rejected.
`loadProducts()` can return false while retaining the valid rows in memory.
An eight-digit barcode with a bad checksum is retained with a warning.

## Lookup and description

- `searchByBarcode()` performs an exact lookup of the full barcode.
- `searchByID()` performs an integer ID lookup.
- `searchByCode()` tries barcode, numeric ID shorter than eight digits, then acronym.
- Acronym matching and text searches are case-insensitive.
- Text search covers name, acronym, and barcode; an empty query returns all products.
- Descriptions include stock status and `price - discount + tax` per unit.

The [sample catalogue](../../data/Product.txt) contains 14 product rows.
A read-only inspection found 14 valid EAN-8 checksums and 14 barcode PNG files.
For a presentation, `40001029`, `olpmilk`, and `1` select the same milk product.
Eggs (`40007359`) have stock 4; cola (`40011721`) has stock 0 in the sample.

## Boundaries to explain accurately

The implemented acronym limit is 10, although the increment plan specifies 8.
The loader does not reject duplicate acronyms; keep sample acronyms unique.
Price parsing/validation does not exclude every nonfinite numeric value;
the order layer separately rejects nonfinite amounts before adding them.
The writer uses a temporary file with a replacement fallback, not a database transaction.
The older strict `ProductRecordCodec` accepts exactly ten tokens and differs from the active parser.
See [product tests](../../tests/product_tests.cpp) for existing parser, search, and stock cases.
