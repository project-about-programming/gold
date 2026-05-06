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

bool IsSuperAdmin(AppRole role) { return role == AppRole::SuperAdmin; }
bool IsAdmin(AppRole role) { return role == AppRole::SystemAdministrator; }
bool IsDirector(AppRole role) { return role == AppRole::Director; }
bool IsSupervisor(AppRole role) { return role == AppRole::SalesSupervisor; }
bool IsSalesManager(AppRole role) { return role == AppRole::SalesManager; }
bool IsCashier(AppRole role) { return role == AppRole::Cashier; }
bool IsProductManager(AppRole role) { return role == AppRole::ProductManager; }
bool IsWarehouseWorker(AppRole role) { return role == AppRole::WarehouseWorker; }
bool IsAccountant(AppRole role) { return role == AppRole::Accountant; }
bool IsAnalyst(AppRole role) { return role == AppRole::Analyst; }

} // namespace

AppRole RoleFromText(const std::wstring& role) {
    const std::wstring value = Lower(role);
    if (Contains(value, L"superadmin") || Contains(value, L"super admin") || Contains(value, L"owner")) {
        return AppRole::SuperAdmin;
    }
    if (Contains(value, L"system administrator") || Contains(value, L"sysadmin") || Contains(value, L"сисадмин")
        || Contains(value, L"admin") || Contains(value, L"administrator") || Contains(value, L"администратор")) {
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
    if (Contains(value, L"warehouse") || Contains(value, L"stock") || Contains(value, L"склад")) {
        return AppRole::WarehouseWorker;
    }
    if (Contains(value, L"cashier") || Contains(value, L"seller") || Contains(value, L"кассир") || Contains(value, L"продавец")) {
        return AppRole::Cashier;
    }
    if (Contains(value, L"accountant") || Contains(value, L"finance") || Contains(value, L"бухгалтер")) {
        return AppRole::Accountant;
    }
    if (Contains(value, L"analyst")) {
        return AppRole::Analyst;
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
    case AppRole::SuperAdmin: return L"SuperAdmin";
    case AppRole::SystemAdministrator: return L"System Administrator";
    case AppRole::Director: return L"Director";
    case AppRole::SalesSupervisor: return L"Head of Sales";
    case AppRole::SalesManager: return L"Sales Manager";
    case AppRole::Cashier: return L"Cashier";
    case AppRole::ProductManager: return L"Product Manager";
    case AppRole::WarehouseWorker: return L"Warehouse Worker";
    case AppRole::Accountant: return L"Accountant";
    case AppRole::Analyst: return L"Analyst";
    }
    return L"Sales Manager";
}

std::wstring RoleInterfaceName(AppRole role) {
    switch (role) {
    case AppRole::SuperAdmin: return L"Full Control Workspace";
    case AppRole::SystemAdministrator: return L"Admin Console";
    case AppRole::Director: return L"Executive Workspace";
    case AppRole::SalesSupervisor: return L"Team Sales Workspace";
    case AppRole::SalesManager: return L"Sales Manager Workspace";
    case AppRole::Cashier: return L"Cashier Workspace";
    case AppRole::ProductManager: return L"Inventory Workspace";
    case AppRole::WarehouseWorker: return L"Warehouse Workspace";
    case AppRole::Accountant: return L"Finance Workspace";
    case AppRole::Analyst: return L"Analytics Workspace";
    }
    return L"Sales Manager Workspace";
}

bool HasPermission(AppRole role, Permission permission) {
    if (IsSuperAdmin(role)) {
        return true;
    }

    switch (permission) {
    case Permission::DashboardView:
        return IsAdmin(role) || IsDirector(role) || IsSupervisor(role) || IsSalesManager(role) || IsCashier(role)
            || IsProductManager(role) || IsWarehouseWorker(role) || IsAccountant(role) || IsAnalyst(role);

    case Permission::AnalyticsView:
        return IsDirector(role) || IsSupervisor(role) || IsAccountant(role) || IsAnalyst(role);

    case Permission::SalesView:
        return IsDirector(role) || IsSupervisor(role) || IsSalesManager(role) || IsCashier(role) || IsAccountant(role) || IsAnalyst(role);
    case Permission::SalesCreate:
        return IsSalesManager(role) || IsCashier(role);
    case Permission::SalesEdit:
        return IsSupervisor(role) || IsSalesManager(role);
    case Permission::SalesDelete:
        return false;
    case Permission::SalesExport:
        return IsDirector(role) || IsSupervisor(role) || IsAccountant(role) || IsAnalyst(role);

    case Permission::OrdersView:
        return IsDirector(role) || IsSupervisor(role) || IsSalesManager(role) || IsCashier(role)
            || IsWarehouseWorker(role) || IsAccountant(role) || IsAnalyst(role);
    case Permission::OrdersCreate:
        return IsSalesManager(role) || IsCashier(role);
    case Permission::OrdersEdit:
        return IsSupervisor(role) || IsSalesManager(role);
    case Permission::OrdersChangeStatus:
        return IsSupervisor(role) || IsSalesManager(role) || IsCashier(role) || IsWarehouseWorker(role);
    case Permission::OrdersDelete:
        return false;
    case Permission::OrdersExport:
        return IsDirector(role) || IsSupervisor(role) || IsAccountant(role);

    case Permission::InventoryView:
        return IsDirector(role) || IsSupervisor(role) || IsSalesManager(role) || IsProductManager(role) || IsWarehouseWorker(role) || IsAnalyst(role);
    case Permission::InventoryCreate:
    case Permission::InventoryEdit:
    case Permission::InventoryStockIn:
    case Permission::InventoryStockOut:
        return IsProductManager(role) || IsWarehouseWorker(role);
    case Permission::InventoryDelete:
    case Permission::InventoryExport:
        return IsProductManager(role) || IsWarehouseWorker(role);

    case Permission::CustomersView:
        return IsDirector(role) || IsSupervisor(role) || IsSalesManager(role) || IsCashier(role) || IsAnalyst(role);
    case Permission::CustomersCreate:
        return IsSalesManager(role) || IsCashier(role);
    case Permission::CustomersEdit:
        return IsSalesManager(role);
    case Permission::CustomersDelete:
        return false;
    case Permission::CustomersExport:
        return IsDirector(role) || IsSupervisor(role) || IsAnalyst(role);

    case Permission::EmployeesView:
        return IsDirector(role) || IsSupervisor(role);
    case Permission::EmployeesCreate:
    case Permission::EmployeesEdit:
    case Permission::EmployeesDelete:
        return false;

    case Permission::ReportsView:
        return IsDirector(role) || IsSupervisor(role) || IsProductManager(role) || IsWarehouseWorker(role) || IsAccountant(role) || IsAnalyst(role);
    case Permission::ReportsGenerate:
    case Permission::ReportsExportPdf:
    case Permission::ReportsExportExcel:
        return IsDirector(role) || IsSupervisor(role) || IsAccountant(role);

    case Permission::UsersView:
    case Permission::UsersCreate:
    case Permission::UsersEdit:
    case Permission::UsersDisable:
    case Permission::RolesView:
    case Permission::RolesEdit:
        return IsAdmin(role);
    case Permission::AuditView:
        return IsAdmin(role) || IsDirector(role);
    case Permission::AuditExport:
    case Permission::SettingsView:
    case Permission::SettingsEdit:
    case Permission::SystemBackup:
    case Permission::SystemRestore:
        return IsAdmin(role);
    case Permission::UsersDelete:
        return false;
    case Permission::ClearBusinessData:
        return IsAdmin(role);
    case Permission::NotificationsView:
        return IsAdmin(role) || IsDirector(role) || IsSupervisor(role) || IsSalesManager(role) || IsCashier(role)
            || IsProductManager(role) || IsWarehouseWorker(role) || IsAccountant(role) || IsAnalyst(role);
    case Permission::ProfileView:
        return true;
    case Permission::ManageTasks:
        return IsSupervisor(role);
    }
    return false;
}

bool HasPermission(const data::AccountRecord& account, Permission permission) {
    return HasPermission(RoleForAccount(account), permission);
}

bool HasPermission(AppRole role, const std::wstring& permissionCode) {
    for (const auto permission : AllPermissions()) {
        if (PermissionCode(permission) == permissionCode) {
            return HasPermission(role, permission);
        }
    }
    return false;
}

bool HasPermission(const data::AccountRecord& account, const std::wstring& permissionCode) {
    return HasPermission(RoleForAccount(account), permissionCode);
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
    case kPageAuditLog: return HasPermission(role, Permission::OpenAuditLog);
    case kPageUsersRoles: return HasPermission(role, Permission::UsersView) || HasPermission(role, Permission::RolesView);
    case kPageNotifications: return HasPermission(role, Permission::OpenNotifications);
    case kPageProfile: return HasPermission(role, Permission::OpenProfile);
    }
    return false;
}

