# Persistent text records

The application uses four local text files. Records are whitespace-delimited and
end with `#`; blank lines are skipped by the normal loaders.
The layouts below name logical fields in their exact stored order.

## Product.txt

```text
productId barcode acronym productName unit unitPrice taxPerUnit discountPerUnit availableQuantity barcodePath#
```

The active [`ProductParser`](../../Backend/src/product/ProductStorage.cpp) accepts
a multiword name by reading the six trailing fields from the right. Quantity is
an integer, including for a `weight` product. The barcode path is one token.
The separate `ProductRecordCodec` requires exactly ten tokens; it does not share
the active parser's multiword-name behavior. Product names are not percent-encoded.

## Order.txt and OrderDetail.txt

```text
orderId date time customerName customerContact status paymentAmount paymentMode paymentMerchant #
orderId productId productBarcode quantity #
```

The first layout belongs to `Order.txt`; the second belongs to `OrderDetail.txt`.
Orders require nine fields, details four. Orders have eight-digit IDs using
`YYMM` + daily sequence `01..99` + `DD`; dates/times use `YYYY-MM-DD`/`HH:MM:SS`.
Details save product identity and quantity, not a historical price snapshot.
[`OrderManager`](../../Backend/src/order/OrderManager.cpp) formats money to two
decimals and quantity to three decimals with trailing zeros removed.

## Bill.txt

```text
billId customerName orderId productAcronym unitPrice discountPerUnit quantity withoutTax totalTax totalDiscount payable paymentStatus #
```

There are twelve fields and a terminator. Each item occupies one row; all rows of
one bill repeat its ID, customer, order, aggregate totals, and status.
Bill IDs use `YYMMDD` + daily sequence `001..999`.
[`BillRepository`](../../Backend/src/billing/BillRepository.cpp) groups rows by ID.
The schema saves total tax but omits per-item tax, cash tender, and change.

## Text escaping and validation limits

Order text fields and bill customer/acronym/status fields use the same convention:
space becomes `_`, literal `_` becomes `%5F`, `%` becomes `%25`, and `#` becomes
`%23`. Empty text is `-`; a literal `-` is `%2D`. Tabs/newlines normalize to spaces
on round trip. See [`RecordText`](../../Backend/src/common/RecordText.cpp).

Order loading checks field count, numbers, enums, record validity, and duplicate IDs.
Bill parsing checks its terminator, field count, nine-digit bill ID, encoding, and
finite numbers; it does not fully check statuses, amount signs, order IDs, or totals.
Grouping bill rows does not check that repeated headers agree.
Temporary-file writes and recovery do not form a transaction across all four files.
There is no multi-process locking; avoid concurrent writers and keep data backups.
