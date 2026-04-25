#pragma once

#include "DataModels.h"

#include <string>

namespace access {

enum class AppRole {
    SystemAdministrator,
    Director,
    SalesSupervisor,
    SalesManager,
    ProductManager
};

enum class DataScope {
    None,
    Own,
    Team,
    All
};

enum class Permission {
    OpenDashboard,
    OpenSales,
    OpenOrders,
    OpenProducts,
    OpenClients,
    OpenEmployees,
    OpenAnalytics,
    OpenReports,
    OpenSettings,

    ViewCompanyAnalytics,
    ViewEmployeePerformance,
    ViewAuditLog,

    CreateClient,
    EditClient,
    DeleteClient,

    CreateSale,
    ChangeSaleStatus,
    DeleteSale,
    ExportSales,

    CreateOrder,
    ChangeOrderStatus,
    DeleteOrder,

    CreateProduct,
    EditProduct,
    DeleteProduct,
    ImportExportCatalog,

    ExportReports,
    ManageTasks,
    ManageUsers,
    ManageRoles,
    ManageSystemSettings,
    ClearBusinessData
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

AppRole RoleFromText(const std::wstring& role);
AppRole RoleForAccount(const data::AccountRecord& account);
std::wstring CanonicalRoleName(AppRole role);
std::wstring RoleInterfaceName(AppRole role);

bool HasPermission(AppRole role, Permission permission);
bool HasPermission(const data::AccountRecord& account, Permission permission);
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

} // namespace access