bool CanOpenPage(const data::AccountRecord& account, int pageIndex) {
    return CanOpenPage(RoleForAccount(account), pageIndex);
}

int StartupPage(AppRole role) {
    if (IsSuperAdmin(role)) return kPageDashboard;
    if (IsAdmin(role)) return kPageUsersRoles;
    if (IsDirector(role)) return kPageDashboard;
    if (IsSupervisor(role)) return kPageDashboard;
    if (IsSalesManager(role)) return kPageSales;
    if (IsCashier(role)) return kPageSales;
    if (IsProductManager(role)) return kPageProducts;
    if (IsWarehouseWorker(role)) return kPageProducts;
    if (IsAccountant(role)) return kPageReports;
    if (IsAnalyst(role)) return kPageAnalytics;
    return kPageDashboard;
}

int StartupPage(const data::AccountRecord& account) {
    return StartupPage(RoleForAccount(account));
}

DataScope SalesScope(AppRole role) {
    if (IsSuperAdmin(role)) return DataScope::All;
    if (IsSalesManager(role) || IsCashier(role)) return DataScope::Own;
    if (IsSupervisor(role)) return DataScope::Team;
    if (IsDirector(role) || IsAccountant(role) || IsAnalyst(role)) return DataScope::All;
    return DataScope::None;
}

