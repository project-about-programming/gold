#include "AccessControl.h"

#include <algorithm>
#include <cwctype>

namespace access {
namespace {

std::wstring Lower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return value;
}

bool Contains(const std::wstring& haystack, const std::wstring& needle) {
    return haystack.find(needle) != std::wstring::npos;
}

bool IsAdmin(AppRole role) { return role == AppRole::SystemAdministrator; }
bool IsDirector(AppRole role) { return role == AppRole::Director; }
bool IsSupervisor(AppRole role) { return role == AppRole::SalesSupervisor; }
bool IsSalesManager(AppRole role) { return role == AppRole::SalesManager; }
bool IsProductManager(AppRole role) { return role == AppRole::ProductManager; }

} // namespace

AppRole RoleFromText(const std::wstring& role) {
    const std::wstring value = Lower(role);
    if (Contains(value, L"admin") || Contains(value, L"administrator") || Contains(value, L"администратор")) {
        return AppRole::SystemAdministrator;
    }
    if (Contains(value, L"director") || Contains(value, L"executive") || Contains(value, L"директор")) {
        return AppRole::Director;
    }
    if (Contains(value, L"supervisor") || Contains(value, L"head of sales") || Contains(value, L"regional sales manager")
        || Contains(value, L"руководитель")) {
        return AppRole::SalesSupervisor;
    }
    if (Contains(value, L"procurement") || Contains(value, L"product manager") || Contains(value, L"inventory")
        || Contains(value, L"товаровед")) {
        return AppRole::ProductManager;
    }
    if (Contains(value, L"analyst")) {
        return AppRole::Director;
    }
    if (Contains(value, L"sales") || Contains(value, L"account manager") || Contains(value, L"operator")
        || Contains(value, L"менеджер")) {
        return AppRole::SalesManager;
    }
    return AppRole::SalesManager;
}

AppRole RoleForAccount(const data::AccountRecord& account) {
    return RoleFromText(account.role);
}

std::wstring CanonicalRoleName(AppRole role) {
    switch (role) {
    case AppRole::SystemAdministrator: return L"System Administrator";
    case AppRole::Director: return L"Director";
    case AppRole::SalesSupervisor: return L"Head of Sales";
    case AppRole::SalesManager: return L"Sales Manager";
    case AppRole::ProductManager: return L"Product Manager";
    }
    return L"Sales Manager";
}

std::wstring RoleInterfaceName(AppRole role) {
    switch (role) {
    case AppRole::SystemAdministrator: return L"Admin Console";
    case AppRole::Director: return L"Executive Workspace";
    case AppRole::SalesSupervisor: return L"Team Sales Workspace";
    case AppRole::SalesManager: return L"Sales Manager Workspace";
    case AppRole::ProductManager: return L"Inventory Workspace";
    }
    return L"Sales Manager Workspace";
}

bool HasPermission(AppRole role, Permission permission) {
    if (IsAdmin(role)) {
        return true;
    }

    switch (permission) {
    case Permission::OpenDashboard:
        return IsAdmin(role) || IsDirector(role) || IsSupervisor(role) || IsSalesManager(role);
    case Permission::OpenSales:
        return IsAdmin(role) || IsDirector(role) || IsSupervisor(role) || IsSalesManager(role);
    case Permission::OpenOrders:
        return IsAdmin(role) || IsSupervisor(role);
    case Permission::OpenProducts:
        return IsAdmin(role) || IsDirector(role) || IsSupervisor(role) || IsSalesManager(role) || IsProductManager(role);
    case Permission::OpenClients:
        return IsAdmin(role) || IsDirector(role) || IsSupervisor(role) || IsSalesManager(role);
    case Permission::OpenEmployees:
        return IsAdmin(role) || IsDirector(role) || IsSupervisor(role);
    case Permission::OpenAnalytics:
        return IsAdmin(role) || IsDirector(role) || IsSupervisor(role);
    case Permission::OpenReports:
        return IsAdmin(role) || IsDirector(role) || IsSupervisor(role) || IsProductManager(role);
    case Permission::OpenSettings:
        return IsAdmin(role);

    case Permission::ViewCompanyAnalytics:
        return IsAdmin(role) || IsDirector(role) || IsSupervisor(role);
    case Permission::ViewEmployeePerformance:
        return IsAdmin(role) || IsDirector(role) || IsSupervisor(role);
    case Permission::ViewAuditLog:
        return IsAdmin(role);

    case Permission::CreateClient:
        return IsSalesManager(role);
    case Permission::EditClient:
        return IsSalesManager(role);
    case Permission::DeleteClient:
        return false;

    case Permission::CreateSale:
        return IsSalesManager(role);
    case Permission::ChangeSaleStatus:
        return IsSalesManager(role) || IsSupervisor(role);
    case Permission::DeleteSale:
        return false;
    case Permission::ExportSales:
        return IsDirector(role) || IsSupervisor(role);

    case Permission::CreateOrder:
        return false;
    case Permission::ChangeOrderStatus:
        return IsSupervisor(role);
    case Permission::DeleteOrder:
        return false;

    case Permission::CreateProduct:
    case Permission::EditProduct:
    case Permission::DeleteProduct:
    case Permission::ImportExportCatalog:
        return IsProductManager(role);

    case Permission::ExportReports:
        return IsDirector(role) || IsSupervisor(role);
    case Permission::ManageTasks:
        return IsSupervisor(role);
    case Permission::ManageUsers:
    case Permission::ManageRoles:
    case Permission::ManageSystemSettings:
    case Permission::ClearBusinessData:
        return IsAdmin(role);
    }
    return false;
}

