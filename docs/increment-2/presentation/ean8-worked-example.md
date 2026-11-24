# Explain a scanned EAN-8 code

Use the milk catalogue entry `40001029` to explain how a picture becomes a
product lookup. This is a worked source example, not a recorded camera test.

## Check the eighth digit

EAN-8 uses seven data digits followed by one check digit. The implementation
weights the first seven positions alternately by three and one.

| Position | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Digit | 4 | 0 | 0 | 0 | 1 | 0 | 2 |
| Weight | 3 | 1 | 3 | 1 | 3 | 1 | 3 |
| Product | 12 | 0 | 0 | 0 | 3 | 0 | 6 |

The weighted sum is 21. The check digit is `(10 - (21 % 10)) % 10 = 9`.
Therefore `40001029` passes the checksum. A checksum-valid code can still be
absent from the catalogue; product lookup is a separate operation.

## Explain the bars as 67 modules

A module is the narrow unit used to express bar and space widths. Here `1`
means a black module and `0` a white module. Guards separate the digit groups.

```text
start  left digits: 4       0       0       0       center
101                 0100011 0001101 0001101 0001101 01010

right digits: 1       0       2       9       end
              1100110 1110010 1101100 1110100 101
```

The layout contains `3 + (4 * 7) + 5 + (4 * 7) + 3 = 67` modules.
Right-side digit patterns are the bitwise complements of the left-side
patterns. The decoder measures 43 alternating stripe runs, estimates a
module width, rounds each run to one through four modules, and validates
the guards and digit patterns before checking the checksum.

## Connect decoding to the POS

The historical API name `decodeSix()` returns the full **eight-digit** code.
The output `40001029` matches the barcode field in `Product.txt`; the GUI can
then preview the product and explicitly add it to an order. The method name
does not mean that the current catalogue uses six-digit barcode keys.

The main POS does not expose a barcode-generation screen. The unregistered
older regression file references generator APIs absent from the current tree.
Use the existing catalogue PNG when explaining this example.

Sources: [decoder](../../../Backend/src/scanner/decoder.cpp),
[decoder contract](../../../Backend/include/scanner/decoder.h),
[EAN-8 helper](../../../Backend/include/product/Ean8.h),
[catalogue](../../../data/Product.txt), and
[legacy regression source](../../../tests/regression.cpp).

## Compare the decoder with catalogue validation

Change only the final digit of the example to produce 40001028. Its first
seven digits still require check digit 9, so the camera decoder rejects that
candidate. The catalogue's BarcodeRule instead reports a checksum warning and
can retain a row with an eight-digit numeric code that has the wrong checksum.

Consequently a product reachable through typed catalogue lookup need not be
readable from a barcode image with that same invalid check digit. This is a
difference between the implemented validators, not a claim that malformed
labels were tested on hardware. See
[BarcodeRule](../../../Backend/src/product/ProductValidation.cpp) and
[decoder checksum validation](../../../Backend/src/scanner/decoder.cpp).