DataScope ClientScope(AppRole role) {
    if (IsSuperAdmin(role)) return DataScope::All;
    if (IsSalesManager(role) || IsCashier(role)) return DataScope::Own;
    if (IsSupervisor(role)) return DataScope::Team;
    if (IsDirector(role) || IsAnalyst(role)) return DataScope::All;
    return DataScope::None;
}

DataScope TaskScope(AppRole role) {
    if (IsSuperAdmin(role)) return DataScope::All;
    if (IsSalesManager(role) || IsCashier(role) || IsProductManager(role) || IsWarehouseWorker(role) || IsAccountant(role) || IsAnalyst(role)) return DataScope::Own;
    if (IsSupervisor(role)) return DataScope::Team;
    if (IsAdmin(role) || IsDirector(role)) return DataScope::All;
    return DataScope::None;
}

std::wstring PageTitle(AppRole role, int pageIndex) {
    if (pageIndex == kPageDashboard && IsAdmin(role)) return L"System Dashboard";
    if (pageIndex == kPageDashboard && IsDirector(role)) return L"Executive Dashboard";
    if (pageIndex == kPageDashboard && IsSupervisor(role)) return L"Team Dashboard";
    if (pageIndex == kPageDashboard && IsSalesManager(role)) return L"My Dashboard";
    if (pageIndex == kPageDashboard && IsCashier(role)) return L"Cashier Dashboard";
    if (pageIndex == kPageDashboard && (IsProductManager(role) || IsWarehouseWorker(role))) return L"Inventory Dashboard";
    if (pageIndex == kPageDashboard && IsAccountant(role)) return L"Finance Dashboard";
    if (pageIndex == kPageDashboard && IsAnalyst(role)) return L"Analytics Dashboard";
    if (pageIndex == kPageSales && IsSalesManager(role)) return L"My Sales";
    if (pageIndex == kPageSales && IsCashier(role)) return L"Point of Sale";
    if (pageIndex == kPageSales && IsSupervisor(role)) return L"Team Sales";
    if (pageIndex == kPageProducts && IsSalesManager(role)) return L"Inventory";
    if (pageIndex == kPageProducts && IsProductManager(role)) return L"Inventory";
    if (pageIndex == kPageProducts && IsWarehouseWorker(role)) return L"Warehouse Stock";
    if (pageIndex == kPageClients && IsSalesManager(role)) return L"My Customers";
    if (pageIndex == kPageEmployees && IsSupervisor(role)) return L"Managers";
    if (pageIndex == kPageReports && IsAdmin(role)) return L"Reports";
    if (pageIndex == kPageSettings && IsAdmin(role)) return L"System Settings";
    if (pageIndex == kPageAuditLog && IsAdmin(role)) return L"Audit Log";
    if (pageIndex == kPageUsersRoles && IsAdmin(role)) return L"Users & Roles";
    if (pageIndex == kPageNotifications && IsAdmin(role)) return L"System Notifications";
    if (pageIndex == kPageNotifications && (IsProductManager(role) || IsWarehouseWorker(role))) return L"Stock Alerts";
    if (pageIndex == kPageNotifications && (IsSalesManager(role) || IsCashier(role))) return L"My Tasks";
    if (pageIndex == kPageNotifications) return L"Notifications";
    if (pageIndex == kPageProfile) return L"Profile";
    return L"";
}