bool HasPermission(const data::AccountRecord& account, Permission permission) {
    return HasPermission(RoleForAccount(account), permission);
}

bool CanOpenPage(AppRole role, int pageIndex) {
    switch (pageIndex) {
    case kPageDashboard: return HasPermission(role, Permission::OpenDashboard);
    case kPageSales: return HasPermission(role, Permission::OpenSales);
    case kPageOrders: return HasPermission(role, Permission::OpenOrders);
    case kPageProducts: return HasPermission(role, Permission::OpenProducts);
    case kPageClients: return HasPermission(role, Permission::OpenClients);
    case kPageEmployees: return HasPermission(role, Permission::OpenEmployees);
    case kPageAnalytics: return HasPermission(role, Permission::OpenAnalytics);
    case kPageReports: return HasPermission(role, Permission::OpenReports);
    case kPageSettings: return HasPermission(role, Permission::OpenSettings);
    }
    return false;
}

bool CanOpenPage(const data::AccountRecord& account, int pageIndex) {
    return CanOpenPage(RoleForAccount(account), pageIndex);
}

int StartupPage(AppRole role) {
    if (IsAdmin(role)) return kPageSettings;
    if (IsDirector(role)) return kPageDashboard;
    if (IsSupervisor(role)) return kPageDashboard;
    if (IsSalesManager(role)) return kPageSales;
    if (IsProductManager(role)) return kPageProducts;
    return kPageDashboard;
}

int StartupPage(const data::AccountRecord& account) {
    return StartupPage(RoleForAccount(account));
}

DataScope SalesScope(AppRole role) {
    if (IsSalesManager(role)) return DataScope::Own;
    if (IsSupervisor(role)) return DataScope::Team;
    if (IsDirector(role)) return DataScope::All;
    return DataScope::None;
}

DataScope ClientScope(AppRole role) {
    if (IsSalesManager(role)) return DataScope::Own;
    if (IsSupervisor(role)) return DataScope::Team;
    if (IsDirector(role)) return DataScope::All;
    return DataScope::None;
}

DataScope TaskScope(AppRole role) {
    if (IsSalesManager(role)) return DataScope::Own;
    if (IsSupervisor(role)) return DataScope::Team;
    return DataScope::None;
}

std::wstring PageTitle(AppRole role, int pageIndex) {
    if (pageIndex == kPageDashboard && IsDirector(role)) return L"Executive Dashboard";
    if (pageIndex == kPageDashboard && IsSupervisor(role)) return L"Team Dashboard";
    if (pageIndex == kPageDashboard && IsSalesManager(role)) return L"My Dashboard";
    if (pageIndex == kPageSales && IsSalesManager(role)) return L"My Sales";
    if (pageIndex == kPageSales && IsSupervisor(role)) return L"Team Sales";
    if (pageIndex == kPageProducts && IsSalesManager(role)) return L"Product Catalog";
    if (pageIndex == kPageProducts && IsProductManager(role)) return L"Inventory";
    if (pageIndex == kPageClients && IsSalesManager(role)) return L"My Clients";
    if (pageIndex == kPageEmployees && IsSupervisor(role)) return L"Managers";
    if (pageIndex == kPageReports && IsAdmin(role)) return L"Reports";
    if (pageIndex == kPageSettings && IsAdmin(role)) return L"Users & System Settings";
    return L"";
}

std::wstring PageNavLabel(AppRole role, int pageIndex) {
    if (pageIndex == kPageSettings && IsAdmin(role)) return L"Settings";
    if (pageIndex == kPageReports && IsAdmin(role)) return L"Reports";
    if (pageIndex == kPageDashboard && IsDirector(role)) return L"Executive";
    if (pageIndex == kPageDashboard && IsSupervisor(role)) return L"Team Dashboard";
    if (pageIndex == kPageDashboard && IsSalesManager(role)) return L"My Dashboard";
    if (pageIndex == kPageSales && IsSalesManager(role)) return L"My Sales";
    if (pageIndex == kPageSales && IsSupervisor(role)) return L"Team Sales";
    if (pageIndex == kPageClients && IsSalesManager(role)) return L"My Clients";
    if (pageIndex == kPageProducts && IsSalesManager(role)) return L"Catalog";
    if (pageIndex == kPageProducts && IsProductManager(role)) return L"Inventory";
    return L"";
}

std::wstring PermissionDeniedMessage(Permission) {
    return L"Access denied for the current user role.";
}

} // namespace access
