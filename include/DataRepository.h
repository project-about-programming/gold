#pragma once

#include "DataModels.h"
#include "SqliteDatabase.h"
#include "AccessControl.h"

#include <string>
#include <vector>

namespace data {

class Repository {
public:
    static Repository& Instance();

    bool Initialize();
    bool ResetDemoData();
    bool ClearBusinessData();

    DashboardSnapshot LoadDashboardSnapshot();
    std::vector<SalesRecord> LoadSales(const SalesFilter& filter);
    std::vector<OrderRecord> LoadOrders(const OrderFilter& filter);
    std::vector<ProductRecord> LoadProducts(const ProductFilter& filter);
    InventorySummary LoadInventorySummary(const ProductFilter& filter);
    std::vector<CategoryRecord> LoadCategories();
    std::vector<SupplierRecord> LoadSuppliers();
    std::vector<StockMovementRecord> LoadStockMovements(int productId = 0, int limit = 80);
    std::vector<InventoryAlertRecord> LoadInventoryAlerts(bool openOnly = true, int limit = 80);
    std::vector<ClientRecord> LoadClients(const ClientFilter& filter);
    std::vector<ClientOrderRecord> LoadClientOrderHistory(int clientId);
    std::vector<EmployeePerformanceRecord> LoadEmployeePerformance();
    std::vector<AccountRecord> LoadAccounts();
    std::vector<RoleSummaryRecord> LoadRoleSummaries();
    std::vector<ProductExpiryRecord> LoadProductExpiryReport();
    std::vector<PipelineStageRecord> LoadPipelineSummary();
    std::vector<TaskRecord> LoadOpenTasks();
    std::vector<AuditRecord> LoadAuditLog(int limit = 80);
    DatabaseOverview LoadDatabaseOverview();

    AppSettings LoadSettings();
    bool SaveSettings(const AppSettings& settings);

    std::wstring ExportSalesCsv(const SalesFilter& filter);
    std::wstring ExportOrdersCsv(const OrderFilter& filter);
    std::wstring ExportProductExpiryCsv();
    std::wstring ExportDailySalesSummaryReport();
    std::wstring ExportFulfillmentReport();
    std::wstring ExportEmployeePerformanceReport();
    std::wstring ExportClientReport();
    std::wstring ExportProductProfitReport();
    std::wstring ExportCancellationReport();
    std::wstring ExportAuditLogReport();
    std::wstring ExportPdfQueueManifest();
    std::wstring ArchiveExportsSnapshot();
    std::wstring BackupDatabaseSnapshot();
    bool AddDemoSale();
    bool CreateDirectSale(const std::wstring& customer, const std::wstring& product, const std::wstring& quantity,
        const std::wstring& discountPercent, const std::wstring& paymentMethod, const std::wstring& paymentAmount);
    bool AdvanceSaleStatus(int saleId);
    bool DeleteSale(int saleId);
    bool AddDemoOrder();
    bool CreateOrder(const std::wstring& customer, const std::wstring& product, const std::wstring& quantity,
        const std::wstring& discountPercent, const std::wstring& paymentMethod, const std::wstring& paymentAmount,
        const std::wstring& notes);
    bool UpdateOrder(int orderId, const std::wstring& customer, const std::wstring& product, const std::wstring& quantity,
        const std::wstring& discountPercent, const std::wstring& paymentMethod, const std::wstring& paymentAmount,
        const std::wstring& notes);
    bool AdvanceOrderStatus(int orderId);
    bool CancelOrder(int orderId);
    bool DeleteOrder(int orderId);
    bool AddDemoProduct();
    bool UpdateDemoProduct(int productId);
    bool SaveProduct(const ProductRecord& product, bool createNew);
    bool RemoveProduct(int productId);
    bool ImportProductsCsv(const std::wstring& filePath, int* importedCount = nullptr);
    std::wstring ExportInventoryCsv(const ProductFilter& filter);
    bool AdjustProductStock(int productId, int delta, const std::wstring& reason);
    bool AdjustProductStock(int productId, int delta, const std::wstring& reason, const std::wstring& reference);
    bool CreateQuickEntry(const std::wstring& entryType, const std::wstring& primaryValue, const std::wstring& secondaryValue, const std::wstring& quantityValue, const std::wstring& amountValue);
    bool CompleteTask(int taskId);
    bool AddDemoClient();
    bool UpdateDemoClient(int clientId);
    bool DeleteClient(int clientId);
    bool AddDemoAccount();
    bool ToggleAccountStatus(int userId);
    bool ResetAccountPassword(int userId);
    bool CycleAccountRole(int userId);
    bool SetAccountRole(int userId, int roleId);
    bool DeleteAccount(int userId);
    bool AuthenticateUser(const std::wstring& username, const std::wstring& password, AccountRecord* account = nullptr);
    bool RegisterAccount(const std::wstring& fullName, const std::wstring& username, const std::wstring& email, const std::wstring& password);
    bool HasPermission(access::Permission permission) const;
    bool LogAccessDenied(const std::wstring& module, const std::wstring& details);
    bool LogLogout();
    AccountRecord CurrentAccount() const;
    bool HasCurrentAccount() const;

    bool IsConnected() const;
    const std::wstring& LastError() const;

private:
    Repository();

    bool EnsureSchema();
    bool SeedDemoData();
    bool EnsureDefaultSettings();
    bool EnsureDefaultAdminAccount();
    bool EnsureDemoAccountPasswords();
    bool EnsureDefaultRoles();
    bool EnsureInitialized();
    bool RequirePermission(access::Permission permission, const std::wstring& actionName);
    bool HasAnyPermission(access::Permission first, access::Permission second) const;
    int EnsureEmployeeForCurrentAccount();
    bool ColumnExists(const std::string& table, const std::string& column);
    bool BackfillProductExpiryData();
    bool BackfillExtendedData();
    bool BackfillLifecycleData();
    bool EnsureInventoryCatalogData();
    bool RefreshInventoryAlerts();
    bool UpdateSetting(const std::wstring& key, const std::wstring& value);
    std::wstring ReadSetting(const std::wstring& key, const std::wstring& fallback = L"");
    std::wstring EnsureExportsDirectory() const;
    std::wstring DatabasePath() const;
    int ResolveClientId(const std::wstring& value);
    int ResolveProductId(const std::wstring& value);
    bool DeductStockForCommercialFlow(int productId, int quantity, const std::wstring& reference, const std::wstring& reason);
    bool CompleteOrderIntoSale(int orderId);
    bool LogAudit(const std::wstring& entityType, int entityId, const std::wstring& action, const std::wstring& details);
    void SetLastError(const std::wstring& error);

    SqliteDatabase db_;
    bool initialized_;
    bool hasCurrentAccount_;
    AccountRecord currentAccount_;
    std::wstring lastError_;
};

} // namespace data
