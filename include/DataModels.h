#pragma once

#include "UiHelpers.h"

#include <string>
#include <vector>

namespace data {

struct DashboardMetrics {
    std::wstring totalRevenue;
    std::wstring totalOrders;
    std::wstring productsSold;
    std::wstring activeClients;
    std::wstring revenueCaption;
    std::wstring ordersCaption;
    std::wstring productsCaption;
    std::wstring clientsCaption;
};

struct DashboardSnapshot {
    DashboardMetrics metrics;
    std::vector<ui::SeriesPoint> revenueTrend;
    std::vector<ui::SeriesPoint> monthlyOrderVolume;
    std::vector<std::vector<std::wstring>> topProducts;
    std::vector<std::vector<std::wstring>> recentOrders;
    std::vector<std::vector<std::wstring>> lowStock;
};

struct SalesFilter {
    std::wstring search;
    std::wstring status;
    std::wstring channel;
    std::wstring fromDateIso;
    std::wstring toDateIso;
};

struct SalesRecord {
    int id{};
    std::wstring saleCode;
    std::wstring manager;
    std::wstring client;
    std::wstring product;
    std::wstring channel;
    int quantity{};
    double unitPrice{};
    double revenue{};
    double totalAmount{};
    double marginPercent{};
    std::wstring dateIso;
    std::wstring dateDisplay;
    std::wstring status;
    std::wstring paymentMethod;
    std::wstring cancellationReason;
};

struct OrderFilter {
    std::wstring search;
    std::wstring status;
    std::wstring fromDateIso;
    std::wstring toDateIso;
};

struct OrderRecord {
    int id{};
    std::wstring orderCode;
    std::wstring client;
    std::wstring product;
    int quantity{};
    double totalPrice{};
    std::wstring dateIso;
    std::wstring dateDisplay;
    std::wstring status;
    std::wstring priority;
};

struct ProductFilter {
    std::wstring search;
    std::wstring category;
    std::wstring status;
};

struct ProductRecord {
    int id{};
    std::wstring sku;
    std::wstring name;
    std::wstring category;
    int stock{};
    int reorderLevel{};
    double price{};
    double cost{};
    double discountPercent{};
    std::wstring status;
    std::wstring supplier;
    double marginPercent{};
    double profitPerUnit{};
    std::wstring expirationDateIso;
    std::wstring expirationDateDisplay;
    std::wstring batchCode;
};

struct ClientFilter {
    std::wstring search;
    std::wstring segment;
    std::wstring sort;
};

struct ClientRecord {
    int id{};
    std::wstring company;
    std::wstring contact;
    std::wstring email;
    std::wstring phone;
    std::wstring segment;
    std::wstring region;
    std::wstring lastOrderDateIso;
    std::wstring lastOrderDateDisplay;
    double lifetimeValue{};
    std::wstring accountManager;
};

struct ClientOrderRecord {
    std::wstring orderCode;
    std::wstring product;
    std::wstring dateIso;
    std::wstring dateDisplay;
    double amount{};
};

struct EmployeePerformanceRecord {
    int id{};
    std::wstring fullName;
    std::wstring role;
    int completedSales{};
    double revenue{};
    double avgMarginPercent{};
    double conversionPercent{};
    std::wstring status;
};

struct AccountRecord {
    int id{};
    std::wstring fullName;
    std::wstring username;
    std::wstring email;
    std::wstring role;
    std::wstring status;
    std::wstring lastLogin;
    std::wstring createdAt;
};

struct ProductExpiryRecord {
    int id{};
    std::wstring sku;
    std::wstring product;
    std::wstring category;
    std::wstring expirationDateIso;
    std::wstring expirationDateDisplay;
    int daysRemaining{};
    std::wstring status;
    std::wstring supplier;
    std::wstring batchCode;
};

struct PipelineStageRecord {
    std::wstring stage;
    int count{};
    double value{};
};

struct TaskRecord {
    int id{};
    std::wstring title;
    std::wstring assignedTo;
    std::wstring dueDateIso;
    std::wstring dueDateDisplay;
    std::wstring status;
    std::wstring priority;
};

struct AuditRecord {
    int id{};
    std::wstring entityType;
    int entityId{};
    std::wstring action;
    std::wstring userName;
    std::wstring details;
    std::wstring createdAt;
};

struct DatabaseOverview {
    bool connected{};
    std::wstring engineVersion;
    std::wstring databasePath;
    std::wstring lastSeededAt;
    int usersCount{};
    int clientsCount{};
    int productsCount{};
    int ordersCount{};
    int salesCount{};
};

struct AppSettings {
    std::wstring theme;
    std::wstring language;
    std::wstring dateFormat;
    std::wstring appName;
    std::wstring autoRefreshMinutes;
    std::wstring startupPage;
};

} // namespace data
