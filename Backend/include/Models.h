#pragma once

// Kept so older includes still compile. Bill now lives in billing/Bill.h
// (namespace barcodehub::billing); Order / OrderDetail live in the order module.
#include "billing/Bill.h"

using Bill = barcodehub::billing::Bill;
