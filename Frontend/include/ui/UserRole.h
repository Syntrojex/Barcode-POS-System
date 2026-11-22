#pragma once
#include <QString>

// Who is using the terminal. Chosen once at sign-in.
enum class UserRole
{
    Cashier, // records orders, edits orders (paid ones after manager approval), views products, prints bills
    Manager  // everything a cashier can, plus categories, products, inventory, discounts and reports
};

// Sidebar pages, in sidebar order. MainWindow::Page has exactly the same values.
namespace Pages
{
    enum Id
    {
        Dashboard,
        RecordOrder,
        Orders,
        Inventory,
        Products,
        Categories,
        Scanner,
        Bills,
        Reports,
        Discounts,
        Settings,
        Count
    };
}

// Single place that decides what each role may open (SRP).
// Adding a role or changing a permission only touches this file (OCP for the rest of the UI).
namespace RolePolicy
{
inline bool isManagerOnly(int page)
{
    return page == Pages::Inventory || page == Pages::Categories || page == Pages::Reports ||
           page == Pages::Discounts;
}

inline bool canOpen(UserRole role, int page)
{
    if (page < 0 || page >= Pages::Count)
        return false;
    return role == UserRole::Manager || !isManagerOnly(page);
}

// Only a manager may add or edit products, generate barcodes and change stock (Products page is view-only for cashiers).
inline bool canEditCatalogue(UserRole role) { return role == UserRole::Manager; }

// First page shown after signing in.
inline int startPage(UserRole role)
{
    return role == UserRole::Manager ? int(Pages::Dashboard) : int(Pages::RecordOrder);
}

inline QString name(UserRole role)
{
    return role == UserRole::Manager ? QStringLiteral("Manager") : QStringLiteral("Cashier");
}

// "Terminal Manager" / "Terminal Cashier" (sidebar user card)
inline QString title(UserRole role)
{
    return role == UserRole::Manager ? QStringLiteral("Terminal Manager") : QStringLiteral("Terminal Cashier");
}
} // namespace RolePolicy
