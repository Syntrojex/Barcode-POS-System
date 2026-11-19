# Backend viva practice

Inspection began at `1b16a70`; final verification used revision `883425c`.
Backend CTest passed 5/5 and the Qt GUI built with `CAMERA=OFF`.
Interactive GUI checkout and hardware camera operation remain untested.

## Questions about the design

**Why separate Product, Order, and Billing?** Product owns catalogue values,
lookup and stock; Order owns cart rules and order records; Billing owns totals,
payment selection and bill records. Each can be exercised without Qt.
See [CMake targets](../../../CMakeLists.txt).

**Where is the Single Responsibility Principle visible?**
[BillCalculator](../../../Backend/src/billing/BillCalculator.cpp) calculates;
[BillRecordCodec](../../../Backend/src/billing/BillRecordCodec.cpp) translates
text records; [BillRepository](../../../Backend/src/billing/BillRepository.cpp)
loads/saves them. Changing display styling does not require these components.

**What does the adapter do?**
[ProductAdapter](../../../Backend/src/integration/ProductAdapter.cpp) copies a
catalogue `Product` into `ProductSnapshot`, including converting its integer ID
to text. Order code therefore does not include Product's implementation.

**How is Billing independent of the catalogue?** Its constructor receives a
`ProductLookup` callback. The GUI supplies catalogue/manual-item lookup and tests
can supply a synthetic product. See
[BillingService](../../../Backend/include/billing/BillingService.h).

**Where is the Open/Closed Principle visible, and what is its limit?**
[IProductRule and IStockPolicy](../../../Backend/include/product/ProductValidation.h)
permit new validation/policy implementations. Payment strategies share
[IPaymentMethod](../../../Backend/include/billing/PaymentMethod.h).
A truly new payment mode still needs changes to the `PaymentMode` enum,
serialization and GUI selection; the registry alone does not remove those changes.

## Questions about actual behavior

**Does creating an order reduce stock?** `OrderManager::createOrder` saves an
in-progress order and clears its cart. Qt's
[AppState::addOrder](../../../Frontend/src/ui/AppState.cpp) separately saves
stock and invokes billing. Backend checkout by itself does not own inventory.

**Are tax and discount percentages?** Both are PKR amounts per unit. Two units
priced at 250 with tax 12.50 and discount 10 total `500 + 25 - 20 = 505`.

**Does a card payment contact a bank?** No external payment gateway is called.
[Card/wallet strategies](../../../Backend/src/billing/PaymentMethod.cpp)
validate merchant text locally; cash validates tender and calculates change.

**Can weighted products use fractions?** Backend order quantities are `double`
and allow fractions for `weight`. Catalogue stock and current UI quantities are
integers, so fractional checkout is not implemented throughout the GUI.

**Are text files a transaction system?** Writers use temporary files and attempt
rollback. Files are updated separately and no journal/process lock coordinates
them. Describe recovery as best effort, not crash-safe atomic checkout.

**What do passing tests prove?** The five registered suites exercise backend
rules and normal file persistence. They do not prove camera operation or GUI
checkout. See [verification](../verification.md) and [coverage](../../../tests/checkout_test.cpp).

## Trace one catalogue purchase aloud

For milk product ID 1, ProductManager finds the catalogue object using barcode
40001029. ProductAdapter::toSnapshot copies ID, price, tax, discount, unit,
and available stock into the order representation. The backend cart validates
the requested and combined quantity before createOrder saves detail/header
records. BillingService::preview resolves the saved product ID again and
calculates a bill; checkout validates payment, marks paid and appends the bill.
In the GUI, AppState additionally coordinates the inventory save and rollback.

The boundary matters: neither a successful lookup nor a successful order save
means a finalized sale exists. Point to the paid order and its associated bill
when explaining completion. Stock persistence is an additional GUI-owned step.
