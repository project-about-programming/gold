#pragma once

#include "DataModels.h"

#include <string>
#include <vector>

namespace access {

enum class AppRole {
    SuperAdmin,
    SystemAdministrator,
    Director,
    SalesSupervisor,
    SalesManager,
    Cashier,
    ProductManager,
    WarehouseWorker,
    Accountant,
    Analyst
};

enum class DataScope {
    None,
    Own,
    Team,
    All
};

enum class Permission {
    DashboardView,
    AnalyticsView,
    SalesView,
    SalesCreate,
    SalesEdit,
    SalesDelete,
    SalesExport,
    OrdersView,
    OrdersCreate,
    OrdersEdit,
    OrdersChangeStatus,
    OrdersDelete,
    OrdersExport,
    InventoryView,
    InventoryCreate,
    InventoryEdit,
    InventoryDelete,
    InventoryStockIn,
    InventoryStockOut,
    InventoryExport,
    CustomersView,
    CustomersCreate,
    CustomersEdit,
    CustomersDelete,
    CustomersExport,
    EmployeesView,
    EmployeesCreate,
    EmployeesEdit,
    EmployeesDelete,
    ReportsView,
    ReportsGenerate,
    ReportsExportPdf,
    ReportsExportExcel,
    UsersView,
    UsersCreate,
    UsersEdit,
    UsersDisable,
    UsersDelete,
    RolesView,
    RolesEdit,
    AuditView,
    AuditExport,
    SettingsView,
    SettingsEdit,
    SystemBackup,
    SystemRestore,
    ClearBusinessData,
    NotificationsView,
    ProfileView,

    OpenDashboard = DashboardView,
    OpenAnalytics = AnalyticsView,
    ViewCompanyAnalytics = AnalyticsView,
    OpenSales = SalesView,
    CreateSale = SalesCreate,
    ChangeSaleStatus = SalesEdit,
    DeleteSale = SalesDelete,
    ExportSales = SalesExport,
    OpenOrders = OrdersView,
    CreateOrder = OrdersCreate,
    ChangeOrderStatus = OrdersChangeStatus,
    DeleteOrder = OrdersDelete,
    OpenProducts = InventoryView,
    CreateProduct = InventoryCreate,
    EditProduct = InventoryEdit,
    DeleteProduct = InventoryDelete,
    ImportExportCatalog = InventoryExport,
    OpenClients = CustomersView,
    CreateClient = CustomersCreate,
    EditClient = CustomersEdit,
    DeleteClient = CustomersDelete,
    OpenEmployees = EmployeesView,
    ViewEmployeePerformance = EmployeesView,
    OpenReports = ReportsView,
    ExportReports = ReportsExportExcel,
    ManageUsers = UsersView,
    ManageRoles = RolesEdit,
    OpenAuditLog = AuditView,
    ViewAuditLog = AuditView,
    OpenSettings = SettingsView,
    ManageSystemSettings = SettingsEdit,
    OpenNotifications = NotificationsView,
    OpenProfile = ProfileView,
    ManageTasks
};

constexpr int kPageDashboard = 0;
constexpr int kPageSales = 1;
constexpr int kPageOrders = 2;
constexpr int kPageProducts = 3;
constexpr int kPageClients = 4;
constexpr int kPageEmployees = 5;
constexpr int kPageAnalytics = 6;
constexpr int kPageReports = 7;
constexpr int kPageSettings = 8;
constexpr int kPageAuditLog = 9;
constexpr int kPageUsersRoles = 10;
constexpr int kPageNotifications = 11;
constexpr int kPageProfile = 12;

AppRole RoleFromText(const std::wstring& role);
AppRole RoleForAccount(const data::AccountRecord& account);
std::wstring CanonicalRoleName(AppRole role);
std::wstring RoleInterfaceName(AppRole role);

bool HasPermission(AppRole role, Permission permission);
bool HasPermission(const data::AccountRecord& account, Permission permission);
bool HasPermission(AppRole role, const std::wstring& permissionCode);
bool HasPermission(const data::AccountRecord& account, const std::wstring& permissionCode);
bool CanOpenPage(AppRole role, int pageIndex);
bool CanOpenPage(const data::AccountRecord& account, int pageIndex);
int StartupPage(AppRole role);
int StartupPage(const data::AccountRecord& account);
DataScope SalesScope(AppRole role);
DataScope ClientScope(AppRole role);
DataScope TaskScope(AppRole role);

std::wstring PageTitle(AppRole role, int pageIndex);
std::wstring PageNavLabel(AppRole role, int pageIndex);
std::wstring PermissionDeniedMessage(Permission permission);
std::wstring PermissionCode(Permission permission);
std::wstring PermissionModule(Permission permission);
std::wstring PermissionDescription(Permission permission);
std::vector<Permission> AllPermissions();

} // namespace access
