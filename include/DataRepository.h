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
    std::vector<ClientRecord> LoadClients(const ClientFilter& filter);
    std::vector<ClientOrderRecord> LoadClientOrderHistory(int clientId);
    std::vector<EmployeePerformanceRecord> LoadEmployeePerformance();
    std::vector<AccountRecord> LoadAccounts();
    std::vector<ProductExpiryRecord> LoadProductExpiryReport();
    std::vector<PipelineStageRecord> LoadPipelineSummary();
    std::vector<TaskRecord> LoadOpenTasks();
    std::vector<AuditRecord> LoadAuditLog(int limit = 80);
    DatabaseOverview LoadDatabaseOverview();

    AppSettings LoadSettings();
    bool SaveSettings(const AppSettings& settings);

    std::wstring ExportSalesCsv(const SalesFilter& filter);
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
    bool AddDemoSale();
    bool AdvanceSaleStatus(int saleId);
    bool DeleteSale(int saleId);
    bool AddDemoOrder();
    bool AdvanceOrderStatus(int orderId);
    bool DeleteOrder(int orderId);
    bool AddDemoProduct();
    bool UpdateDemoProduct(int productId);
    bool RemoveProduct(int productId);
    bool ImportProductsCsv(const std::wstring& filePath, int* importedCount = nullptr);
    bool CreateQuickEntry(const std::wstring& entryType, const std::wstring& primaryValue, const std::wstring& secondaryValue, const std::wstring& quantityValue, const std::wstring& amountValue);
    bool AddDemoClient();
    bool UpdateDemoClient(int clientId);
    bool DeleteClient(int clientId);
    bool AddDemoAccount();
    bool ToggleAccountStatus(int userId);
    bool ResetAccountPassword(int userId);
    bool CycleAccountRole(int userId);
    bool DeleteAccount(int userId);
    bool AuthenticateUser(const std::wstring& username, const std::wstring& password, AccountRecord* account = nullptr);
    bool RegisterAccount(const std::wstring& fullName, const std::wstring& username, const std::wstring& email, const std::wstring& password);
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
    bool UpdateSetting(const std::wstring& key, const std::wstring& value);
    std::wstring ReadSetting(const std::wstring& key, const std::wstring& fallback = L"");
    std::wstring EnsureExportsDirectory() const;
    std::wstring DatabasePath() const;
    bool LogAudit(const std::wstring& entityType, int entityId, const std::wstring& action, const std::wstring& details);
    void SetLastError(const std::wstring& error);

    SqliteDatabase db_;
    bool initialized_;
    bool hasCurrentAccount_;
    AccountRecord currentAccount_;
    std::wstring lastError_;
};

} // namespace data
