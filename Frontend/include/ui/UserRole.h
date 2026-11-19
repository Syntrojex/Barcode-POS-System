#pragma once
#include <QString>

// Who is using the terminal. Chosen once at start-up.
enum class UserRole
{
    Cashier, // records orders, generates bills, views products
    Manager  // views products only (Products page + scanning a barcode for its description)
};

// Single place that decides what each role may open (SRP).
// Adding a role or changing a permission only touches this file (OCP for the rest of the UI).
// Page numbers are MainWindow::Page values:
// Dashboard 0, RecordOrder 1, Orders 2, Products 3, Scanner 4, Bills 5, Settings 6.
namespace RolePolicy
{
inline bool canOpen(UserRole role, int page)
{
    if (role == UserRole::Cashier)
        return true;
    return page == 3 || page == 4; // Manager: Products, Barcode Scanner
}

// First page shown after signing in.
inline int startPage(UserRole role)
{
    return role == UserRole::Manager ? 3 : 1;
}

inline QString name(UserRole role)
{
    return role == UserRole::Manager ? QStringLiteral("Manager") : QStringLiteral("Cashier");
}
} // namespace RolePolicy