std::wstring PageNavLabel(AppRole role, int pageIndex) {
    if (pageIndex == kPageProfile) return L"Profile";
    if (pageIndex == kPageSettings && IsAdmin(role)) return L"Settings";
    if (pageIndex == kPageAuditLog && IsAdmin(role)) return L"Audit Log";
    if (pageIndex == kPageUsersRoles && IsAdmin(role)) return L"Users & Roles";
    if (pageIndex == kPageNotifications && IsAdmin(role)) return L"System Alerts";
    if (pageIndex == kPageNotifications && (IsSalesManager(role) || IsCashier(role))) return L"My Tasks";
    if (pageIndex == kPageNotifications) return L"Notifications";
    if (pageIndex == kPageReports && IsAdmin(role)) return L"Reports";
    if (pageIndex == kPageDashboard && IsDirector(role)) return L"Executive";
    if (pageIndex == kPageDashboard && IsSupervisor(role)) return L"Team Dashboard";
    if (pageIndex == kPageDashboard && IsSalesManager(role)) return L"My Dashboard";
    if (pageIndex == kPageSales && IsSalesManager(role)) return L"My Sales";
    if (pageIndex == kPageSales && IsCashier(role)) return L"POS";
    if (pageIndex == kPageSales && IsSupervisor(role)) return L"Team Sales";
    if (pageIndex == kPageClients && IsSalesManager(role)) return L"My Customers";
    if (pageIndex == kPageProducts && IsSalesManager(role)) return L"Catalog";
    if (pageIndex == kPageProducts && IsProductManager(role)) return L"Inventory";
    return L"";
}

std::wstring PermissionDeniedMessage(Permission) {
    return L"Access denied for the current user role.";
}

std::wstring PermissionCode(Permission permission) {
    switch (permission) {
    case Permission::DashboardView: return L"dashboard.view";
    case Permission::AnalyticsView: return L"analytics.view";
    case Permission::SalesView: return L"sales.view";
    case Permission::SalesCreate: return L"sales.create";
    case Permission::SalesEdit: return L"sales.edit";
    case Permission::SalesDelete: return L"sales.delete";
    case Permission::SalesExport: return L"sales.export";
    case Permission::OrdersView: return L"orders.view";
    case Permission::OrdersCreate: return L"orders.create";
    case Permission::OrdersEdit: return L"orders.edit";
    case Permission::OrdersChangeStatus: return L"orders.change_status";
    case Permission::OrdersDelete: return L"orders.delete";
    case Permission::OrdersExport: return L"orders.export";
    case Permission::InventoryView: return L"inventory.view";
    case Permission::InventoryCreate: return L"inventory.create";
    case Permission::InventoryEdit: return L"inventory.edit";
    case Permission::InventoryDelete: return L"inventory.delete";
    case Permission::InventoryStockIn: return L"inventory.stock_in";
    case Permission::InventoryStockOut: return L"inventory.stock_out";
    case Permission::InventoryExport: return L"inventory.export";
    case Permission::CustomersView: return L"customers.view";
    case Permission::CustomersCreate: return L"customers.create";
    case Permission::CustomersEdit: return L"customers.edit";
    case Permission::CustomersDelete: return L"customers.delete";
    case Permission::CustomersExport: return L"customers.export";
    case Permission::EmployeesView: return L"employees.view";
    case Permission::EmployeesCreate: return L"employees.create";
    case Permission::EmployeesEdit: return L"employees.edit";
    case Permission::EmployeesDelete: return L"employees.delete";
    case Permission::ReportsView: return L"reports.view";
    case Permission::ReportsGenerate: return L"reports.generate";
    case Permission::ReportsExportPdf: return L"reports.export_pdf";
    case Permission::ReportsExportExcel: return L"reports.export_excel";
    case Permission::UsersView: return L"users.view";
    case Permission::UsersCreate: return L"users.create";
    case Permission::UsersEdit: return L"users.edit";
    case Permission::UsersDisable: return L"users.disable";
    case Permission::UsersDelete: return L"users.delete";
    case Permission::RolesView: return L"roles.view";
    case Permission::RolesEdit: return L"roles.edit";
    case Permission::AuditView: return L"audit.view";
    case Permission::AuditExport: return L"audit.export";
    case Permission::SettingsView: return L"settings.view";
    case Permission::SettingsEdit: return L"settings.edit";
    case Permission::SystemBackup: return L"system.backup";
    case Permission::SystemRestore: return L"system.restore";
    case Permission::ClearBusinessData: return L"system.clear_business_data";
    case Permission::NotificationsView: return L"notifications.view";
    case Permission::ProfileView: return L"profile.view";
    case Permission::ManageTasks: return L"tasks.manage";
    }
    return L"unknown";
}

std::wstring PermissionModule(Permission permission) {
    const std::wstring code = PermissionCode(permission);
    const size_t dot = code.find(L'.');
    return dot == std::wstring::npos ? code : code.substr(0, dot);
}

std::wstring PermissionDescription(Permission permission) {
    switch (permission) {
    case Permission::DashboardView: return L"View dashboard";
    case Permission::AnalyticsView: return L"View analytics";
    case Permission::SalesView: return L"View sales";
    case Permission::SalesCreate: return L"Create sales";
    case Permission::SalesEdit: return L"Edit sales";
    case Permission::SalesDelete: return L"Delete sales";
    case Permission::SalesExport: return L"Export sales";
    case Permission::OrdersView: return L"View orders";
    case Permission::OrdersCreate: return L"Create orders";
    case Permission::OrdersEdit: return L"Edit orders";
    case Permission::OrdersChangeStatus: return L"Change order status";
    case Permission::OrdersDelete: return L"Delete orders";
    case Permission::OrdersExport: return L"Export orders";
    case Permission::InventoryView: return L"View inventory";
    case Permission::InventoryCreate: return L"Create inventory items";
    case Permission::InventoryEdit: return L"Edit inventory items";
    case Permission::InventoryDelete: return L"Delete inventory items";
    case Permission::InventoryStockIn: return L"Receive stock";
    case Permission::InventoryStockOut: return L"Write off stock";
    case Permission::InventoryExport: return L"Export inventory";
    case Permission::CustomersView: return L"View customers";
    case Permission::CustomersCreate: return L"Create customers";
    case Permission::CustomersEdit: return L"Edit customers";
    case Permission::CustomersDelete: return L"Delete customers";
    case Permission::CustomersExport: return L"Export customers";
    case Permission::EmployeesView: return L"View employees";
    case Permission::EmployeesCreate: return L"Create employees";
    case Permission::EmployeesEdit: return L"Edit employees";
    case Permission::EmployeesDelete: return L"Delete employees";
    case Permission::ReportsView: return L"View reports";
    case Permission::ReportsGenerate: return L"Generate reports";
    case Permission::ReportsExportPdf: return L"Export PDF reports";
    case Permission::ReportsExportExcel: return L"Export Excel/CSV reports";
    case Permission::UsersView: return L"View users";
    case Permission::UsersCreate: return L"Create users";
    case Permission::UsersEdit: return L"Edit users";
    case Permission::UsersDisable: return L"Disable users";
    case Permission::UsersDelete: return L"Delete users";
    case Permission::RolesView: return L"View roles";
    case Permission::RolesEdit: return L"Edit roles";
    case Permission::AuditView: return L"View audit log";
    case Permission::AuditExport: return L"Export audit log";
    case Permission::SettingsView: return L"View settings";
    case Permission::SettingsEdit: return L"Edit settings";
    case Permission::SystemBackup: return L"Create database backups";
    case Permission::SystemRestore: return L"Restore database backups";
    case Permission::ClearBusinessData: return L"Clear business tables";
    case Permission::NotificationsView: return L"View notifications";
    case Permission::ProfileView: return L"View profile";
    case Permission::ManageTasks: return L"Manage tasks";
    }
    return L"Unknown permission";
}

std::vector<Permission> AllPermissions() {
    return {
        Permission::DashboardView,
        Permission::AnalyticsView,
        Permission::SalesView, Permission::SalesCreate, Permission::SalesEdit, Permission::SalesDelete, Permission::SalesExport,
        Permission::OrdersView, Permission::OrdersCreate, Permission::OrdersEdit, Permission::OrdersChangeStatus, Permission::OrdersDelete, Permission::OrdersExport,
        Permission::InventoryView, Permission::InventoryCreate, Permission::InventoryEdit, Permission::InventoryDelete, Permission::InventoryStockIn, Permission::InventoryStockOut, Permission::InventoryExport,
        Permission::CustomersView, Permission::CustomersCreate, Permission::CustomersEdit, Permission::CustomersDelete, Permission::CustomersExport,
        Permission::EmployeesView, Permission::EmployeesCreate, Permission::EmployeesEdit, Permission::EmployeesDelete,
        Permission::ReportsView, Permission::ReportsGenerate, Permission::ReportsExportPdf, Permission::ReportsExportExcel,
        Permission::UsersView, Permission::UsersCreate, Permission::UsersEdit, Permission::UsersDisable, Permission::UsersDelete,
        Permission::RolesView, Permission::RolesEdit,
        Permission::AuditView, Permission::AuditExport,
        Permission::SettingsView, Permission::SettingsEdit,
        Permission::SystemBackup, Permission::SystemRestore, Permission::ClearBusinessData,
        Permission::NotificationsView, Permission::ProfileView, Permission::ManageTasks
    };
}

} // namespace access
