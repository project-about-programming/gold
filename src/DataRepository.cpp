#include "DataRepository.h"

#include "AccessControl.h"
#include "Localization.h"
#include "sqlite3.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <utility>
#include <vector>

namespace data {
namespace {

struct SeedUser {
    int id;
    const wchar_t* fullName;
    const wchar_t* username;
    const wchar_t* email;
    const wchar_t* role;
    const wchar_t* status;
    const wchar_t* lastLogin;
    const wchar_t* createdAt;
};

struct SeedClient {
    int id;
    const wchar_t* company;
    const wchar_t* contact;
    const wchar_t* email;
    const wchar_t* phone;
    const wchar_t* segment;
    const wchar_t* region;
    int isActive;
    double lifetimeValue;
    const wchar_t* accountManager;
};

struct SeedProduct {
    int id;
    const wchar_t* sku;
    const wchar_t* name;
    const wchar_t* category;
    int stock;
    int reorderLevel;
    double price;
    const wchar_t* status;
    const wchar_t* supplier;
    double marginPercent;
    const wchar_t* expirationDate;
    const wchar_t* batchCode;
};

struct SeedEmployee {
    int id;
    const wchar_t* fullName;
    const wchar_t* role;
    const wchar_t* region;
    const wchar_t* status;
};

struct SeedTask {
    int id;
    const wchar_t* title;
    const wchar_t* assignedTo;
    const wchar_t* dueDate;
    const wchar_t* status;
    const wchar_t* priority;
    int relatedClientId;
};

struct SeedPipelineDeal {
    int id;
    const wchar_t* leadName;
    int clientId;
    int managerId;
    const wchar_t* stage;
    double value;
    const wchar_t* expectedCloseDate;
    const wchar_t* status;
    const wchar_t* updatedAt;
};

constexpr SeedUser kUsers[] = {
    {1, L"Anastasia Petrova", L"a.petrova", L"a.petrova@company.example", L"System Administrator", L"Active", L"2026-04-05 12:48", L"2025-01-10"},
    {2, L"Leonid Smirnov", L"l.smirnov", L"l.smirnov@company.example", L"Director", L"Active", L"2026-04-05 11:15", L"2025-01-10"},
    {3, L"Marina Ivanova", L"m.ivanova", L"m.ivanova@company.example", L"Director", L"Active", L"2026-04-05 09:32", L"2025-01-12"},
    {4, L"Daniel Belov", L"d.belov", L"d.belov@company.example", L"Head of Sales", L"Active", L"2026-04-04 17:41", L"2025-02-01"},
    {5, L"Elena Kozlova", L"e.kozlova", L"e.kozlova@company.example", L"Sales Manager", L"Active", L"2026-03-29 15:04", L"2025-02-04"},
    {6, L"Victor Morozov", L"v.morozov", L"v.morozov@company.example", L"Product Manager", L"Active", L"2026-04-05 08:12", L"2025-02-15"}
};

constexpr SeedClient kClients[] = {
    {1, L"Northwind Retail", L"Elena Sokolova", L"elena.sokolova@northwind.example", L"+1 312 555 0142", L"Key Account", L"North America", 1, 428000.0, L"Anastasia Petrova"},
    {2, L"Mercury Logistics", L"Dan Cooper", L"dan.cooper@mercury.example", L"+1 646 555 0190", L"Mid-Market", L"North America", 1, 196500.0, L"Daniel Belov"},
    {3, L"Blue Peak Stores", L"Liam Turner", L"liam.turner@bluepeak.example", L"+44 20 7946 1184", L"Key Account", L"Western Europe", 1, 384900.0, L"Leonid Smirnov"},
    {4, L"Astra Wholesale", L"Irina Volkova", L"irina.volkova@astra.example", L"+49 30 555 6670", L"Regional Partner", L"Central Europe", 1, 142700.0, L"Anastasia Petrova"},
    {5, L"Cobalt Trade", L"Pavel Mironov", L"pavel.mironov@cobalt.example", L"+7 495 555 3841", L"Mid-Market", L"Eastern Europe", 1, 176300.0, L"Marina Ivanova"},
    {6, L"Urban Depot", L"Sophia Reed", L"sophia.reed@urbandepot.example", L"+1 415 555 0106", L"Key Account", L"North America", 1, 312800.0, L"Elena Kozlova"},
    {7, L"Transit Nova", L"Martin Weber", L"martin.weber@transitnova.example", L"+49 89 555 1022", L"Regional Partner", L"Central Europe", 1, 158600.0, L"Daniel Belov"},
    {8, L"Atlas Commerce", L"Olivia Carter", L"olivia.carter@atlas.example", L"+44 161 555 7820", L"Mid-Market", L"Western Europe", 1, 221400.0, L"Leonid Smirnov"}
};

constexpr SeedProduct kProducts[] = {
    {1, L"SKU-X12", L"Industrial Sensor X12", L"Electronics", 142, 60, 330.0, L"Active", L"Connective Labs", 31.0, L"2027-02-15", L"B-X12-2601"},
    {2, L"SKU-RTL", L"Retail Terminal Lite", L"Hardware", 21, 30, 355.0, L"Low Stock", L"PointServe Manufacturing", 28.0, L"2026-05-08", L"B-RTL-2602"},
    {3, L"SKU-SSM", L"Smart Shelf Module", L"Automation", 12, 24, 406.0, L"Low Stock", L"ShelfCore Systems", 33.0, L"2026-04-18", L"B-SSM-2603"},
    {4, L"SKU-TG4", L"Telemetry Gateway 4G", L"Connectivity", 18, 20, 596.0, L"Low Stock", L"Connective Labs", 26.0, L"2026-04-12", L"B-TG4-2604"},
    {5, L"SKU-WSP", L"Warehouse Scanner Pro", L"Logistics", 26, 28, 457.0, L"Low Stock", L"Logisource Ltd.", 27.0, L"2026-06-20", L"B-WSP-2605"},
    {6, L"SKU-CAM", L"Counter Analytics Module", L"Analytics", 74, 22, 468.0, L"Active", L"Retail Insight Works", 35.0, L"2026-09-30", L"B-CAM-2606"}
};

constexpr SeedEmployee kEmployees[] = {
    {1, L"Anastasia Petrova", L"Senior Sales Manager", L"North America", L"Exceeding Target"},
    {2, L"Leonid Smirnov", L"Key Account Manager", L"Western Europe", L"Exceeding Target"},
    {3, L"Marina Ivanova", L"Regional Manager", L"Eastern Europe", L"On Track"},
    {4, L"Daniel Belov", L"Sales Manager", L"Central Europe", L"On Track"},
    {5, L"Elena Kozlova", L"Sales Manager", L"North America", L"Watch"}
};

constexpr SeedTask kTasks[] = {
    {1, L"Call Blue Peak Stores about renewal", L"Leonid Smirnov", L"2026-04-19", L"Open", L"High", 3},
    {2, L"Prepare proposal for Astra Wholesale", L"Anastasia Petrova", L"2026-04-24", L"In Progress", L"Medium", 4},
    {3, L"Review low stock purchasing plan", L"Victor Morozov", L"2026-04-20", L"Open", L"High", 0}
};

constexpr SeedPipelineDeal kPipelineDeals[] = {
    {1, L"Northwind expansion", 1, 1, L"New Lead", 58000.0, L"2026-05-10", L"Open", L"2026-04-08"},
    {2, L"Mercury logistics hardware roll-out", 2, 4, L"In Work", 92000.0, L"2026-05-18", L"Open", L"2026-04-10"},
    {3, L"Blue Peak renewal", 3, 2, L"Proposal Sent", 125000.0, L"2026-05-22", L"Open", L"2026-04-12"},
    {4, L"Astra Q2 contract", 4, 1, L"Negotiation", 76000.0, L"2026-05-28", L"Open", L"2026-04-02"},
    {5, L"Cobalt add-on license", 5, 3, L"Closed Won", 41000.0, L"2026-04-15", L"Closed", L"2026-04-15"},
    {6, L"Transit Nova pilot", 7, 4, L"Lost", 26000.0, L"2026-04-12", L"Lost", L"2026-04-12"}
};

constexpr double kRevenueTargets[12] = {220000, 248000, 236000, 271000, 295000, 318000, 337000, 329000, 358000, 381000, 409000, 438000};
constexpr int kOrderTargets[12] = {68, 74, 71, 82, 88, 93, 97, 94, 102, 108, 113, 121};
constexpr int kStartYear = 2025;
constexpr int kStartMonth = 5;

std::wstring MonthName(int month) {
    return i18n::MonthShort(month);
}

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

std::wstring FormatNumber(long long value) {
    std::wstring digits = std::to_wstring(value);
    std::wstring result;
    int count = 0;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        if (count && count % 3 == 0) {
            result.push_back(L',');
        }
        result.push_back(*it);
        ++count;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::wstring FormatMoney(double value) {
    return L"$" + FormatNumber(static_cast<long long>(std::llround(value)));
}

std::wstring FormatCompactMoney(double value) {
    std::wostringstream stream;
    stream << L"$";
    if (value >= 1000000.0) {
        stream << std::fixed << std::setprecision(2) << (value / 1000000.0) << L"M";
    } else if (value >= 1000.0) {
        stream << std::fixed << std::setprecision(1) << (value / 1000.0) << L"K";
    } else {
        stream << std::fixed << std::setprecision(0) << value;
    }
    return stream.str();
}

std::wstring FormatPercent(double value, int decimals = 1) {
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(decimals) << value << L"%";
    return stream.str();
}

std::wstring FormatAverage(double value, int decimals = 1) {
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(decimals) << value;
    return stream.str();
}

std::wstring FormatDisplayDate(const std::wstring& isoDate) {
    if (isoDate.size() < 10) {
        return isoDate;
    }
    const int year = std::stoi(isoDate.substr(0, 4));
    const int month = std::stoi(isoDate.substr(5, 2));
    const int day = std::stoi(isoDate.substr(8, 2));
    std::wostringstream stream;
    stream << std::setfill(L'0') << std::setw(2) << day << L" " << MonthName(month) << L" " << year;
    return stream.str();
}

std::wstring CurrentDateIso() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    wchar_t buffer[32]{};
    wcsftime(buffer, 32, L"%Y-%m-%d", &local);
    return buffer;
}

std::wstring CurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    wchar_t buffer[64]{};
    wcsftime(buffer, 64, L"%Y-%m-%d %H:%M:%S", &local);
    return buffer;
}

std::wstring DemoPasswordHash(const std::wstring& password) {
    static constexpr wchar_t kSalt[] = L"SalesFlow::2026::auth";
    std::wstring payload = password;
    payload += L"|";
    payload += kSalt;

    const std::size_t hashValue = std::hash<std::wstring>{}(payload);
    std::wostringstream encoded;
    encoded << L"v1:" << std::hex << std::setw(sizeof(std::size_t) * 2) << std::setfill(L'0') << hashValue;
    return encoded.str();
}

std::wstring Trim(const std::wstring& value) {
    const auto first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) {
        return L"";
    }
    const auto last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

int ParseInt(const std::wstring& value, int fallback = 0) {
    try {
        return std::stoi(Trim(value));
    } catch (const std::exception&) {
        return fallback;
    }
}

double ParseDouble(const std::wstring& value, double fallback = 0.0) {
    try {
        return std::stod(Trim(value));
    } catch (const std::exception&) {
        return fallback;
    }
}

std::vector<std::wstring> SplitCsvLine(const std::wstring& line) {
    std::vector<std::wstring> cells;
    std::wstring current;
    bool quoted = false;
    for (size_t index = 0; index < line.size(); ++index) {
        const wchar_t ch = line[index];
        if (ch == L'"') {
            if (quoted && index + 1 < line.size() && line[index + 1] == L'"') {
                current.push_back(L'"');
                ++index;
            } else {
                quoted = !quoted;
            }
        } else if (ch == L',' && !quoted) {
            cells.push_back(Trim(current));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    cells.push_back(Trim(current));
    return cells;
}

std::wstring MakeIsoDate(int year, int month, int day) {
    std::wostringstream stream;
    stream << std::setfill(L'0') << std::setw(4) << year << L"-" << std::setw(2) << month << L"-" << std::setw(2) << day;
    return stream.str();
}

std::string QuoteSql(const std::wstring& value) {
    std::string utf8 = ToUtf8(value);
    std::string escaped;
    escaped.reserve(utf8.size() + 4);
    escaped.push_back('\'');
    for (char ch : utf8) {
        if (ch == '\'') {
            escaped.push_back('\'');
        }
        escaped.push_back(ch);
    }
    escaped.push_back('\'');
    return escaped;
}

std::wstring ExecutableDirectory() {
    wchar_t buffer[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path().wstring();
}

std::wstring TimestampForFile() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    wchar_t buffer[64]{};
    wcsftime(buffer, 64, L"%Y%m%d_%H%M%S", &local);
    return buffer;
}

std::string CsvEscape(const std::wstring& value) {
    std::string utf8 = ToUtf8(value);
    std::string escaped;
    escaped.reserve(utf8.size() + 4);
    escaped.push_back('"');
    for (char ch : utf8) {
        if (ch == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

bool WriteUtf8BomFile(const std::wstring& filePath, const std::string& body, std::wstring* error) {
    std::ofstream stream(std::filesystem::path(filePath), std::ios::binary);
    if (!stream.is_open()) {
        if (error) {
            *error = L"Unable to create export file.";
        }
        return false;
    }
    stream << "\xEF\xBB\xBF";
    stream.write(body.data(), static_cast<std::streamsize>(body.size()));
    return true;
}

void AppendDefaultSettings(std::ostringstream& sql, const std::wstring& timestamp) {
    sql << "INSERT INTO app_settings(setting_key, setting_value) VALUES"
        << "(" << QuoteSql(L"theme") << "," << QuoteSql(L"Corporate Light") << "),"
        << "(" << QuoteSql(L"language") << "," << QuoteSql(L"English") << "),"
        << "(" << QuoteSql(L"date_format") << "," << QuoteSql(L"dd MMM yyyy") << "),"
        << "(" << QuoteSql(L"app_name") << "," << QuoteSql(L"SalesFlow") << "),"
        << "(" << QuoteSql(L"auto_refresh_minutes") << "," << QuoteSql(L"5") << "),"
        << "(" << QuoteSql(L"startup_page") << "," << QuoteSql(L"Dashboard") << "),"
        << "(" << QuoteSql(L"last_seeded_at") << "," << QuoteSql(timestamp) << ");";
}

std::string BuildSeedScript() {
    std::ostringstream sql;
    sql << "BEGIN IMMEDIATE;";
    sql << "DELETE FROM audit_log;";
    sql << "DELETE FROM tasks;";
    sql << "DELETE FROM pipeline_deals;";
    sql << "DELETE FROM sales;";
    sql << "DELETE FROM orders;";
    sql << "DELETE FROM employees;";
    sql << "DELETE FROM products;";
    sql << "DELETE FROM clients;";
    sql << "DELETE FROM users;";
    sql << "DELETE FROM app_settings;";

    AppendDefaultSettings(sql, CurrentTimestamp());

    for (const auto& user : kUsers) {
        sql << "INSERT INTO users(id, full_name, username, email, role, password_hash, status, last_login, created_at) VALUES("
            << user.id << ","
            << QuoteSql(user.fullName) << ","
            << QuoteSql(user.username) << ","
            << QuoteSql(user.email) << ","
            << QuoteSql(user.role) << ","
            << QuoteSql(DemoPasswordHash(L"demo123")) << ","
            << QuoteSql(user.status) << ","
            << QuoteSql(user.lastLogin) << ","
            << QuoteSql(user.createdAt) << ");";
    }

    for (const auto& client : kClients) {
        sql << "INSERT INTO clients(id, company_name, contact_name, email, phone, segment, region, is_active, last_order_date, lifetime_value, account_manager) VALUES("
            << client.id << ","
            << QuoteSql(client.company) << ","
            << QuoteSql(client.contact) << ","
            << QuoteSql(client.email) << ","
            << QuoteSql(client.phone) << ","
            << QuoteSql(client.segment) << ","
            << QuoteSql(client.region) << ","
            << client.isActive << ","
            << QuoteSql(L"2026-04-05") << ","
            << client.lifetimeValue << ","
            << QuoteSql(client.accountManager) << ");";
    }

    for (const auto& product : kProducts) {
        sql << "INSERT INTO products(id, sku, name, category, stock, reorder_level, price, status, supplier, margin_percent, expiration_date, batch_code) VALUES("
            << product.id << ","
            << QuoteSql(product.sku) << ","
            << QuoteSql(product.name) << ","
            << QuoteSql(product.category) << ","
            << product.stock << ","
            << product.reorderLevel << ","
            << product.price << ","
            << QuoteSql(product.status) << ","
            << QuoteSql(product.supplier) << ","
            << product.marginPercent << ","
            << QuoteSql(product.expirationDate) << ","
            << QuoteSql(product.batchCode) << ");";
    }

    for (const auto& employee : kEmployees) {
        sql << "INSERT INTO employees(id, full_name, role, region, status) VALUES("
            << employee.id << ","
            << QuoteSql(employee.fullName) << ","
            << QuoteSql(employee.role) << ","
            << QuoteSql(employee.region) << ","
            << QuoteSql(employee.status) << ");";
    }

    int saleId = 1;
    int orderId = 1;
    const double revenueWeights[4] = {0.21, 0.24, 0.26, 0.29};
    const wchar_t* saleStatuses[4] = {L"Completed", L"Completed", L"Completed", L"Pending"};
    const wchar_t* saleChannels[3] = {L"Direct", L"Online", L"Partner"};

    for (int period = 0; period < 12; ++period) {
        const int year = kStartYear + ((kStartMonth - 1 + period) / 12);
        const int month = ((kStartMonth - 1 + period) % 12) + 1;

        for (int slot = 0; slot < 4; ++slot) {
            const int employeeId = (period + slot) % static_cast<int>(std::size(kEmployees)) + 1;
            const int clientId = (period * 2 + slot) % static_cast<int>(std::size(kClients)) + 1;
            const int productId = (period + slot * 2) % static_cast<int>(std::size(kProducts)) + 1;
            const double revenue = std::round(kRevenueTargets[period] * revenueWeights[slot]);
            const double margin = kProducts[productId - 1].marginPercent - (slot % 2 == 0 ? 0.5 : 1.3);
            const int day = (year == 2026 && month == 4) ? (slot + 1) : (3 + slot * 6);

            std::wostringstream saleCode;
            saleCode << L"SAL-" << (2100 + saleId);

            sql << "INSERT INTO sales(id, sale_code, employee_id, client_id, product_id, channel, revenue, margin_percent, sale_date, status) VALUES("
                << saleId << ","
                << QuoteSql(saleCode.str()) << ","
                << employeeId << ","
                << clientId << ","
                << productId << ","
                << QuoteSql(saleChannels[(period + slot) % 3]) << ","
                << revenue << ","
                << margin << ","
                << QuoteSql(MakeIsoDate(year, month, day)) << ","
                << QuoteSql(saleStatuses[(period == 11) ? slot : std::min(slot, 2)]) << ");";
            ++saleId;
        }

        for (int slot = 0; slot < kOrderTargets[period]; ++slot) {
            const int clientId = (period + slot) % static_cast<int>(std::size(kClients)) + 1;
            const int productId = (period * 3 + slot) % static_cast<int>(std::size(kProducts)) + 1;
            const int quantity = 8 + ((period * 5 + slot * 3) % 18);
            const double totalPrice = quantity * kProducts[productId - 1].price;
            int day = 1 + (slot % 27);
            if (year == 2026 && month == 4) {
                day = 1 + (slot % 5);
            }

            const wchar_t* status = L"Delivered";
            if (year == 2026 && month == 4) {
                static const wchar_t* currentStatuses[] = {L"Processing", L"Confirmed", L"Packed", L"Delivered"};
                status = currentStatuses[slot % 4];
            } else if (year == 2026 && month == 3) {
                static const wchar_t* marchStatuses[] = {L"Delivered", L"Delivered", L"Packed", L"Confirmed"};
                status = marchStatuses[slot % 4];
            }

            const wchar_t* priority = (kProducts[productId - 1].stock <= kProducts[productId - 1].reorderLevel || slot % 9 == 0)
                ? L"High"
                : ((slot % 3 == 0) ? L"Medium" : L"Normal");

            std::wostringstream orderCode;
            orderCode << L"ORD-" << (1000 + orderId);

            sql << "INSERT INTO orders(id, order_code, client_id, product_id, quantity, total_price, order_date, status, priority) VALUES("
                << orderId << ","
                << QuoteSql(orderCode.str()) << ","
                << clientId << ","
                << productId << ","
                << quantity << ","
                << totalPrice << ","
                << QuoteSql(MakeIsoDate(year, month, day)) << ","
                << QuoteSql(status) << ","
                << QuoteSql(priority) << ");";
            ++orderId;
        }
    }

    for (const auto& task : kTasks) {
        sql << "INSERT INTO tasks(id, title, assigned_to, due_date, status, priority, related_client_id, created_at) VALUES("
            << task.id << ","
            << QuoteSql(task.title) << ","
            << QuoteSql(task.assignedTo) << ","
            << QuoteSql(task.dueDate) << ","
            << QuoteSql(task.status) << ","
            << QuoteSql(task.priority) << ","
            << task.relatedClientId << ","
            << QuoteSql(CurrentTimestamp()) << ");";
    }

    for (const auto& deal : kPipelineDeals) {
        sql << "INSERT INTO pipeline_deals(id, lead_name, client_id, manager_id, stage, value, expected_close_date, status, updated_at) VALUES("
            << deal.id << ","
            << QuoteSql(deal.leadName) << ","
            << deal.clientId << ","
            << deal.managerId << ","
            << QuoteSql(deal.stage) << ","
            << deal.value << ","
            << QuoteSql(deal.expectedCloseDate) << ","
            << QuoteSql(deal.status) << ","
            << QuoteSql(deal.updatedAt) << ");";
    }

    sql << "INSERT INTO audit_log(id, entity_type, entity_id, action, user_name, details, created_at) VALUES("
        << 1 << ","
        << QuoteSql(L"system") << ","
        << 0 << ","
        << QuoteSql(L"Seed") << ","
        << QuoteSql(L"system") << ","
        << QuoteSql(L"Demo dataset was generated") << ","
        << QuoteSql(CurrentTimestamp()) << ");";

    sql << "COMMIT;";
    return sql.str();
}

bool GetStatementText(SqliteDatabase& db, const std::string& query, SqliteStatement* statement) {
    std::wstring ignored;
    return db.Prepare(query, statement, &ignored);
}

double QueryBoundDouble(SqliteDatabase& db, const std::string& query, const std::wstring& value, double fallback = 0.0) {
    SqliteStatement statement;
    std::wstring ignored;
    if (!db.Prepare(query, &statement, &ignored)) {
        return fallback;
    }
    statement.BindText(1, value);
    return statement.Step() == SQLITE_ROW ? statement.ColumnDouble(0) : fallback;
}

int QueryBoundInt(SqliteDatabase& db, const std::string& query, const std::wstring& value, int fallback = 0) {
    return static_cast<int>(std::llround(QueryBoundDouble(db, query, value, fallback)));
}

} // namespace

Repository& Repository::Instance() {
    static Repository instance;
    return instance;
}

Repository::Repository() : initialized_(false), hasCurrentAccount_(false) {}

bool Repository::Initialize() {
    if (initialized_) {
        return true;
    }

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(DatabasePath()).parent_path(), ec);
    if (ec) {
        SetLastError(L"Unable to create database folder.");
        return false;
    }

    std::wstring openError;
    if (!db_.Open(DatabasePath(), &openError)) {
        SetLastError(openError.empty() ? L"Unable to open SQLite database." : openError);
        return false;
    }

    if (!EnsureSchema()) {
        return false;
    }

    if (!EnsureDefaultSettings() || !EnsureDefaultRoles() || !EnsureDefaultAdminAccount() || !EnsureDemoAccountPasswords()) {
        return false;
    }

    initialized_ = true;
    return true;
}

bool Repository::ResetDemoData() {
    if (!EnsureInitialized()) {
        return false;
    }
    return SeedDemoData();
}

bool Repository::ClearBusinessData() {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::ClearBusinessData, L"Clear business data")) {
        return false;
    }

    std::ostringstream sql;
    sql << "BEGIN IMMEDIATE;";
    sql << "DELETE FROM tasks;";
    sql << "DELETE FROM pipeline_deals;";
    sql << "DELETE FROM sales;";
    sql << "DELETE FROM orders;";
    sql << "DELETE FROM employees;";
    sql << "DELETE FROM products;";
    sql << "DELETE FROM clients;";
    sql << "COMMIT;";

    if (!db_.Execute(sql.str(), &lastError_)) {
        return false;
    }
    LogAudit(L"database", 0, L"ClearBusinessData", L"Business tables were cleared from Settings");
    return UpdateSetting(L"last_seeded_at", CurrentTimestamp());
}

DashboardSnapshot Repository::LoadDashboardSnapshot() {
    DashboardSnapshot snapshot{};
    if (!EnsureInitialized()) {
        return snapshot;
    }
    if (hasCurrentAccount_ && !access::HasPermission(currentAccount_, access::Permission::OpenDashboard)) {
        SetLastError(L"Dashboard is not available for role: " + currentAccount_.role);
        return snapshot;
    }

    const bool ownSalesScope = hasCurrentAccount_
        && access::SalesScope(access::RoleForAccount(currentAccount_)) == access::DataScope::Own;
    const std::wstring dashboardOwner = ownSalesScope ? currentAccount_.fullName : L"";

    const double totalRevenue = ownSalesScope
        ? QueryBoundDouble(db_, "SELECT COALESCE(SUM(sales.revenue), 0) FROM sales JOIN employees ON employees.id = sales.employee_id WHERE lower(employees.full_name) = lower(?1);", dashboardOwner)
        : db_.QueryScalarInt("SELECT CAST(COALESCE(SUM(revenue), 0) AS INTEGER) FROM sales;", 0);
    const int totalOrders = ownSalesScope
        ? QueryBoundInt(db_, "SELECT COUNT(*) FROM sales JOIN employees ON employees.id = sales.employee_id WHERE lower(employees.full_name) = lower(?1);", dashboardOwner)
        : db_.QueryScalarInt("SELECT COUNT(*) FROM orders;", 0);
    const int productsSold = ownSalesScope
        ? QueryBoundInt(db_, "SELECT COALESCE(SUM(sales.quantity), 0) FROM sales JOIN employees ON employees.id = sales.employee_id WHERE lower(employees.full_name) = lower(?1);", dashboardOwner)
        : db_.QueryScalarInt("SELECT CAST(COALESCE(SUM(quantity), 0) AS INTEGER) FROM orders;", 0);
    const int activeClients = ownSalesScope
        ? QueryBoundInt(db_, "SELECT COUNT(*) FROM clients WHERE is_active = 1 AND lower(account_manager) = lower(?1);", dashboardOwner)
        : db_.QueryScalarInt("SELECT COUNT(*) FROM clients WHERE is_active = 1;", 0);
    const int shipments = ownSalesScope
        ? QueryBoundInt(db_, "SELECT COUNT(*) FROM sales JOIN employees ON employees.id = sales.employee_id WHERE sales.status = 'Pending' AND lower(employees.full_name) = lower(?1);", dashboardOwner)
        : db_.QueryScalarInt("SELECT COUNT(*) FROM orders WHERE status IN ('Processing', 'Confirmed', 'Packed');", 0);
    const int keyClients = ownSalesScope
        ? QueryBoundInt(db_, "SELECT COUNT(*) FROM clients WHERE segment = 'Key Account' AND is_active = 1 AND lower(account_manager) = lower(?1);", dashboardOwner)
        : db_.QueryScalarInt("SELECT COUNT(*) FROM clients WHERE segment = 'Key Account' AND is_active = 1;", 0);

    SqliteStatement basketStatement;
    double averageBasket = 0.0;
    const std::string basketQuery = ownSalesScope
        ? "SELECT COALESCE(AVG(sales.quantity), 0) FROM sales JOIN employees ON employees.id = sales.employee_id WHERE lower(employees.full_name) = lower(?1);"
        : "SELECT COALESCE(AVG(quantity), 0) FROM orders;";
    const bool basketReady = ownSalesScope ? db_.Prepare(basketQuery, &basketStatement, &lastError_) : GetStatementText(db_, basketQuery, &basketStatement);
    if (basketReady) {
        if (ownSalesScope) {
            basketStatement.BindText(1, dashboardOwner);
        }
        if (basketStatement.Step() == SQLITE_ROW) {
            averageBasket = basketStatement.ColumnDouble(0);
        }
    }

    snapshot.metrics.totalRevenue = FormatCompactMoney(totalRevenue);
    snapshot.metrics.totalOrders = FormatNumber(totalOrders);
    snapshot.metrics.productsSold = FormatNumber(productsSold);
    snapshot.metrics.activeClients = FormatNumber(activeClients);

    SqliteStatement revenueTrend;
    const std::string revenueTrendQuery = ownSalesScope
        ? "SELECT CAST(strftime('%m', sales.sale_date) AS INTEGER) AS month_no, "
          "       CAST(COALESCE(SUM(sales.revenue), 0) AS INTEGER) AS revenue_value "
          "FROM sales JOIN employees ON employees.id = sales.employee_id "
          "WHERE lower(employees.full_name) = lower(?1) "
          "GROUP BY strftime('%Y-%m', sales.sale_date) "
          "ORDER BY strftime('%Y-%m', sales.sale_date);"
        : "SELECT CAST(strftime('%m', sale_date) AS INTEGER) AS month_no, "
          "       CAST(COALESCE(SUM(revenue), 0) AS INTEGER) AS revenue_value "
          "FROM sales "
          "GROUP BY strftime('%Y-%m', sale_date) "
          "ORDER BY strftime('%Y-%m', sale_date);";
    if (ownSalesScope ? db_.Prepare(revenueTrendQuery, &revenueTrend, &lastError_) : GetStatementText(db_, revenueTrendQuery, &revenueTrend)) {
        if (ownSalesScope) {
            revenueTrend.BindText(1, dashboardOwner);
        }
        while (revenueTrend.Step() == SQLITE_ROW) {
            snapshot.revenueTrend.push_back({static_cast<double>(revenueTrend.ColumnInt(1)), MonthName(revenueTrend.ColumnInt(0))});
        }
    }

    SqliteStatement orderTrend;
    const std::string orderTrendQuery = ownSalesScope
        ? "SELECT CAST(strftime('%m', sales.sale_date) AS INTEGER) AS month_no, COUNT(*) AS order_count "
          "FROM sales JOIN employees ON employees.id = sales.employee_id "
          "WHERE lower(employees.full_name) = lower(?1) "
          "GROUP BY strftime('%Y-%m', sales.sale_date) "
          "ORDER BY strftime('%Y-%m', sales.sale_date);"
        : "SELECT CAST(strftime('%m', order_date) AS INTEGER) AS month_no, COUNT(*) AS order_count "
          "FROM orders "
          "GROUP BY strftime('%Y-%m', order_date) "
          "ORDER BY strftime('%Y-%m', order_date);";
    if (ownSalesScope ? db_.Prepare(orderTrendQuery, &orderTrend, &lastError_) : GetStatementText(db_, orderTrendQuery, &orderTrend)) {
        if (ownSalesScope) {
            orderTrend.BindText(1, dashboardOwner);
        }
        while (orderTrend.Step() == SQLITE_ROW) {
            snapshot.monthlyOrderVolume.push_back({static_cast<double>(orderTrend.ColumnInt(1)), MonthName(orderTrend.ColumnInt(0))});
        }
    }

    if (snapshot.revenueTrend.size() >= 6) {
        double previousQuarter = 0.0;
        double latestQuarter = 0.0;
        const size_t split = snapshot.revenueTrend.size() - 6;
        for (size_t index = split; index < split + 3; ++index) {
            previousQuarter += snapshot.revenueTrend[index].value;
        }
        for (size_t index = split + 3; index < split + 6; ++index) {
            latestQuarter += snapshot.revenueTrend[index].value;
        }
        const double delta = previousQuarter > 0.0 ? ((latestQuarter - previousQuarter) / previousQuarter) * 100.0 : 0.0;
        snapshot.metrics.revenueCaption = (delta >= 0.0 ? L"+" : L"") + FormatPercent(delta);
        snapshot.metrics.revenueCaption += UiText(L" versus previous quarter", L" к предыдущему кварталу");
    } else {
        snapshot.metrics.revenueCaption = UiText(L"SQL-backed trend is active", L"SQL-тренд активен");
    }

    snapshot.metrics.ordersCaption = FormatNumber(shipments) + UiText(L" orders awaiting shipment", L" заказов ожидают отгрузки");
    snapshot.metrics.productsCaption = UiText(L"Average basket size: ", L"Средний размер корзины: ") + FormatAverage(averageBasket) + UiText(L" units", L" ед.");
    snapshot.metrics.clientsCaption = FormatNumber(keyClients) + UiText(L" key accounts under active monitoring", L" ключевых клиентов под наблюдением");

    SqliteStatement topProducts;
    const std::string topProductsQuery = ownSalesScope
        ? "SELECT products.name, products.category, SUM(sales.quantity) AS units, "
          "       SUM(sales.revenue) AS revenue_total, AVG(products.margin_percent) AS margin_pct "
          "FROM sales "
          "JOIN products ON products.id = sales.product_id "
          "JOIN employees ON employees.id = sales.employee_id "
          "WHERE lower(employees.full_name) = lower(?1) "
          "GROUP BY sales.product_id "
          "ORDER BY units DESC "
          "LIMIT 5;"
        : "SELECT products.name, products.category, SUM(orders.quantity) AS units, "
          "       SUM(orders.total_price) AS revenue_total, AVG(products.margin_percent) AS margin_pct "
          "FROM orders "
          "JOIN products ON products.id = orders.product_id "
          "GROUP BY orders.product_id "
          "ORDER BY units DESC "
          "LIMIT 5;";
    if (ownSalesScope ? db_.Prepare(topProductsQuery, &topProducts, &lastError_) : GetStatementText(db_, topProductsQuery, &topProducts)) {
        if (ownSalesScope) {
            topProducts.BindText(1, dashboardOwner);
        }
        while (topProducts.Step() == SQLITE_ROW) {
            snapshot.topProducts.push_back({
                topProducts.ColumnText(0),
                topProducts.ColumnText(1),
                FormatNumber(topProducts.ColumnInt(2)),
                FormatMoney(topProducts.ColumnDouble(3)),
                FormatPercent(topProducts.ColumnDouble(4), 0)
            });
        }
    }

    SqliteStatement recentOrders;
    const std::string recentOrdersQuery = ownSalesScope
        ? "SELECT orders.order_code, clients.company_name, orders.order_date, orders.total_price, orders.status "
          "FROM orders "
          "JOIN clients ON clients.id = orders.client_id "
          "WHERE lower(clients.account_manager) = lower(?1) "
          "ORDER BY orders.order_date DESC, orders.id DESC "
          "LIMIT 5;"
        : "SELECT orders.order_code, clients.company_name, orders.order_date, orders.total_price, orders.status "
          "FROM orders "
          "JOIN clients ON clients.id = orders.client_id "
          "ORDER BY orders.order_date DESC, orders.id DESC "
          "LIMIT 5;";
    if (ownSalesScope ? db_.Prepare(recentOrdersQuery, &recentOrders, &lastError_) : GetStatementText(db_, recentOrdersQuery, &recentOrders)) {
        if (ownSalesScope) {
            recentOrders.BindText(1, dashboardOwner);
        }
        while (recentOrders.Step() == SQLITE_ROW) {
            snapshot.recentOrders.push_back({
                recentOrders.ColumnText(0),
                recentOrders.ColumnText(1),
                FormatDisplayDate(recentOrders.ColumnText(2)),
                FormatMoney(recentOrders.ColumnDouble(3)),
                recentOrders.ColumnText(4)
            });
        }
    }

    SqliteStatement lowStock;
    if (GetStatementText(db_,
            "SELECT alert_type, title, owner, due_text, status_text "
            "FROM ("
            "    SELECT 'Task' AS alert_type, title, assigned_to AS owner, due_date AS due_text, "
            "           CASE WHEN julianday(due_date) < julianday(date('now')) THEN 'Overdue' ELSE status END AS status_text, "
            "           0 AS priority_order "
            "    FROM tasks "
            "    WHERE status <> 'Done' "
            "    UNION ALL "
            "    SELECT 'Stock', name, supplier, CAST(stock AS TEXT) || '/' || CAST(reorder_level AS TEXT), "
            "           CASE WHEN stock <= reorder_level THEN 'High' WHEN stock <= reorder_level + 6 THEN 'Medium' ELSE 'Watch' END, "
            "           1 "
            "    FROM products "
            "    WHERE stock <= reorder_level + 8 "
            "    UNION ALL "
            "    SELECT 'Deal', pipeline_deals.lead_name, COALESCE(employees.full_name, 'Unassigned'), pipeline_deals.updated_at, pipeline_deals.stage, "
            "           2 "
            "    FROM pipeline_deals "
            "    LEFT JOIN employees ON employees.id = pipeline_deals.manager_id "
            "    WHERE pipeline_deals.status = 'Open' AND julianday(date('now')) - julianday(pipeline_deals.updated_at) >= 10 "
            ") "
            "ORDER BY priority_order ASC, due_text ASC "
            "LIMIT 5;",
            &lowStock)) {
        while (lowStock.Step() == SQLITE_ROW) {
            snapshot.lowStock.push_back({
                lowStock.ColumnText(0),
                lowStock.ColumnText(1),
                lowStock.ColumnText(2),
                lowStock.ColumnText(3),
                lowStock.ColumnText(4)
            });
        }
    }

    return snapshot;
}

std::vector<SalesRecord> Repository::LoadSales(const SalesFilter& filter) {
    std::vector<SalesRecord> rows;
    if (!EnsureInitialized()) {
        return rows;
    }
    if (hasCurrentAccount_ && !access::HasPermission(currentAccount_, access::Permission::OpenSales)) {
        SetLastError(L"Sales are not available for role: " + currentAccount_.role);
        return rows;
    }
    const bool ownScope = hasCurrentAccount_
        && access::SalesScope(access::RoleForAccount(currentAccount_)) == access::DataScope::Own;
    const std::wstring managerScope = ownScope ? currentAccount_.fullName : L"";

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT sales.id, sales.sale_code, employees.full_name, clients.company_name, products.name, "
            "       sales.channel, COALESCE(sales.quantity, 1), COALESCE(sales.unit_price, sales.revenue), "
            "       sales.revenue, COALESCE(sales.total_amount, sales.revenue), sales.margin_percent, "
            "       sales.sale_date, sales.status, COALESCE(sales.payment_method, 'Card'), COALESCE(sales.cancellation_reason, '') "
            "FROM sales "
            "JOIN employees ON employees.id = sales.employee_id "
            "JOIN clients ON clients.id = sales.client_id "
            "JOIN products ON products.id = sales.product_id "
            "WHERE (?1 = '' OR lower(sales.sale_code || ' ' || employees.full_name || ' ' || clients.company_name || ' ' || products.name) LIKE lower(?2)) "
            "  AND (?3 = '' OR ?3 = 'All Statuses' OR sales.status = ?3) "
            "  AND (?4 = '' OR ?4 = 'All Channels' OR sales.channel = ?4) "
            "  AND (?5 = '' OR sales.sale_date >= ?5) "
            "  AND (?6 = '' OR sales.sale_date <= ?6) "
            "  AND (?7 = '' OR lower(employees.full_name) = lower(?7)) "
            "ORDER BY sales.sale_date DESC, sales.id DESC;",
            &statement,
            &lastError_)) {
        return rows;
    }

    const std::wstring searchPattern = filter.search.empty() ? L"" : L"%" + filter.search + L"%";
    statement.BindText(1, filter.search);
    statement.BindText(2, searchPattern);
    statement.BindText(3, filter.status);
    statement.BindText(4, filter.channel);
    statement.BindText(5, filter.fromDateIso);
    statement.BindText(6, filter.toDateIso);
    statement.BindText(7, managerScope);

    while (statement.Step() == SQLITE_ROW) {
        SalesRecord record{};
        record.id = statement.ColumnInt(0);
        record.saleCode = statement.ColumnText(1);
        record.manager = statement.ColumnText(2);
        record.client = statement.ColumnText(3);
        record.product = statement.ColumnText(4);
        record.channel = statement.ColumnText(5);
        record.quantity = statement.ColumnInt(6);
        record.unitPrice = statement.ColumnDouble(7);
        record.revenue = statement.ColumnDouble(8);
        record.totalAmount = statement.ColumnDouble(9);
        record.marginPercent = statement.ColumnDouble(10);
        record.dateIso = statement.ColumnText(11);
        record.dateDisplay = FormatDisplayDate(record.dateIso);
        record.status = statement.ColumnText(12);
        record.paymentMethod = statement.ColumnText(13);
        record.cancellationReason = statement.ColumnText(14);
        rows.push_back(record);
    }

    return rows;
}

std::vector<OrderRecord> Repository::LoadOrders(const OrderFilter& filter) {
    std::vector<OrderRecord> rows;
    if (!EnsureInitialized()) {
        return rows;
    }
    if (hasCurrentAccount_
        && !access::HasPermission(currentAccount_, access::Permission::OpenOrders)
        && access::RoleForAccount(currentAccount_) != access::AppRole::Director) {
        SetLastError(L"Orders are not available for role: " + currentAccount_.role);
        return rows;
    }

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT orders.id, orders.order_code, clients.company_name, products.name, orders.quantity, "
            "       orders.total_price, orders.order_date, orders.status, orders.priority "
            "FROM orders "
            "JOIN clients ON clients.id = orders.client_id "
            "JOIN products ON products.id = orders.product_id "
            "WHERE (?1 = '' OR lower(orders.order_code || ' ' || clients.company_name || ' ' || products.name) LIKE lower(?2)) "
            "  AND (?3 = '' OR ?3 = 'All Statuses' OR orders.status = ?3) "
            "  AND (?4 = '' OR orders.order_date >= ?4) "
            "  AND (?5 = '' OR orders.order_date <= ?5) "
            "ORDER BY orders.order_date DESC, orders.id DESC;",
            &statement,
            &lastError_)) {
        return rows;
    }

    const std::wstring searchPattern = filter.search.empty() ? L"" : L"%" + filter.search + L"%";
    statement.BindText(1, filter.search);
    statement.BindText(2, searchPattern);
    statement.BindText(3, filter.status);
    statement.BindText(4, filter.fromDateIso);
    statement.BindText(5, filter.toDateIso);

    while (statement.Step() == SQLITE_ROW) {
        OrderRecord record{};
        record.id = statement.ColumnInt(0);
        record.orderCode = statement.ColumnText(1);
        record.client = statement.ColumnText(2);
        record.product = statement.ColumnText(3);
        record.quantity = statement.ColumnInt(4);
        record.totalPrice = statement.ColumnDouble(5);
        record.dateIso = statement.ColumnText(6);
        record.dateDisplay = FormatDisplayDate(record.dateIso);
        record.status = statement.ColumnText(7);
        record.priority = statement.ColumnText(8);
        rows.push_back(record);
    }

    return rows;
}

std::vector<ProductRecord> Repository::LoadProducts(const ProductFilter& filter) {
    std::vector<ProductRecord> rows;
    if (!EnsureInitialized()) {
        return rows;
    }
    if (hasCurrentAccount_ && !access::HasPermission(currentAccount_, access::Permission::OpenProducts)) {
        SetLastError(L"Products are not available for role: " + currentAccount_.role);
        return rows;
    }

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT id, sku, name, category, stock, reorder_level, price, COALESCE(cost, 0), COALESCE(discount_percent, 0), status, supplier, margin_percent, "
            "       expiration_date, COALESCE(batch_code, '') "
            "FROM products "
            "WHERE (?1 = '' OR lower(sku || ' ' || name || ' ' || category || ' ' || supplier) LIKE lower(?2)) "
            "  AND (?3 = '' OR ?3 = 'All Categories' OR category = ?3) "
            "  AND (?4 = '' OR ?4 = 'All Statuses' OR status = ?4) "
            "ORDER BY CASE status WHEN 'Low Stock' THEN 0 WHEN 'Active' THEN 1 ELSE 2 END, name ASC;",
            &statement,
            &lastError_)) {
        return rows;
    }

    const std::wstring searchPattern = filter.search.empty() ? L"" : L"%" + filter.search + L"%";
    statement.BindText(1, filter.search);
    statement.BindText(2, searchPattern);
    statement.BindText(3, filter.category);
    statement.BindText(4, filter.status);

    while (statement.Step() == SQLITE_ROW) {
        ProductRecord record{};
        record.id = statement.ColumnInt(0);
        record.sku = statement.ColumnText(1);
        record.name = statement.ColumnText(2);
        record.category = statement.ColumnText(3);
        record.stock = statement.ColumnInt(4);
        record.reorderLevel = statement.ColumnInt(5);
        record.price = statement.ColumnDouble(6);
        record.cost = statement.ColumnDouble(7);
        record.discountPercent = statement.ColumnDouble(8);
        record.status = statement.ColumnText(9);
        record.supplier = statement.ColumnText(10);
        record.marginPercent = statement.ColumnDouble(11);
        record.profitPerUnit = record.price - record.cost - (record.price * record.discountPercent / 100.0);
        record.expirationDateIso = statement.ColumnText(12);
        record.expirationDateDisplay = FormatDisplayDate(record.expirationDateIso);
        record.batchCode = statement.ColumnText(13);
        rows.push_back(record);
    }

    return rows;
}

std::vector<ClientRecord> Repository::LoadClients(const ClientFilter& filter) {
    std::vector<ClientRecord> rows;
    if (!EnsureInitialized()) {
        return rows;
    }
    if (hasCurrentAccount_ && !access::HasPermission(currentAccount_, access::Permission::OpenClients)) {
        SetLastError(L"Clients are not available for role: " + currentAccount_.role);
        return rows;
    }
    const bool ownScope = hasCurrentAccount_
        && access::ClientScope(access::RoleForAccount(currentAccount_)) == access::DataScope::Own;
    const std::wstring managerScope = ownScope ? currentAccount_.fullName : L"";

    std::string orderBy = "company_name ASC";
    if (filter.sort == L"Sort: Segment") {
        orderBy = "segment ASC, company_name ASC";
    } else if (filter.sort == L"Sort: Region") {
        orderBy = "region ASC, company_name ASC";
    }

    SqliteStatement statement;
    const std::string query =
        "SELECT id, company_name, contact_name, email, phone, segment, region, "
        "       COALESCE(last_order_date, ''), lifetime_value, account_manager "
        "FROM clients "
        "WHERE (?1 = '' OR lower(company_name || ' ' || contact_name || ' ' || email || ' ' || phone) LIKE lower(?2)) "
        "  AND (?3 = '' OR ?3 = 'All Segments' OR segment = ?3) "
        "  AND (?4 = '' OR lower(account_manager) = lower(?4)) "
        "ORDER BY " + orderBy + ";";
    if (!db_.Prepare(query, &statement, &lastError_)) {
        return rows;
    }

    const std::wstring searchPattern = filter.search.empty() ? L"" : L"%" + filter.search + L"%";
    statement.BindText(1, filter.search);
    statement.BindText(2, searchPattern);
    statement.BindText(3, filter.segment);
    statement.BindText(4, managerScope);

    while (statement.Step() == SQLITE_ROW) {
        ClientRecord record{};
        record.id = statement.ColumnInt(0);
        record.company = statement.ColumnText(1);
        record.contact = statement.ColumnText(2);
        record.email = statement.ColumnText(3);
        record.phone = statement.ColumnText(4);
        record.segment = statement.ColumnText(5);
        record.region = statement.ColumnText(6);
        record.lastOrderDateIso = statement.ColumnText(7);
        record.lastOrderDateDisplay = record.lastOrderDateIso.empty() ? L"" : FormatDisplayDate(record.lastOrderDateIso);
        record.lifetimeValue = statement.ColumnDouble(8);
        record.accountManager = statement.ColumnText(9);
        rows.push_back(record);
    }

    return rows;
}

std::vector<ClientOrderRecord> Repository::LoadClientOrderHistory(int clientId) {
    std::vector<ClientOrderRecord> rows;
    if (!EnsureInitialized() || clientId <= 0) {
        return rows;
    }
    if (hasCurrentAccount_ && access::ClientScope(access::RoleForAccount(currentAccount_)) == access::DataScope::Own) {
        SqliteStatement ownership;
        if (!db_.Prepare("SELECT account_manager FROM clients WHERE id = ?1;", &ownership, &lastError_)) {
            return rows;
        }
        ownership.BindInt(1, clientId);
        if (ownership.Step() != SQLITE_ROW || ownership.ColumnText(0) != currentAccount_.fullName) {
            SetLastError(L"Client history is outside the current user's data scope.");
            return rows;
        }
    }

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT orders.order_code, products.name, orders.order_date, orders.total_price "
            "FROM orders "
            "JOIN products ON products.id = orders.product_id "
            "WHERE orders.client_id = ?1 "
            "ORDER BY orders.order_date DESC, orders.id DESC "
            "LIMIT 12;",
            &statement,
            &lastError_)) {
        return rows;
    }
    statement.BindInt(1, clientId);

    while (statement.Step() == SQLITE_ROW) {
        ClientOrderRecord record{};
        record.orderCode = statement.ColumnText(0);
        record.product = statement.ColumnText(1);
        record.dateIso = statement.ColumnText(2);
        record.dateDisplay = FormatDisplayDate(record.dateIso);
        record.amount = statement.ColumnDouble(3);
        rows.push_back(record);
    }
    return rows;
}

std::vector<EmployeePerformanceRecord> Repository::LoadEmployeePerformance() {
    std::vector<EmployeePerformanceRecord> rows;
    if (!EnsureInitialized()) {
        return rows;
    }
    if (hasCurrentAccount_ && !access::HasPermission(currentAccount_, access::Permission::ViewEmployeePerformance)) {
        SetLastError(L"Employee performance is not available for role: " + currentAccount_.role);
        return rows;
    }

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT employees.id, employees.full_name, employees.role, employees.status, "
            "       COUNT(sales.id) AS total_sales, "
            "       SUM(CASE WHEN sales.status = 'Completed' THEN 1 ELSE 0 END) AS completed_sales, "
            "       COALESCE(SUM(sales.revenue), 0), "
            "       COALESCE(AVG(sales.margin_percent), 0) "
            "FROM employees "
            "LEFT JOIN sales ON sales.employee_id = employees.id "
            "GROUP BY employees.id, employees.full_name, employees.role, employees.status "
            "ORDER BY COALESCE(SUM(sales.revenue), 0) DESC, employees.full_name ASC;",
            &statement,
            &lastError_)) {
        return rows;
    }

    while (statement.Step() == SQLITE_ROW) {
        EmployeePerformanceRecord record{};
        record.id = statement.ColumnInt(0);
        record.fullName = statement.ColumnText(1);
        record.role = statement.ColumnText(2);
        record.status = statement.ColumnText(3);
        const int totalSales = statement.ColumnInt(4);
        record.completedSales = statement.ColumnInt(5);
        record.revenue = statement.ColumnDouble(6);
        record.avgMarginPercent = statement.ColumnDouble(7);
        record.conversionPercent = totalSales > 0 ? (record.completedSales * 100.0 / static_cast<double>(totalSales)) : 0.0;
        rows.push_back(record);
    }
    return rows;
}

std::vector<AccountRecord> Repository::LoadAccounts() {
    std::vector<AccountRecord> rows;
    if (!EnsureInitialized()) {
        return rows;
    }
    if (hasCurrentAccount_ && !access::HasPermission(currentAccount_, access::Permission::ManageUsers)) {
        SetLastError(L"User accounts are not available for role: " + currentAccount_.role);
        return rows;
    }

    SqliteStatement statement;
    if (!GetStatementText(db_,
            "SELECT id, full_name, username, email, role, status, COALESCE(last_login, 'Never'), created_at "
            "FROM users "
            "ORDER BY CASE status WHEN 'Active' THEN 0 ELSE 1 END, full_name ASC;",
            &statement)) {
        SetLastError(db_.LastError());
        return rows;
    }

    while (statement.Step() == SQLITE_ROW) {
        AccountRecord record{};
        record.id = statement.ColumnInt(0);
        record.fullName = statement.ColumnText(1);
        record.username = statement.ColumnText(2);
        record.email = statement.ColumnText(3);
        record.role = statement.ColumnText(4);
        record.status = statement.ColumnText(5);
        record.lastLogin = statement.ColumnText(6);
        record.createdAt = statement.ColumnText(7);
        rows.push_back(record);
    }

    return rows;
}

std::vector<ProductExpiryRecord> Repository::LoadProductExpiryReport() {
    std::vector<ProductExpiryRecord> rows;
    if (!EnsureInitialized()) {
        return rows;
    }
    if (hasCurrentAccount_ && !access::HasPermission(currentAccount_, access::Permission::OpenReports)) {
        SetLastError(L"Product expiry reports are not available for role: " + currentAccount_.role);
        return rows;
    }

    SqliteStatement statement;
    if (!GetStatementText(db_,
            "SELECT id, sku, name, category, expiration_date, "
            "       CAST(julianday(expiration_date) - julianday(date('now')) AS INTEGER) AS days_remaining, "
            "       CASE "
            "           WHEN julianday(expiration_date) < julianday(date('now')) THEN 'Expired' "
            "           WHEN julianday(expiration_date) - julianday(date('now')) <= 14 THEN 'Critical' "
            "           WHEN julianday(expiration_date) - julianday(date('now')) <= 45 THEN 'Warning' "
            "           ELSE 'OK' "
            "       END AS expiry_status, "
            "       supplier, COALESCE(batch_code, '') "
            "FROM products "
            "ORDER BY expiration_date ASC, name ASC;",
            &statement)) {
        SetLastError(db_.LastError());
        return rows;
    }

    while (statement.Step() == SQLITE_ROW) {
        ProductExpiryRecord record{};
        record.id = statement.ColumnInt(0);
        record.sku = statement.ColumnText(1);
        record.product = statement.ColumnText(2);
        record.category = statement.ColumnText(3);
        record.expirationDateIso = statement.ColumnText(4);
        record.expirationDateDisplay = FormatDisplayDate(record.expirationDateIso);
        record.daysRemaining = statement.ColumnInt(5);
        record.status = statement.ColumnText(6);
        record.supplier = statement.ColumnText(7);
        record.batchCode = statement.ColumnText(8);
        rows.push_back(record);
    }

    return rows;
}

std::vector<PipelineStageRecord> Repository::LoadPipelineSummary() {
    std::vector<PipelineStageRecord> rows;
    if (!EnsureInitialized()) {
        return rows;
    }
    if (hasCurrentAccount_ && !access::HasPermission(currentAccount_, access::Permission::ViewCompanyAnalytics)) {
        SetLastError(L"Sales funnel is not available for role: " + currentAccount_.role);
        return rows;
    }

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT stage, COUNT(*), COALESCE(SUM(value), 0) "
            "FROM pipeline_deals "
            "GROUP BY stage "
            "ORDER BY CASE stage "
            "    WHEN 'New Lead' THEN 1 "
            "    WHEN 'In Work' THEN 2 "
            "    WHEN 'Proposal Sent' THEN 3 "
            "    WHEN 'Negotiation' THEN 4 "
            "    WHEN 'Closed Won' THEN 5 "
            "    WHEN 'Lost' THEN 6 "
            "    ELSE 7 END;",
            &statement,
            &lastError_)) {
        return rows;
    }

    while (statement.Step() == SQLITE_ROW) {
        PipelineStageRecord record{};
        record.stage = statement.ColumnText(0);
        record.count = statement.ColumnInt(1);
        record.value = statement.ColumnDouble(2);
        rows.push_back(record);
    }
    return rows;
}

std::vector<TaskRecord> Repository::LoadOpenTasks() {
    std::vector<TaskRecord> rows;
    if (!EnsureInitialized()) {
        return rows;
    }
    if (hasCurrentAccount_) {
        const access::DataScope scope = access::TaskScope(access::RoleForAccount(currentAccount_));
        if (scope == access::DataScope::None) {
            SetLastError(L"Tasks are not available for role: " + currentAccount_.role);
            return rows;
        }
    }
    const std::wstring assigneeScope = hasCurrentAccount_
        && access::TaskScope(access::RoleForAccount(currentAccount_)) == access::DataScope::Own
            ? currentAccount_.fullName
            : L"";

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT id, title, assigned_to, due_date, status, priority "
            "FROM tasks "
            "WHERE status <> 'Done' "
            "  AND (?1 = '' OR lower(assigned_to) = lower(?1)) "
            "ORDER BY due_date ASC, CASE priority WHEN 'High' THEN 0 WHEN 'Medium' THEN 1 ELSE 2 END, id ASC;",
            &statement,
            &lastError_)) {
        return rows;
    }
    statement.BindText(1, assigneeScope);

    while (statement.Step() == SQLITE_ROW) {
        TaskRecord record{};
        record.id = statement.ColumnInt(0);
        record.title = statement.ColumnText(1);
        record.assignedTo = statement.ColumnText(2);
        record.dueDateIso = statement.ColumnText(3);
        record.dueDateDisplay = FormatDisplayDate(record.dueDateIso);
        record.status = statement.ColumnText(4);
        record.priority = statement.ColumnText(5);
        rows.push_back(record);
    }
    return rows;
}

std::vector<AuditRecord> Repository::LoadAuditLog(int limit) {
    std::vector<AuditRecord> rows;
    if (!EnsureInitialized()) {
        return rows;
    }
    if (hasCurrentAccount_ && !access::HasPermission(currentAccount_, access::Permission::ViewAuditLog)) {
        SetLastError(L"Audit log is not available for role: " + currentAccount_.role);
        return rows;
    }

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT id, entity_type, entity_id, action, user_name, details, created_at "
            "FROM audit_log "
            "ORDER BY id DESC "
            "LIMIT ?1;",
            &statement,
            &lastError_)) {
        return rows;
    }
    statement.BindInt(1, std::max(1, limit));

    while (statement.Step() == SQLITE_ROW) {
        AuditRecord record{};
        record.id = statement.ColumnInt(0);
        record.entityType = statement.ColumnText(1);
        record.entityId = statement.ColumnInt(2);
        record.action = statement.ColumnText(3);
        record.userName = statement.ColumnText(4);
        record.details = statement.ColumnText(5);
        record.createdAt = statement.ColumnText(6);
        rows.push_back(record);
    }
    return rows;
}

DatabaseOverview Repository::LoadDatabaseOverview() {
    DatabaseOverview overview{};
    if (!EnsureInitialized()) {
        return overview;
    }

    overview.connected = true;
    overview.engineVersion = db_.QueryScalarText("SELECT sqlite_version();", L"SQLite");
    overview.databasePath = DatabasePath();
    overview.lastSeededAt = ReadSetting(L"last_seeded_at", L"");
    overview.usersCount = db_.QueryScalarInt("SELECT COUNT(*) FROM users;", 0);
    overview.clientsCount = db_.QueryScalarInt("SELECT COUNT(*) FROM clients;", 0);
    overview.productsCount = db_.QueryScalarInt("SELECT COUNT(*) FROM products;", 0);
    overview.ordersCount = db_.QueryScalarInt("SELECT COUNT(*) FROM orders;", 0);
    overview.salesCount = db_.QueryScalarInt("SELECT COUNT(*) FROM sales;", 0);
    return overview;
}

AppSettings Repository::LoadSettings() {
    AppSettings settings{};
    if (!EnsureInitialized()) {
        return settings;
    }

    settings.theme = ReadSetting(L"theme", L"Corporate Light");
    settings.language = ReadSetting(L"language", L"English");
    settings.dateFormat = ReadSetting(L"date_format", L"dd MMM yyyy");
    settings.appName = ReadSetting(L"app_name", L"SalesFlow");
    if (settings.appName == L"Sales Monitoring Information System") {
        settings.appName = L"SalesFlow";
        UpdateSetting(L"app_name", settings.appName);
    }
    settings.autoRefreshMinutes = ReadSetting(L"auto_refresh_minutes", L"5");
    settings.startupPage = ReadSetting(L"startup_page", L"Dashboard");
    return settings;
}

bool Repository::SaveSettings(const AppSettings& settings) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::ManageSystemSettings, L"Save system settings")) {
        return false;
    }

    return UpdateSetting(L"theme", settings.theme)
        && UpdateSetting(L"language", settings.language)
        && UpdateSetting(L"date_format", settings.dateFormat)
        && UpdateSetting(L"app_name", settings.appName)
        && UpdateSetting(L"auto_refresh_minutes", settings.autoRefreshMinutes)
        && UpdateSetting(L"startup_page", settings.startupPage);
}

std::wstring Repository::ExportSalesCsv(const SalesFilter& filter) {
    if (!EnsureInitialized()) {
        return L"";
    }
    if (!RequirePermission(access::Permission::ExportSales, L"Export sales CSV")) {
        return L"";
    }
    const auto rows = LoadSales(filter);

    const std::wstring exportsDir = EnsureExportsDirectory();
    if (exportsDir.empty()) {
        return L"";
    }

    const std::wstring filePath = exportsDir + L"\\sales_export_" + TimestampForFile() + L".csv";
    std::ofstream stream(std::filesystem::path(filePath), std::ios::binary);
    if (!stream.is_open()) {
        SetLastError(L"Unable to create CSV export file.");
        return L"";
    }

    stream << "\xEF\xBB\xBF";
    stream << "Sale ID,Manager,Client,Product,Channel,Revenue,Margin,Date,Status\r\n";
    for (const auto& row : rows) {
        const auto toCsv = [](const std::wstring& value) {
            std::string utf8 = ToUtf8(value);
            std::string escaped;
            escaped.reserve(utf8.size() + 4);
            escaped.push_back('"');
            for (char ch : utf8) {
                if (ch == '"') {
                    escaped.push_back('"');
                }
                escaped.push_back(ch);
            }
            escaped.push_back('"');
            return escaped;
        };

        stream << toCsv(row.saleCode) << ','
               << toCsv(row.manager) << ','
               << toCsv(row.client) << ','
               << toCsv(row.product) << ','
               << toCsv(row.channel) << ','
               << toCsv(FormatMoney(row.revenue)) << ','
               << toCsv(FormatPercent(row.marginPercent, 1)) << ','
               << toCsv(row.dateDisplay) << ','
               << toCsv(row.status) << "\r\n";
    }
    return filePath;
}

std::wstring Repository::ExportProductExpiryCsv() {
    if (!EnsureInitialized()) {
        return L"";
    }
    if (!HasAnyPermission(access::Permission::ExportReports, access::Permission::ImportExportCatalog)) {
        SetLastError(L"Export product expiry CSV is not allowed for role: " + currentAccount_.role);
        return L"";
    }
    const auto rows = LoadProductExpiryReport();

    const std::wstring exportsDir = EnsureExportsDirectory();
    if (exportsDir.empty()) {
        return L"";
    }

    const std::wstring filePath = exportsDir + L"\\product_expiry_report_" + TimestampForFile() + L".csv";
    std::ofstream stream(std::filesystem::path(filePath), std::ios::binary);
    if (!stream.is_open()) {
        SetLastError(L"Unable to create product expiration CSV export file.");
        return L"";
    }

    const auto toCsv = [](const std::wstring& value) {
        std::string utf8 = ToUtf8(value);
        std::string escaped;
        escaped.reserve(utf8.size() + 4);
        escaped.push_back('"');
        for (char ch : utf8) {
            if (ch == '"') {
                escaped.push_back('"');
            }
            escaped.push_back(ch);
        }
        escaped.push_back('"');
        return escaped;
    };

    stream << "\xEF\xBB\xBF";
    stream << "SKU,Product,Category,Expiration Date,Days Remaining,Risk Status,Supplier,Batch\r\n";
    for (const auto& row : rows) {
        stream << toCsv(row.sku) << ','
               << toCsv(row.product) << ','
               << toCsv(row.category) << ','
               << toCsv(row.expirationDateDisplay) << ','
               << row.daysRemaining << ','
               << toCsv(row.status) << ','
               << toCsv(row.supplier) << ','
               << toCsv(row.batchCode) << "\r\n";
    }
    return filePath;
}

std::wstring Repository::ExportDailySalesSummaryReport() {
    if (!EnsureInitialized()) {
        return L"";
    }
    if (!RequirePermission(access::Permission::ExportReports, L"Export daily sales summary")) {
        return L"";
    }

    const std::wstring reportDate = db_.QueryScalarText("SELECT COALESCE(MAX(sale_date), '') FROM sales;", L"");
    SalesFilter filter{};
    filter.fromDateIso = reportDate;
    filter.toDateIso = reportDate;
    const auto rows = LoadSales(filter);

    const std::wstring exportsDir = EnsureExportsDirectory();
    if (exportsDir.empty()) {
        return L"";
    }

    double totalRevenue = 0.0;
    int completed = 0;
    int pending = 0;
    int direct = 0;
    int online = 0;
    int partner = 0;
    std::vector<std::pair<std::wstring, double>> managerTotals;
    for (const auto& row : rows) {
        totalRevenue += row.revenue;
        completed += row.status == L"Completed" ? 1 : 0;
        pending += row.status == L"Pending" ? 1 : 0;
        direct += row.channel == L"Direct" ? 1 : 0;
        online += row.channel == L"Online" ? 1 : 0;
        partner += row.channel == L"Partner" ? 1 : 0;

        auto it = std::find_if(managerTotals.begin(), managerTotals.end(),
            [&row](const auto& item) { return item.first == row.manager; });
        if (it == managerTotals.end()) {
            managerTotals.push_back({row.manager, row.revenue});
        } else {
            it->second += row.revenue;
        }
    }
    std::sort(managerTotals.begin(), managerTotals.end(),
        [](const auto& left, const auto& right) { return left.second > right.second; });

    std::wostringstream report;
    report << L"Daily Sales Summary\r\n";
    report << L"Reporting date: " << (reportDate.empty() ? L"N/A" : FormatDisplayDate(reportDate)) << L"\r\n";
    report << L"Rows exported: " << rows.size() << L"\r\n";
    report << L"Total revenue: " << FormatMoney(totalRevenue) << L"\r\n";
    report << L"Completed deals: " << completed << L"\r\n";
    report << L"Pending deals: " << pending << L"\r\n";
    report << L"Channel mix: Direct=" << direct << L", Online=" << online << L", Partner=" << partner << L"\r\n\r\n";
    report << L"Top managers\r\n";
    const size_t managerCount = std::min<size_t>(managerTotals.size(), 5);
    for (size_t index = 0; index < managerCount; ++index) {
        report << L"  " << (index + 1) << L". " << managerTotals[index].first
               << L"  " << FormatMoney(managerTotals[index].second) << L"\r\n";
    }
    report << L"\r\nTransactions\r\n";
    for (const auto& row : rows) {
        report << L"  " << row.saleCode << L" | " << row.manager << L" | " << row.client
               << L" | " << row.product << L" | " << row.channel
               << L" | " << FormatMoney(row.revenue) << L" | " << row.status << L"\r\n";
    }

    const std::wstring filePath = exportsDir + L"\\daily_sales_summary_" + TimestampForFile() + L".txt";
    std::wstring error;
    if (!WriteUtf8BomFile(filePath, ToUtf8(report.str()), &error)) {
        SetLastError(error);
        return L"";
    }
    return filePath;
}

std::wstring Repository::ExportFulfillmentReport() {
    if (!EnsureInitialized()) {
        return L"";
    }
    if (!RequirePermission(access::Permission::ExportReports, L"Export fulfillment report")) {
        return L"";
    }

    const auto rows = LoadOrders(OrderFilter{});
    const std::wstring exportsDir = EnsureExportsDirectory();
    if (exportsDir.empty()) {
        return L"";
    }

    int processing = 0;
    int confirmed = 0;
    int packed = 0;
    int delivered = 0;
    int highPriority = 0;
    double openValue = 0.0;
    for (const auto& row : rows) {
        processing += row.status == L"Processing" ? 1 : 0;
        confirmed += row.status == L"Confirmed" ? 1 : 0;
        packed += row.status == L"Packed" ? 1 : 0;
        delivered += row.status == L"Delivered" ? 1 : 0;
        highPriority += row.priority == L"High" ? 1 : 0;
        if (row.status != L"Delivered") {
            openValue += row.totalPrice;
        }
    }

    std::wostringstream report;
    report << L"Order Fulfillment Report\r\n";
    report << L"Generated at: " << CurrentTimestamp() << L"\r\n";
    report << L"Visible orders: " << rows.size() << L"\r\n";
    report << L"Processing: " << processing << L"\r\n";
    report << L"Confirmed: " << confirmed << L"\r\n";
    report << L"Packed: " << packed << L"\r\n";
    report << L"Delivered: " << delivered << L"\r\n";
    report << L"High priority: " << highPriority << L"\r\n";
    report << L"Open pipeline value: " << FormatMoney(openValue) << L"\r\n\r\n";
    report << L"Open operational queue\r\n";
    for (const auto& row : rows) {
        if (row.status == L"Delivered") {
            continue;
        }
        report << L"  " << row.orderCode << L" | " << row.client << L" | " << row.product
               << L" | qty " << row.quantity << L" | " << row.status
               << L" | priority " << row.priority << L" | " << FormatMoney(row.totalPrice) << L"\r\n";
    }

    const std::wstring filePath = exportsDir + L"\\fulfillment_report_" + TimestampForFile() + L".txt";
    std::wstring error;
    if (!WriteUtf8BomFile(filePath, ToUtf8(report.str()), &error)) {
        SetLastError(error);
        return L"";
    }
    return filePath;
}

std::wstring Repository::ExportEmployeePerformanceReport() {
    if (!EnsureInitialized()) {
        return L"";
    }
    if (!RequirePermission(access::Permission::ViewEmployeePerformance, L"Export employee performance report")) {
        return L"";
    }

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT employees.full_name, COUNT(sales.id), COALESCE(SUM(sales.revenue), 0), "
            "       COALESCE(AVG(sales.margin_percent), 0), "
            "       SUM(CASE WHEN sales.status = 'Completed' THEN 1 ELSE 0 END) "
            "FROM employees "
            "LEFT JOIN sales ON sales.employee_id = employees.id "
            "GROUP BY employees.id, employees.full_name "
            "ORDER BY COALESCE(SUM(sales.revenue), 0) DESC, employees.full_name ASC;",
            &statement,
            &lastError_)) {
        return L"";
    }

    const std::wstring exportsDir = EnsureExportsDirectory();
    if (exportsDir.empty()) {
        return L"";
    }

    std::wostringstream report;
    report << L"Employee Performance Report\r\n";
    report << L"Generated at: " << CurrentTimestamp() << L"\r\n\r\n";
    while (statement.Step() == SQLITE_ROW) {
        const std::wstring employee = statement.ColumnText(0);
        const int deals = statement.ColumnInt(1);
        const double revenue = statement.ColumnDouble(2);
        const double margin = statement.ColumnDouble(3);
        const int completed = statement.ColumnInt(4);
        report << employee << L"\r\n";
        report << L"  Deals: " << deals << L"\r\n";
        report << L"  Revenue: " << FormatMoney(revenue) << L"\r\n";
        report << L"  Average margin: " << FormatPercent(margin, 1) << L"\r\n";
        report << L"  Completed: " << completed << L"\r\n\r\n";
    }

    const std::wstring filePath = exportsDir + L"\\employee_performance_" + TimestampForFile() + L".txt";
    std::wstring error;
    if (!WriteUtf8BomFile(filePath, ToUtf8(report.str()), &error)) {
        SetLastError(error);
        return L"";
    }
    return filePath;
}

std::wstring Repository::ExportClientReport() {
    if (!EnsureInitialized()) {
        return L"";
    }
    if (!RequirePermission(access::Permission::ExportReports, L"Export client report")) {
        return L"";
    }

    const std::wstring exportsDir = EnsureExportsDirectory();
    if (exportsDir.empty()) {
        return L"";
    }

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT clients.company_name, clients.contact_name, clients.segment, clients.region, clients.account_manager, "
            "       COUNT(sales.id), COALESCE(SUM(sales.revenue), 0), COALESCE(MAX(sales.sale_date), clients.last_order_date) "
            "FROM clients "
            "LEFT JOIN sales ON sales.client_id = clients.id "
            "GROUP BY clients.id "
            "ORDER BY COALESCE(SUM(sales.revenue), clients.lifetime_value) DESC, clients.company_name ASC;",
            &statement,
            &lastError_)) {
        return L"";
    }

    std::wostringstream report;
    report << L"Client Report\r\n";
    report << L"Generated at: " << CurrentTimestamp() << L"\r\n\r\n";
    while (statement.Step() == SQLITE_ROW) {
        const std::wstring lastDate = statement.ColumnText(7);
        report << statement.ColumnText(0) << L"\r\n";
        report << L"  Contact: " << statement.ColumnText(1) << L"\r\n";
        report << L"  Segment / region: " << statement.ColumnText(2) << L" / " << statement.ColumnText(3) << L"\r\n";
        report << L"  Account manager: " << statement.ColumnText(4) << L"\r\n";
        report << L"  Sales count: " << statement.ColumnInt(5) << L"\r\n";
        report << L"  Revenue: " << FormatMoney(statement.ColumnDouble(6)) << L"\r\n";
        report << L"  Last activity: " << (lastDate.empty() ? L"N/A" : FormatDisplayDate(lastDate)) << L"\r\n\r\n";
    }

    const std::wstring filePath = exportsDir + L"\\client_report_" + TimestampForFile() + L".txt";
    std::wstring error;
    if (!WriteUtf8BomFile(filePath, ToUtf8(report.str()), &error)) {
        SetLastError(error);
        return L"";
    }
    LogAudit(L"report", 0, L"Export", L"Client report exported");
    return filePath;
}

std::wstring Repository::ExportProductProfitReport() {
    if (!EnsureInitialized()) {
        return L"";
    }
    if (!RequirePermission(access::Permission::ExportReports, L"Export product profit report")) {
        return L"";
    }

    const std::wstring exportsDir = EnsureExportsDirectory();
    if (exportsDir.empty()) {
        return L"";
    }

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT products.sku, products.name, products.category, products.price, COALESCE(products.cost, 0), "
            "       COALESCE(products.discount_percent, 0), "
            "       COALESCE(SUM(sales.quantity), 0), COALESCE(SUM(sales.revenue), 0), "
            "       COALESCE(SUM(sales.revenue - (COALESCE(products.cost, 0) * sales.quantity)), 0) "
            "FROM products "
            "LEFT JOIN sales ON sales.product_id = products.id "
            "GROUP BY products.id "
            "ORDER BY COALESCE(SUM(sales.revenue - (COALESCE(products.cost, 0) * sales.quantity)), 0) DESC, products.name ASC;",
            &statement,
            &lastError_)) {
        return L"";
    }

    std::wostringstream report;
    report << L"Product Profit Report\r\n";
    report << L"Generated at: " << CurrentTimestamp() << L"\r\n\r\n";
    while (statement.Step() == SQLITE_ROW) {
        const double price = statement.ColumnDouble(3);
        const double cost = statement.ColumnDouble(4);
        const double discount = statement.ColumnDouble(5);
        const double netUnit = price - cost - (price * discount / 100.0);
        report << statement.ColumnText(0) << L" | " << statement.ColumnText(1) << L"\r\n";
        report << L"  Category: " << statement.ColumnText(2) << L"\r\n";
        report << L"  Price / cost / discount: " << FormatMoney(price) << L" / " << FormatMoney(cost) << L" / " << FormatPercent(discount, 1) << L"\r\n";
        report << L"  Profit per unit: " << FormatMoney(netUnit) << L"\r\n";
        report << L"  Units sold: " << statement.ColumnInt(6) << L"\r\n";
        report << L"  Revenue: " << FormatMoney(statement.ColumnDouble(7)) << L"\r\n";
        report << L"  Estimated profit: " << FormatMoney(statement.ColumnDouble(8)) << L"\r\n\r\n";
    }

    const std::wstring filePath = exportsDir + L"\\product_profit_report_" + TimestampForFile() + L".txt";
    std::wstring error;
    if (!WriteUtf8BomFile(filePath, ToUtf8(report.str()), &error)) {
        SetLastError(error);
        return L"";
    }
    LogAudit(L"report", 0, L"Export", L"Product profit report exported");
    return filePath;
}

std::wstring Repository::ExportCancellationReport() {
    if (!EnsureInitialized()) {
        return L"";
    }
    if (!RequirePermission(access::Permission::ExportReports, L"Export cancellation and returns report")) {
        return L"";
    }

    const std::wstring exportsDir = EnsureExportsDirectory();
    if (exportsDir.empty()) {
        return L"";
    }

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT sales.sale_code, clients.company_name, products.name, employees.full_name, sales.revenue, "
            "       sales.sale_date, sales.status, COALESCE(sales.cancellation_reason, '') "
            "FROM sales "
            "JOIN clients ON clients.id = sales.client_id "
            "JOIN products ON products.id = sales.product_id "
            "JOIN employees ON employees.id = sales.employee_id "
            "WHERE sales.status IN ('Cancelled', 'Returned') "
            "ORDER BY sales.sale_date DESC, sales.id DESC;",
            &statement,
            &lastError_)) {
        return L"";
    }

    std::wostringstream report;
    report << L"Cancellations and Returns Report\r\n";
    report << L"Generated at: " << CurrentTimestamp() << L"\r\n\r\n";
    int count = 0;
    double value = 0.0;
    while (statement.Step() == SQLITE_ROW) {
        ++count;
        value += statement.ColumnDouble(4);
        report << statement.ColumnText(0) << L" | " << statement.ColumnText(6) << L" | " << FormatDisplayDate(statement.ColumnText(5)) << L"\r\n";
        report << L"  Client: " << statement.ColumnText(1) << L"\r\n";
        report << L"  Product: " << statement.ColumnText(2) << L"\r\n";
        report << L"  Manager: " << statement.ColumnText(3) << L"\r\n";
        report << L"  Value: " << FormatMoney(statement.ColumnDouble(4)) << L"\r\n";
        report << L"  Reason: " << (statement.ColumnText(7).empty() ? L"N/A" : statement.ColumnText(7)) << L"\r\n\r\n";
    }
    report << L"Summary: " << count << L" rows, " << FormatMoney(value) << L" affected value.\r\n";

    const std::wstring filePath = exportsDir + L"\\cancellations_returns_" + TimestampForFile() + L".txt";
    std::wstring error;
    if (!WriteUtf8BomFile(filePath, ToUtf8(report.str()), &error)) {
        SetLastError(error);
        return L"";
    }
    LogAudit(L"report", 0, L"Export", L"Cancellation and returns report exported");
    return filePath;
}

std::wstring Repository::ExportAuditLogReport() {
    if (!EnsureInitialized()) {
        return L"";
    }
    if (!RequirePermission(access::Permission::ViewAuditLog, L"Export audit log report")) {
        return L"";
    }
    const auto rows = LoadAuditLog(200);

    const std::wstring exportsDir = EnsureExportsDirectory();
    if (exportsDir.empty()) {
        return L"";
    }

    std::wostringstream report;
    report << L"Audit Log Report\r\n";
    report << L"Generated at: " << CurrentTimestamp() << L"\r\n\r\n";
    for (const auto& row : rows) {
        report << row.createdAt << L" | " << row.userName << L" | " << row.action << L" | "
               << row.entityType << L"#" << row.entityId << L"\r\n";
        report << L"  " << row.details << L"\r\n";
    }

    const std::wstring filePath = exportsDir + L"\\audit_log_" + TimestampForFile() + L".txt";
    std::wstring error;
    if (!WriteUtf8BomFile(filePath, ToUtf8(report.str()), &error)) {
        SetLastError(error);
        return L"";
    }
    LogAudit(L"report", 0, L"Export", L"Audit log report exported");
    return filePath;
}

std::wstring Repository::ExportPdfQueueManifest() {
    if (!EnsureInitialized()) {
        return L"";
    }
    if (!RequirePermission(access::Permission::ExportReports, L"Create PDF queue manifest")) {
        return L"";
    }

    const std::wstring exportsDir = EnsureExportsDirectory();
    if (exportsDir.empty()) {
        return L"";
    }

    std::wostringstream manifest;
    manifest << L"PDF Queue Manifest\r\n";
    manifest << L"Generated at: " << CurrentTimestamp() << L"\r\n";
    manifest << L"Database: " << DatabasePath() << L"\r\n\r\n";
    manifest << L"Queued report packages\r\n";
    manifest << L"  - Daily Sales Summary\r\n";
    manifest << L"  - Order Fulfillment Report\r\n";
    manifest << L"  - Inventory Expiration Watchlist\r\n";
    manifest << L"  - Employee Performance Report\r\n\r\n";
    manifest << L"  - Client Report\r\n";
    manifest << L"  - Product Profit Report\r\n";
    manifest << L"  - Cancellations and Returns Report\r\n";
    manifest << L"\r\n";
    manifest << L"This file is intended for a future PDF renderer pipeline.\r\n";

    const std::wstring filePath = exportsDir + L"\\pdf_queue_manifest_" + TimestampForFile() + L".txt";
    std::wstring error;
    if (!WriteUtf8BomFile(filePath, ToUtf8(manifest.str()), &error)) {
        SetLastError(error);
        return L"";
    }
    return filePath;
}

std::wstring Repository::ArchiveExportsSnapshot() {
    if (!EnsureInitialized()) {
        return L"";
    }
    if (!RequirePermission(access::Permission::ExportReports, L"Archive export files")) {
        return L"";
    }

    const std::wstring exportsDir = EnsureExportsDirectory();
    if (exportsDir.empty()) {
        return L"";
    }

    std::error_code ec;
    const std::filesystem::path sourceDir(exportsDir);
    const std::filesystem::path archiveDir = sourceDir / (L"archive_" + TimestampForFile());
    std::filesystem::create_directories(archiveDir, ec);
    if (ec) {
        SetLastError(L"Unable to create archive folder.");
        return L"";
    }

    bool copiedAny = false;
    for (const auto& entry : std::filesystem::directory_iterator(sourceDir, ec)) {
        if (ec) {
            SetLastError(L"Unable to enumerate export files.");
            return L"";
        }
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::filesystem::path destination = archiveDir / entry.path().filename();
        std::filesystem::copy_file(entry.path(), destination, std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec) {
            copiedAny = true;
        }
        ec.clear();
    }

    if (!copiedAny) {
        std::wstring error;
        const std::wstring readmePath = (archiveDir / L"README.txt").wstring();
        if (!WriteUtf8BomFile(readmePath, ToUtf8(L"No export files were available at the time of archiving.\r\n"), &error)) {
            SetLastError(error);
            return L"";
        }
    }
    return archiveDir.wstring();
}

bool Repository::AddDemoSale() {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::CreateSale, L"Create sale")) {
        return false;
    }

    if (db_.QueryScalarInt("SELECT COUNT(*) FROM employees;", 0) == 0) {
        SqliteStatement employee;
        if (!db_.Prepare(
                "INSERT INTO employees(id, full_name, role, region, status) VALUES(1, ?1, ?2, ?3, ?4);",
                &employee,
                &lastError_)) {
            return false;
        }
        employee.BindText(1, L"SalesFlow Manager");
        employee.BindText(2, L"Sales Manager");
        employee.BindText(3, L"Default Region");
        employee.BindText(4, L"On Track");
        if (employee.Step() != SQLITE_DONE) {
            SetLastError(L"Unable to create a default employee for the sale.");
            return false;
        }
    }

    if (db_.QueryScalarInt("SELECT COUNT(*) FROM clients;", 0) == 0) {
        if (!AddDemoClient()) {
            return false;
        }
    }

    if (db_.QueryScalarInt("SELECT COUNT(*) FROM products WHERE status <> 'Discontinued';", 0) == 0) {
        if (!AddDemoProduct()) {
            return false;
        }
    }

    const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM sales;", 1);
    const int employeeId = EnsureEmployeeForCurrentAccount();
    const bool ownClientScope = hasCurrentAccount_
        && access::ClientScope(access::RoleForAccount(currentAccount_)) == access::DataScope::Own;
    int clientId = ownClientScope
        ? db_.QueryScalarInt(("SELECT id FROM clients WHERE lower(account_manager) = lower(" + QuoteSql(currentAccount_.fullName) + ") ORDER BY id ASC LIMIT 1;").c_str(), 0)
        : db_.QueryScalarInt("SELECT id FROM clients ORDER BY id ASC LIMIT 1;", 1);
    if (clientId <= 0) {
        if (!AddDemoClient()) {
            return false;
        }
        clientId = db_.QueryScalarInt(("SELECT id FROM clients WHERE lower(account_manager) = lower(" + QuoteSql(currentAccount_.fullName) + ") ORDER BY id ASC LIMIT 1;").c_str(), 1);
    }
    const int productId = db_.QueryScalarInt("SELECT id FROM products WHERE status <> 'Discontinued' ORDER BY id ASC LIMIT 1;", 1);

    SqliteStatement product;
    if (!db_.Prepare("SELECT price, margin_percent, stock, COALESCE(cost, 0), COALESCE(discount_percent, 0) FROM products WHERE id = ?1;", &product, &lastError_)) {
        return false;
    }
    product.BindInt(1, productId);
    if (product.Step() != SQLITE_ROW) {
        SetLastError(L"Unable to find a product for the new sale.");
        return false;
    }

    const double price = product.ColumnDouble(0);
    const double margin = product.ColumnDouble(1);
    const int stock = product.ColumnInt(2);
    const double discount = product.ColumnDouble(4);
    const int quantity = std::max(1, std::min(6 + (nextId % 5), std::max(1, stock)));
    const double netUnit = std::round((price - (price * discount / 100.0)) * 100.0) / 100.0;
    const double total = netUnit * quantity;

    std::wostringstream saleCode;
    saleCode << L"SAL-" << (2100 + nextId);

    if (!db_.Execute("BEGIN IMMEDIATE;", &lastError_)) {
        return false;
    }

    {
        SqliteStatement insert;
        if (!db_.Prepare(
                "INSERT INTO sales(id, sale_code, employee_id, client_id, product_id, channel, quantity, unit_price, revenue, total_amount, margin_percent, sale_date, status, payment_method, cancellation_reason) "
                "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15);",
                &insert,
                &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        insert.BindInt(1, nextId);
        insert.BindText(2, saleCode.str());
        insert.BindInt(3, employeeId);
        insert.BindInt(4, clientId);
        insert.BindInt(5, productId);
        insert.BindText(6, (nextId % 3 == 0) ? L"Partner" : ((nextId % 2 == 0) ? L"Online" : L"Direct"));
        insert.BindInt(7, quantity);
        insert.BindDouble(8, netUnit);
        insert.BindDouble(9, total);
        insert.BindDouble(10, total);
        insert.BindDouble(11, margin);
        insert.BindText(12, CurrentDateIso());
        insert.BindText(13, L"Completed");
        insert.BindText(14, (nextId % 2 == 0) ? L"Card" : L"Bank Transfer");
        insert.BindText(15, L"");
        if (insert.Step() != SQLITE_DONE) {
            db_.Execute("ROLLBACK;", nullptr);
            SetLastError(L"Unable to create sale.");
            return false;
        }
    }

    {
        SqliteStatement updateProduct;
        if (!db_.Prepare(
                "UPDATE products "
                "SET stock = MAX(0, stock - ?1), "
                "    status = CASE WHEN MAX(0, stock - ?1) <= reorder_level THEN 'Low Stock' ELSE status END "
                "WHERE id = ?2;",
                &updateProduct,
                &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        updateProduct.BindInt(1, quantity);
        updateProduct.BindInt(2, productId);
        if (updateProduct.Step() != SQLITE_DONE) {
            db_.Execute("ROLLBACK;", nullptr);
            SetLastError(L"Unable to update product stock.");
            return false;
        }
    }

    {
        SqliteStatement updateClient;
        if (db_.Prepare("UPDATE clients SET last_order_date = ?1, lifetime_value = lifetime_value + ?2 WHERE id = ?3;", &updateClient, nullptr)) {
            updateClient.BindText(1, CurrentDateIso());
            updateClient.BindDouble(2, total);
            updateClient.BindInt(3, clientId);
            updateClient.Step();
        }
    }

    if (!db_.Execute("COMMIT;", &lastError_)) {
        db_.Execute("ROLLBACK;", nullptr);
        return false;
    }
    LogAudit(L"sales", nextId, L"Create", L"Sale created from Sales page command");
    return true;
}

bool Repository::AdvanceSaleStatus(int saleId) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::ChangeSaleStatus, L"Change sale status")) {
        return false;
    }
    if (hasCurrentAccount_ && access::SalesScope(access::RoleForAccount(currentAccount_)) == access::DataScope::Own) {
        SqliteStatement ownership;
        if (!db_.Prepare(
                "SELECT employees.full_name FROM sales JOIN employees ON employees.id = sales.employee_id WHERE sales.id = ?1;",
                &ownership,
                &lastError_)) {
            return false;
        }
        ownership.BindInt(1, saleId);
        if (ownership.Step() != SQLITE_ROW || ownership.ColumnText(0) != currentAccount_.fullName) {
            SetLastError(L"You can change status only for your own sales.");
            return false;
        }
    }

    SqliteStatement statement;
    if (!db_.Prepare("SELECT status FROM sales WHERE id = ?1;", &statement, &lastError_)) {
        return false;
    }
    statement.BindInt(1, saleId);
    if (statement.Step() != SQLITE_ROW) {
        SetLastError(L"Select a sale before changing status.");
        return false;
    }

    const std::wstring current = statement.ColumnText(0);
    std::wstring next = L"Pending";
    std::wstring reason;
    if (current == L"Pending") {
        next = L"Completed";
    } else if (current == L"Completed") {
        next = L"Returned";
        reason = L"Marked as return during monitoring review";
    } else if (current == L"Returned") {
        next = L"Cancelled";
        reason = L"Cancelled after return review";
    }

    SqliteStatement update;
    if (!db_.Prepare("UPDATE sales SET status = ?1, cancellation_reason = ?2 WHERE id = ?3;", &update, &lastError_)) {
        return false;
    }
    update.BindText(1, next);
    update.BindText(2, reason);
    update.BindInt(3, saleId);
    if (update.Step() != SQLITE_DONE) {
        SetLastError(L"Unable to update sale status.");
        return false;
    }
    LogAudit(L"sales", saleId, L"UpdateStatus", L"Sale status changed to " + next);
    return true;
}

bool Repository::DeleteSale(int saleId) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::DeleteSale, L"Delete sale")) {
        return false;
    }

    SqliteStatement remove;
    if (!db_.Prepare("DELETE FROM sales WHERE id = ?1;", &remove, &lastError_)) {
        return false;
    }
    remove.BindInt(1, saleId);
    if (remove.Step() != SQLITE_DONE) {
        SetLastError(L"Unable to delete sale.");
        return false;
    }
    LogAudit(L"sales", saleId, L"Delete", L"Sale deleted from Sales page");
    return true;
}

bool Repository::AddDemoOrder() {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::CreateOrder, L"Create order")) {
        return false;
    }

    if (db_.QueryScalarInt("SELECT COUNT(*) FROM clients;", 0) == 0) {
        const int clientId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM clients;", 1);
        std::ostringstream clientSql;
        clientSql << "INSERT INTO clients(id, company_name, contact_name, email, phone, segment, region, is_active, last_order_date, lifetime_value, account_manager) VALUES("
            << clientId << ","
            << QuoteSql(L"New Client") << ","
            << QuoteSql(L"Client Contact") << ","
            << QuoteSql(L"client@salesflow.local") << ","
            << QuoteSql(L"+1 000 000 0000") << ","
            << QuoteSql(L"Mid-Market") << ","
            << QuoteSql(L"North America") << ","
            << 1 << ","
            << QuoteSql(CurrentDateIso()) << ","
            << 0 << ","
            << QuoteSql(L"SalesFlow Administrator") << ");";
        if (!db_.Execute(clientSql.str(), &lastError_)) {
            return false;
        }
    }

    if (db_.QueryScalarInt("SELECT COUNT(*) FROM products WHERE status <> 'Discontinued';", 0) == 0) {
        if (!AddDemoProduct()) {
            return false;
        }
    }

    const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM orders;", 1);
    const int clientId = db_.QueryScalarInt("SELECT id FROM clients ORDER BY id ASC LIMIT 1;", 1);
    const int productId = db_.QueryScalarInt("SELECT id FROM products WHERE status <> 'Discontinued' ORDER BY id ASC LIMIT 1;", 1);
    const int quantity = 12 + (nextId % 8);
    double price = 0.0;
    int stock = 0;
    int reorderLevel = 0;
    SqliteStatement product;
    if (!db_.Prepare("SELECT price, stock, reorder_level FROM products WHERE id = ?1;", &product, &lastError_)) {
        return false;
    }
    product.BindInt(1, productId);
    if (product.Step() != SQLITE_ROW) {
        SetLastError(L"Unable to find a product for the new order.");
        return false;
    }
    price = product.ColumnDouble(0);
    stock = product.ColumnInt(1);
    reorderLevel = product.ColumnInt(2);
    const std::wstring today = CurrentDateIso();

    std::wostringstream orderCode;
    orderCode << L"ORD-" << (1000 + nextId);

    std::ostringstream sql;
    sql << "INSERT INTO orders(id, order_code, client_id, product_id, quantity, total_price, order_date, status, priority) VALUES("
        << nextId << ","
        << QuoteSql(orderCode.str()) << ","
        << clientId << ","
        << productId << ","
        << quantity << ","
        << (quantity * price) << ","
        << QuoteSql(today) << ","
        << QuoteSql(L"Processing") << ","
        << QuoteSql(stock <= reorderLevel ? L"High" : L"Medium") << ");";

    if (!db_.Execute(sql.str(), &lastError_)) {
        return false;
    }

    std::ostringstream updateClient;
    updateClient << "UPDATE clients SET last_order_date = " << QuoteSql(today)
                 << ", lifetime_value = lifetime_value + " << (quantity * price)
                 << " WHERE id = " << clientId << ";";
    db_.Execute(updateClient.str(), nullptr);
    LogAudit(L"orders", nextId, L"Create", L"Order created from Orders page command");
    return true;
}

bool Repository::AdvanceOrderStatus(int orderId) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::ChangeOrderStatus, L"Change order status")) {
        return false;
    }

    SqliteStatement statement;
    if (!db_.Prepare("SELECT status FROM orders WHERE id = ?1;", &statement, &lastError_)) {
        return false;
    }
    statement.BindInt(1, orderId);
    if (statement.Step() != SQLITE_ROW) {
        SetLastError(L"Select an order before editing its status.");
        return false;
    }

    const std::wstring current = statement.ColumnText(0);
    std::wstring next = current;
    if (current == L"Processing") {
        next = L"Confirmed";
    } else if (current == L"Confirmed") {
        next = L"Packed";
    } else if (current == L"Packed") {
        next = L"Delivered";
    }

    std::ostringstream sql;
    sql << "UPDATE orders SET status = " << QuoteSql(next) << " WHERE id = " << orderId << ";";
    if (!db_.Execute(sql.str(), &lastError_)) {
        return false;
    }
    LogAudit(L"orders", orderId, L"UpdateStatus", L"Order status changed to " + next);
    return true;
}

bool Repository::DeleteOrder(int orderId) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::DeleteOrder, L"Delete order")) {
        return false;
    }

    std::ostringstream sql;
    sql << "DELETE FROM orders WHERE id = " << orderId << ";";
    if (!db_.Execute(sql.str(), &lastError_)) {
        return false;
    }
    LogAudit(L"orders", orderId, L"Delete", L"Order deleted from Orders page");
    return true;
}

bool Repository::AddDemoProduct() {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::CreateProduct, L"Create product")) {
        return false;
    }

    const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM products;", 1);
    const wchar_t* categories[] = {L"Electronics", L"Hardware", L"Automation", L"Connectivity", L"Logistics", L"Analytics"};
    const wchar_t* suppliers[] = {L"Connective Labs", L"Retail Insight Works", L"NorthGrid Manufacturing", L"Logisource Ltd."};
    const std::wstring category = categories[nextId % 6];
    const std::wstring supplier = suppliers[nextId % 4];
    const int stock = 18 + (nextId % 70);
    const int reorderLevel = 20 + (nextId % 12);
    const std::wstring status = stock <= reorderLevel ? L"Low Stock" : L"Active";
    const double price = 240.0 + static_cast<double>((nextId % 11) * 37);
    const double margin = 22.0 + static_cast<double>(nextId % 9);
    const double discount = static_cast<double>((nextId % 4) * 2);
    const double cost = std::round((price * (1.0 - margin / 100.0)) * 100.0) / 100.0;

    std::wostringstream sku;
    sku << L"SKU-DM" << std::setw(3) << std::setfill(L'0') << nextId;
    std::wostringstream name;
    name << L"Demo Product " << nextId;
    std::wostringstream batch;
    batch << L"B-DM-" << nextId;
    const int month = ((nextId + 4) % 12) + 1;
    const int day = 8 + (nextId % 18);
    const std::wstring expirationDate = MakeIsoDate(2026 + ((nextId + 4) / 12), month, day);

    SqliteStatement insert;
    if (!db_.Prepare(
            "INSERT INTO products(id, sku, name, category, stock, reorder_level, price, cost, discount_percent, status, supplier, margin_percent, expiration_date, batch_code) "
            "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14);",
            &insert,
            &lastError_)) {
        return false;
    }
    insert.BindInt(1, nextId);
    insert.BindText(2, sku.str());
    insert.BindText(3, name.str());
    insert.BindText(4, category);
    insert.BindInt(5, stock);
    insert.BindInt(6, reorderLevel);
    insert.BindDouble(7, price);
    insert.BindDouble(8, cost);
    insert.BindDouble(9, discount);
    insert.BindText(10, status);
    insert.BindText(11, supplier);
    insert.BindDouble(12, margin);
    insert.BindText(13, expirationDate);
    insert.BindText(14, batch.str());
    if (insert.Step() != SQLITE_DONE) {
        SetLastError(L"Unable to create demo product.");
        return false;
    }
    LogAudit(L"products", nextId, L"Create", L"Product created from Products page command");
    return true;
}

bool Repository::UpdateDemoProduct(int productId) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::EditProduct, L"Edit product")) {
        return false;
    }

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT stock, reorder_level, price, status, margin_percent, expiration_date, COALESCE(discount_percent, 0) "
            "FROM products WHERE id = ?1;",
            &statement,
            &lastError_)) {
        return false;
    }
    statement.BindInt(1, productId);
    if (statement.Step() != SQLITE_ROW) {
        SetLastError(L"Select a product before editing it.");
        return false;
    }

    int stock = statement.ColumnInt(0);
    const int reorderLevel = statement.ColumnInt(1);
    double price = statement.ColumnDouble(2);
    const std::wstring currentStatus = statement.ColumnText(3);
    double margin = statement.ColumnDouble(4);
    const std::wstring expirationDate = statement.ColumnText(5);
    double discount = statement.ColumnDouble(6);

    if (currentStatus == L"Discontinued") {
        stock = std::max(stock, reorderLevel + 6);
    } else if (stock <= reorderLevel) {
        stock += 14;
    } else {
        stock += 5;
    }
    price = std::round((price * 1.03) * 100.0) / 100.0;
    margin = std::min(55.0, margin + 0.6);
    discount = std::min(35.0, discount + 0.5);
    const double cost = std::round((price * (1.0 - margin / 100.0)) * 100.0) / 100.0;
    const std::wstring nextStatus = stock <= reorderLevel ? L"Low Stock" : L"Active";

    SqliteStatement update;
    if (!db_.Prepare(
            "UPDATE products "
            "SET stock = ?1, price = ?2, status = ?3, margin_percent = ?4, expiration_date = ?5, cost = ?6, discount_percent = ?7 "
            "WHERE id = ?8;",
            &update,
            &lastError_)) {
        return false;
    }
    update.BindInt(1, stock);
    update.BindDouble(2, price);
    update.BindText(3, nextStatus);
    update.BindDouble(4, margin);
    update.BindText(5, expirationDate);
    update.BindDouble(6, cost);
    update.BindDouble(7, discount);
    update.BindInt(8, productId);
    if (update.Step() != SQLITE_DONE) {
        SetLastError(L"Unable to update product.");
        return false;
    }
    LogAudit(L"products", productId, L"Update", L"Product stock, price, margin and discount were updated");
    return true;
}

bool Repository::RemoveProduct(int productId) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::DeleteProduct, L"Delete product")) {
        return false;
    }

    if (!db_.Execute("BEGIN IMMEDIATE;", &lastError_)) {
        return false;
    }

    const auto rollback = [this](const std::wstring& message) {
        db_.Execute("ROLLBACK;", nullptr);
        SetLastError(message);
        return false;
    };

    {
        SqliteStatement deleteSales;
        if (!db_.Prepare("DELETE FROM sales WHERE product_id = ?1;", &deleteSales, &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        deleteSales.BindInt(1, productId);
        if (deleteSales.Step() != SQLITE_DONE) {
            return rollback(L"Unable to delete sales linked to the selected product.");
        }
    }

    {
        SqliteStatement deleteOrders;
        if (!db_.Prepare("DELETE FROM orders WHERE product_id = ?1;", &deleteOrders, &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        deleteOrders.BindInt(1, productId);
        if (deleteOrders.Step() != SQLITE_DONE) {
            return rollback(L"Unable to delete orders linked to the selected product.");
        }
    }

    {
        SqliteStatement deleteProduct;
        if (!db_.Prepare("DELETE FROM products WHERE id = ?1;", &deleteProduct, &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        deleteProduct.BindInt(1, productId);
        if (deleteProduct.Step() != SQLITE_DONE) {
            return rollback(L"Unable to delete the selected product.");
        }
    }

    if (!db_.Execute("COMMIT;", &lastError_)) {
        db_.Execute("ROLLBACK;", nullptr);
        return false;
    }
    LogAudit(L"products", productId, L"Delete", L"Product and linked orders/sales were deleted");
    return true;
}

bool Repository::ImportProductsCsv(const std::wstring& filePath, int* importedCount) {
    if (importedCount) {
        *importedCount = 0;
    }
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::ImportExportCatalog, L"Import product catalog")) {
        return false;
    }

    std::ifstream stream(std::filesystem::path(filePath), std::ios::binary);
    if (!stream.is_open()) {
        SetLastError(L"Unable to open the selected CSV file.");
        return false;
    }

    std::string bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (bytes.size() >= 3 &&
        static_cast<unsigned char>(bytes[0]) == 0xEF &&
        static_cast<unsigned char>(bytes[1]) == 0xBB &&
        static_cast<unsigned char>(bytes[2]) == 0xBF) {
        bytes.erase(0, 3);
    }

    std::wistringstream lines(FromUtf8(bytes.c_str()));
    std::wstring line;
    bool firstLine = true;
    int imported = 0;

    if (!db_.Execute("BEGIN IMMEDIATE;", &lastError_)) {
        return false;
    }

    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == L'\r') {
            line.pop_back();
        }
        if (Trim(line).empty()) {
            continue;
        }
        if (firstLine) {
            firstLine = false;
            const std::wstring lowerHeader = line;
            if (lowerHeader.find(L"sku") != std::wstring::npos && lowerHeader.find(L"name") != std::wstring::npos) {
                continue;
            }
        }

        auto cells = SplitCsvLine(line);
        if (cells.size() < 8) {
            db_.Execute("ROLLBACK;", nullptr);
            SetLastError(L"CSV row must contain at least: sku,name,category,stock,reorder,price,status,supplier.");
            return false;
        }

        const std::wstring sku = Trim(cells[0]);
        const std::wstring name = Trim(cells[1]);
        const std::wstring category = Trim(cells[2]);
        const int stock = ParseInt(cells[3], -1);
        const int reorder = ParseInt(cells[4], -1);
        const double price = ParseDouble(cells[5], -1.0);
        const std::wstring status = Trim(cells[6]).empty() ? L"Active" : Trim(cells[6]);
        const std::wstring supplier = Trim(cells[7]);
        const double cost = cells.size() > 8 ? ParseDouble(cells[8], 0.0) : 0.0;
        const double discount = cells.size() > 9 ? ParseDouble(cells[9], 0.0) : 0.0;
        const std::wstring expiration = cells.size() > 10 && !Trim(cells[10]).empty() ? Trim(cells[10]) : L"2026-12-31";
        const std::wstring batch = cells.size() > 11 && !Trim(cells[11]).empty() ? Trim(cells[11]) : L"CSV-IMPORT";

        if (sku.empty() || name.empty() || category.empty() || supplier.empty() || stock < 0 || reorder < 0 || price < 0.0 || cost < 0.0 || discount < 0.0 || discount > 100.0) {
            db_.Execute("ROLLBACK;", nullptr);
            SetLastError(L"CSV contains invalid product values. Required fields cannot be empty and numbers cannot be negative.");
            return false;
        }

        const double margin = price > 0.0 ? std::max(0.0, ((price - cost) / price) * 100.0) : 0.0;
        const int id = db_.QueryScalarInt(("SELECT id FROM products WHERE sku = " + QuoteSql(sku) + " LIMIT 1;").c_str(), 0);
        const int nextId = id > 0 ? id : db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM products;", 1);

        SqliteStatement upsert;
        if (!db_.Prepare(
                "INSERT INTO products(id, sku, name, category, stock, reorder_level, price, cost, discount_percent, status, supplier, margin_percent, expiration_date, batch_code) "
                "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14) "
                "ON CONFLICT(sku) DO UPDATE SET "
                "name = excluded.name, category = excluded.category, stock = excluded.stock, reorder_level = excluded.reorder_level, "
                "price = excluded.price, cost = excluded.cost, discount_percent = excluded.discount_percent, status = excluded.status, "
                "supplier = excluded.supplier, margin_percent = excluded.margin_percent, expiration_date = excluded.expiration_date, batch_code = excluded.batch_code;",
                &upsert,
                &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        upsert.BindInt(1, nextId);
        upsert.BindText(2, sku);
        upsert.BindText(3, name);
        upsert.BindText(4, category);
        upsert.BindInt(5, stock);
        upsert.BindInt(6, reorder);
        upsert.BindDouble(7, price);
        upsert.BindDouble(8, cost);
        upsert.BindDouble(9, discount);
        upsert.BindText(10, status);
        upsert.BindText(11, supplier);
        upsert.BindDouble(12, margin);
        upsert.BindText(13, expiration);
        upsert.BindText(14, batch);
        if (upsert.Step() != SQLITE_DONE) {
            db_.Execute("ROLLBACK;", nullptr);
            SetLastError(L"Unable to import product row.");
            return false;
        }
        ++imported;
    }

    if (!db_.Execute("COMMIT;", &lastError_)) {
        db_.Execute("ROLLBACK;", nullptr);
        return false;
    }
    if (importedCount) {
        *importedCount = imported;
    }
    LogAudit(L"products", 0, L"Import", L"Products imported from CSV: " + std::to_wstring(imported));
    return true;
}

bool Repository::CreateQuickEntry(const std::wstring& entryType, const std::wstring& primaryValue, const std::wstring& secondaryValue, const std::wstring& quantityValue, const std::wstring& amountValue) {
    if (!EnsureInitialized()) {
        return false;
    }

    const std::wstring type = Trim(entryType);
    const std::wstring primary = Trim(primaryValue);
    const std::wstring secondary = Trim(secondaryValue);
    const std::wstring quantityText = Trim(quantityValue);
    const std::wstring amountText = Trim(amountValue);
    const bool isProduct = type == L"Product" || type == L"Товар";
    const bool isClient = type == L"Client" || type == L"Клиент";
    const bool isSale = type == L"Sale" || type == L"Продажа";
    const bool isOrder = type == L"Order" || type == L"Заказ";
    const bool isEmployee = type == L"Employee" || type == L"Сотрудник";
    const bool isTask = type == L"Task" || type == L"Задача";

    if (isProduct) {
        if (!RequirePermission(access::Permission::CreateProduct, L"Create product")) {
            return false;
        }
        const std::wstring sku = primary;
        const std::wstring name = secondary;
        const int stock = ParseInt(quantityText, -1);
        const double price = ParseDouble(amountText, -1.0);
        if (sku.empty() || name.empty() || stock < 0 || price <= 0.0) {
            SetLastError(L"Product entry requires SKU, product name, non-negative stock, and positive price.");
            return false;
        }
        if (db_.QueryScalarInt(("SELECT COUNT(*) FROM products WHERE sku = " + QuoteSql(sku) + ";").c_str(), 0) > 0) {
            SetLastError(L"Product SKU already exists.");
            return false;
        }

        const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM products;", 1);
        const int reorder = std::max(5, stock / 3);
        const double cost = std::round(price * 0.72 * 100.0) / 100.0;
        const double margin = price > 0.0 ? ((price - cost) / price) * 100.0 : 0.0;
        SqliteStatement insert;
        if (!db_.Prepare(
                "INSERT INTO products(id, sku, name, category, stock, reorder_level, price, cost, discount_percent, status, supplier, margin_percent, expiration_date, batch_code) "
                "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, 0, ?9, ?10, ?11, ?12, ?13);",
                &insert,
                &lastError_)) {
            return false;
        }
        insert.BindInt(1, nextId);
        insert.BindText(2, sku);
        insert.BindText(3, name);
        insert.BindText(4, L"Manual Entry");
        insert.BindInt(5, stock);
        insert.BindInt(6, reorder);
        insert.BindDouble(7, price);
        insert.BindDouble(8, cost);
        insert.BindText(9, stock <= reorder ? L"Low Stock" : L"Active");
        insert.BindText(10, L"Manual Supplier");
        insert.BindDouble(11, margin);
        insert.BindText(12, L"2026-12-31");
        insert.BindText(13, L"MANUAL");
        if (insert.Step() != SQLITE_DONE) {
            SetLastError(L"Unable to create product from quick entry.");
            return false;
        }
        LogAudit(L"products", nextId, L"Create", L"Product created from Quick Data Entry");
        return true;
    }

    if (isClient) {
        if (!RequirePermission(access::Permission::CreateClient, L"Create client")) {
            return false;
        }
        const std::wstring company = primary;
        const std::wstring contact = secondary;
        const std::wstring email = quantityText.empty() ? L"client@salesflow.local" : quantityText;
        const std::wstring phone = amountText.empty() ? L"+1 000 000 0000" : amountText;
        if (company.empty() || contact.empty()) {
            SetLastError(L"Client entry requires company and contact name.");
            return false;
        }

        const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM clients;", 1);
        SqliteStatement insert;
        if (!db_.Prepare(
                "INSERT INTO clients(id, company_name, contact_name, email, phone, segment, region, is_active, last_order_date, lifetime_value, account_manager, manager_employee_id) "
                "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, 1, ?8, 0, ?9, ?10);",
                &insert,
                &lastError_)) {
            return false;
        }
        insert.BindInt(1, nextId);
        insert.BindText(2, company);
        insert.BindText(3, contact);
        insert.BindText(4, email);
        insert.BindText(5, phone);
        insert.BindText(6, L"Mid-Market");
        insert.BindText(7, L"Manual Region");
        insert.BindText(8, CurrentDateIso());
        insert.BindText(9, hasCurrentAccount_ && !currentAccount_.fullName.empty() ? currentAccount_.fullName : L"SalesFlow Manager");
        insert.BindInt(10, EnsureEmployeeForCurrentAccount());
        if (insert.Step() != SQLITE_DONE) {
            SetLastError(L"Unable to create client from quick entry.");
            return false;
        }
        LogAudit(L"clients", nextId, L"Create", L"Client created from Quick Data Entry");
        return true;
    }

    if (isSale) {
        if (!RequirePermission(access::Permission::CreateSale, L"Create sale")) {
            return false;
        }
        const int clientId = ParseInt(primary, 0) > 0
            ? ParseInt(primary, 0)
            : db_.QueryScalarInt(("SELECT id FROM clients WHERE lower(company_name) = lower(" + QuoteSql(primary) + ") LIMIT 1;").c_str(), 0);
        const int productId = ParseInt(secondary, 0) > 0
            ? ParseInt(secondary, 0)
            : db_.QueryScalarInt(("SELECT id FROM products WHERE lower(sku) = lower(" + QuoteSql(secondary) + ") OR lower(name) = lower(" + QuoteSql(secondary) + ") LIMIT 1;").c_str(), 0);
        const int quantity = ParseInt(quantityText, -1);
        const std::wstring payment = amountText.empty() ? L"Card" : amountText;
        if (clientId <= 0 || productId <= 0 || quantity <= 0) {
            SetLastError(L"Sale entry requires existing client id/name, product id/SKU/name, and positive quantity.");
            return false;
        }
        if (hasCurrentAccount_ && access::ClientScope(access::RoleForAccount(currentAccount_)) == access::DataScope::Own) {
            SqliteStatement ownership;
            if (!db_.Prepare("SELECT account_manager FROM clients WHERE id = ?1;", &ownership, &lastError_)) {
                return false;
            }
            ownership.BindInt(1, clientId);
            if (ownership.Step() != SQLITE_ROW || ownership.ColumnText(0) != currentAccount_.fullName) {
                SetLastError(L"You can create sales only for your own clients.");
                return false;
            }
        }

        if (db_.QueryScalarInt("SELECT COUNT(*) FROM employees;", 0) == 0) {
            SqliteStatement employee;
            if (!db_.Prepare("INSERT INTO employees(id, full_name, role, region, status) VALUES(1, ?1, ?2, ?3, ?4);", &employee, &lastError_)) {
                return false;
            }
            employee.BindText(1, L"SalesFlow Manager");
            employee.BindText(2, L"Sales Manager");
            employee.BindText(3, L"Manual Region");
            employee.BindText(4, L"On Track");
            if (employee.Step() != SQLITE_DONE) {
                SetLastError(L"Unable to create default employee for quick sale.");
                return false;
            }
        }

        SqliteStatement product;
        if (!db_.Prepare("SELECT price, margin_percent, stock, COALESCE(discount_percent, 0) FROM products WHERE id = ?1;", &product, &lastError_)) {
            return false;
        }
        product.BindInt(1, productId);
        if (product.Step() != SQLITE_ROW) {
            SetLastError(L"Selected product was not found.");
            return false;
        }
        const double price = product.ColumnDouble(0);
        const double margin = product.ColumnDouble(1);
        const int stock = product.ColumnInt(2);
        const double discount = product.ColumnDouble(3);
        if (stock < quantity) {
            SetLastError(L"Not enough stock for the quick sale.");
            return false;
        }

        const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM sales;", 1);
        const int employeeId = EnsureEmployeeForCurrentAccount();
        const double unitPrice = std::round((price - price * discount / 100.0) * 100.0) / 100.0;
        const double total = unitPrice * quantity;
        std::wostringstream saleCode;
        saleCode << L"SAL-" << (2100 + nextId);

        if (!db_.Execute("BEGIN IMMEDIATE;", &lastError_)) {
            return false;
        }
        SqliteStatement insert;
        if (!db_.Prepare(
                "INSERT INTO sales(id, sale_code, employee_id, client_id, product_id, channel, quantity, unit_price, revenue, total_amount, margin_percent, sale_date, status, payment_method, cancellation_reason) "
                "VALUES(?1, ?2, ?3, ?4, ?5, 'Direct', ?6, ?7, ?8, ?9, ?10, ?11, 'Completed', ?12, '');",
                &insert,
                &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        insert.BindInt(1, nextId);
        insert.BindText(2, saleCode.str());
        insert.BindInt(3, employeeId);
        insert.BindInt(4, clientId);
        insert.BindInt(5, productId);
        insert.BindInt(6, quantity);
        insert.BindDouble(7, unitPrice);
        insert.BindDouble(8, total);
        insert.BindDouble(9, total);
        insert.BindDouble(10, margin);
        insert.BindText(11, CurrentDateIso());
        insert.BindText(12, payment);
        if (insert.Step() != SQLITE_DONE) {
            db_.Execute("ROLLBACK;", nullptr);
            SetLastError(L"Unable to create sale from quick entry.");
            return false;
        }
        std::ostringstream update;
        update << "UPDATE products SET stock = stock - " << quantity
               << ", status = CASE WHEN stock - " << quantity << " <= reorder_level THEN 'Low Stock' ELSE status END WHERE id = " << productId << ";";
        if (!db_.Execute(update.str(), &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        std::ostringstream clientUpdate;
        clientUpdate << "UPDATE clients SET lifetime_value = lifetime_value + " << total
                     << ", last_order_date = " << QuoteSql(CurrentDateIso()) << " WHERE id = " << clientId << ";";
        db_.Execute(clientUpdate.str(), nullptr);
        if (!db_.Execute("COMMIT;", &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        LogAudit(L"sales", nextId, L"Create", L"Sale created from Quick Data Entry");
        return true;
    }

    if (isOrder) {
        if (!RequirePermission(access::Permission::CreateOrder, L"Create order")) {
            return false;
        }
        const int clientId = ParseInt(primary, 0) > 0
            ? ParseInt(primary, 0)
            : db_.QueryScalarInt(("SELECT id FROM clients WHERE lower(company_name) = lower(" + QuoteSql(primary) + ") LIMIT 1;").c_str(), 0);
        const int productId = ParseInt(secondary, 0) > 0
            ? ParseInt(secondary, 0)
            : db_.QueryScalarInt(("SELECT id FROM products WHERE lower(sku) = lower(" + QuoteSql(secondary) + ") OR lower(name) = lower(" + QuoteSql(secondary) + ") LIMIT 1;").c_str(), 0);
        const int quantity = ParseInt(quantityText, -1);
        const std::wstring status = amountText.empty() ? L"Processing" : amountText;
        if (clientId <= 0 || productId <= 0 || quantity <= 0) {
            SetLastError(L"Order entry requires existing client id/name, product id/SKU/name, and positive quantity.");
            return false;
        }

        SqliteStatement product;
        if (!db_.Prepare("SELECT price, stock, reorder_level FROM products WHERE id = ?1;", &product, &lastError_)) {
            return false;
        }
        product.BindInt(1, productId);
        if (product.Step() != SQLITE_ROW) {
            SetLastError(L"Selected product was not found.");
            return false;
        }
        const double price = product.ColumnDouble(0);
        const int stock = product.ColumnInt(1);
        const int reorderLevel = product.ColumnInt(2);
        const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM orders;", 1);
        std::wostringstream orderCode;
        orderCode << L"ORD-" << (1000 + nextId);

        SqliteStatement insert;
        if (!db_.Prepare(
                "INSERT INTO orders(id, order_code, client_id, product_id, quantity, total_price, order_date, status, priority) "
                "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9);",
                &insert,
                &lastError_)) {
            return false;
        }
        insert.BindInt(1, nextId);
        insert.BindText(2, orderCode.str());
        insert.BindInt(3, clientId);
        insert.BindInt(4, productId);
        insert.BindInt(5, quantity);
        insert.BindDouble(6, price * quantity);
        insert.BindText(7, CurrentDateIso());
        insert.BindText(8, status);
        insert.BindText(9, stock <= reorderLevel ? L"High" : L"Medium");
        if (insert.Step() != SQLITE_DONE) {
            SetLastError(L"Unable to create order from quick entry.");
            return false;
        }
        std::ostringstream updateClient;
        updateClient << "UPDATE clients SET last_order_date = " << QuoteSql(CurrentDateIso())
                     << ", lifetime_value = lifetime_value + " << (price * quantity)
                     << " WHERE id = " << clientId << ";";
        db_.Execute(updateClient.str(), nullptr);
        LogAudit(L"orders", nextId, L"Create", L"Order created from Quick Data Entry");
        return true;
    }

    if (isEmployee) {
        if (!RequirePermission(access::Permission::ManageUsers, L"Create employee")) {
            return false;
        }
        const std::wstring fullName = primary;
        const std::wstring role = secondary.empty() ? L"Sales Manager" : secondary;
        const std::wstring region = quantityText.empty() ? L"Manual Region" : quantityText;
        const std::wstring status = amountText.empty() ? L"On Track" : amountText;
        if (fullName.empty()) {
            SetLastError(L"Employee entry requires full name.");
            return false;
        }

        const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM employees;", 1);
        SqliteStatement insert;
        if (!db_.Prepare("INSERT INTO employees(id, full_name, role, region, status) VALUES(?1, ?2, ?3, ?4, ?5);", &insert, &lastError_)) {
            return false;
        }
        insert.BindInt(1, nextId);
        insert.BindText(2, fullName);
        insert.BindText(3, role);
        insert.BindText(4, region);
        insert.BindText(5, status);
        if (insert.Step() != SQLITE_DONE) {
            SetLastError(L"Unable to create employee from quick entry.");
            return false;
        }
        LogAudit(L"employees", nextId, L"Create", L"Employee created from Quick Data Entry");
        return true;
    }

    if (isTask) {
        if (!RequirePermission(access::Permission::ManageTasks, L"Create task")) {
            return false;
        }
        const std::wstring title = primary;
        const std::wstring assignedTo = secondary.empty() ? (hasCurrentAccount_ ? currentAccount_.fullName : L"SalesFlow Manager") : secondary;
        const std::wstring dueDate = quantityText.empty() ? CurrentDateIso() : quantityText;
        const std::wstring priority = amountText.empty() ? L"Medium" : amountText;
        if (title.empty()) {
            SetLastError(L"Task entry requires a title.");
            return false;
        }

        const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM tasks;", 1);
        SqliteStatement insert;
        if (!db_.Prepare(
                "INSERT INTO tasks(id, title, assigned_to, due_date, status, priority, related_client_id, created_at) "
                "VALUES(?1, ?2, ?3, ?4, 'Open', ?5, ?6, ?7);",
                &insert,
                &lastError_)) {
            return false;
        }
        insert.BindInt(1, nextId);
        insert.BindText(2, title);
        insert.BindText(3, assignedTo);
        insert.BindText(4, dueDate);
        insert.BindText(5, priority);
        insert.BindNull(6);
        insert.BindText(7, CurrentTimestamp());
        if (insert.Step() != SQLITE_DONE) {
            SetLastError(L"Unable to create task from quick entry.");
            return false;
        }
        LogAudit(L"tasks", nextId, L"Create", L"Task created from Quick Data Entry");
        return true;
    }

    SetLastError(L"Select Product, Client, Sale, Order, Employee, or Task as quick entry type.");
    return false;
}

bool Repository::AddDemoClient() {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::CreateClient, L"Create client")) {
        return false;
    }

    const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM clients;", 1);
    std::wostringstream company;
    company << L"New Client " << nextId;
    std::wostringstream email;
    email << L"client" << nextId << L"@salesflow.local";

    SqliteStatement insert;
    if (!db_.Prepare(
            "INSERT INTO clients(id, company_name, contact_name, email, phone, segment, region, is_active, last_order_date, lifetime_value, account_manager, manager_employee_id) "
            "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, 1, ?8, 0, ?9, ?10);",
            &insert,
            &lastError_)) {
        return false;
    }
    insert.BindInt(1, nextId);
    insert.BindText(2, company.str());
    insert.BindText(3, L"Primary Contact");
    insert.BindText(4, email.str());
    insert.BindText(5, L"+1 000 000 0000");
    insert.BindText(6, nextId % 2 == 0 ? L"Key Account" : L"Mid-Market");
    insert.BindText(7, L"Default Region");
    insert.BindText(8, CurrentDateIso());
    insert.BindText(9, hasCurrentAccount_ && !currentAccount_.fullName.empty() ? currentAccount_.fullName : L"SalesFlow Manager");
    insert.BindInt(10, EnsureEmployeeForCurrentAccount());
    if (insert.Step() != SQLITE_DONE) {
        SetLastError(L"Unable to create client.");
        return false;
    }
    LogAudit(L"clients", nextId, L"Create", L"Client created from Clients page command");
    return true;
}

bool Repository::UpdateDemoClient(int clientId) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::EditClient, L"Edit client")) {
        return false;
    }
    if (hasCurrentAccount_ && access::ClientScope(access::RoleForAccount(currentAccount_)) == access::DataScope::Own) {
        SqliteStatement ownership;
        if (!db_.Prepare("SELECT account_manager FROM clients WHERE id = ?1;", &ownership, &lastError_)) {
            return false;
        }
        ownership.BindInt(1, clientId);
        if (ownership.Step() != SQLITE_ROW || ownership.ColumnText(0) != currentAccount_.fullName) {
            SetLastError(L"You can edit only your own clients.");
            return false;
        }
    }

    SqliteStatement update;
    if (!db_.Prepare(
            "UPDATE clients "
            "SET segment = CASE segment WHEN 'Key Account' THEN 'Mid-Market' WHEN 'Mid-Market' THEN 'Regional Partner' ELSE 'Key Account' END, "
            "    lifetime_value = lifetime_value + 1250, "
            "    last_order_date = ?1 "
            "WHERE id = ?2;",
            &update,
            &lastError_)) {
        return false;
    }
    update.BindText(1, CurrentDateIso());
    update.BindInt(2, clientId);
    if (update.Step() != SQLITE_DONE) {
        SetLastError(L"Unable to update client.");
        return false;
    }
    LogAudit(L"clients", clientId, L"Update", L"Client segment and lifetime value were updated");
    return true;
}

bool Repository::DeleteClient(int clientId) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::DeleteClient, L"Delete client")) {
        return false;
    }

    if (!db_.Execute("BEGIN IMMEDIATE;", &lastError_)) {
        return false;
    }

    const auto rollback = [this](const std::wstring& message) {
        db_.Execute("ROLLBACK;", nullptr);
        SetLastError(message);
        return false;
    };

    {
        SqliteStatement removeTasks;
        if (!db_.Prepare("DELETE FROM tasks WHERE related_client_id = ?1;", &removeTasks, &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        removeTasks.BindInt(1, clientId);
        if (removeTasks.Step() != SQLITE_DONE) {
            return rollback(L"Unable to delete tasks linked to the selected client.");
        }
    }

    {
        SqliteStatement removePipeline;
        if (!db_.Prepare("DELETE FROM pipeline_deals WHERE client_id = ?1;", &removePipeline, &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        removePipeline.BindInt(1, clientId);
        if (removePipeline.Step() != SQLITE_DONE) {
            return rollback(L"Unable to delete pipeline deals linked to the selected client.");
        }
    }

    {
        SqliteStatement removeSales;
        if (!db_.Prepare("DELETE FROM sales WHERE client_id = ?1;", &removeSales, &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        removeSales.BindInt(1, clientId);
        if (removeSales.Step() != SQLITE_DONE) {
            return rollback(L"Unable to delete sales linked to the selected client.");
        }
    }

    {
        SqliteStatement removeOrders;
        if (!db_.Prepare("DELETE FROM orders WHERE client_id = ?1;", &removeOrders, &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        removeOrders.BindInt(1, clientId);
        if (removeOrders.Step() != SQLITE_DONE) {
            return rollback(L"Unable to delete orders linked to the selected client.");
        }
    }

    {
        SqliteStatement removeClient;
        if (!db_.Prepare("DELETE FROM clients WHERE id = ?1;", &removeClient, &lastError_)) {
            db_.Execute("ROLLBACK;", nullptr);
            return false;
        }
        removeClient.BindInt(1, clientId);
        if (removeClient.Step() != SQLITE_DONE) {
            return rollback(L"Unable to delete client.");
        }
    }

    if (!db_.Execute("COMMIT;", &lastError_)) {
        db_.Execute("ROLLBACK;", nullptr);
        return false;
    }
    LogAudit(L"clients", clientId, L"Delete", L"Client and linked orders/sales were deleted");
    return true;
}

bool Repository::AddDemoAccount() {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::ManageUsers, L"Create user account")) {
        return false;
    }

    const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM users;", 1);
    const wchar_t* roles[] = {L"Sales Manager", L"Head of Sales", L"Product Manager"};
    std::wostringstream fullName;
    fullName << L"Demo User " << nextId;
    std::wostringstream username;
    username << L"demo.user" << nextId;

    std::ostringstream sql;
    sql << "INSERT INTO users(id, full_name, username, email, role, password_hash, status, last_login, created_at) VALUES("
        << nextId << ","
        << QuoteSql(fullName.str()) << ","
        << QuoteSql(username.str()) << ","
        << QuoteSql(username.str() + L"@company.example") << ","
        << QuoteSql(roles[nextId % 3]) << ","
        << QuoteSql(DemoPasswordHash(L"demo123")) << ","
        << QuoteSql(L"Active") << ","
        << QuoteSql(L"Never") << ","
        << QuoteSql(CurrentDateIso()) << ");";
    if (!db_.Execute(sql.str(), &lastError_)) {
        return false;
    }
    LogAudit(L"users", nextId, L"Create", L"User account created from Settings page");
    return true;
}

bool Repository::ToggleAccountStatus(int userId) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::ManageUsers, L"Lock or unlock user account")) {
        return false;
    }

    SqliteStatement statement;
    if (!db_.Prepare("SELECT status FROM users WHERE id = ?1;", &statement, &lastError_)) {
        return false;
    }
    statement.BindInt(1, userId);
    if (statement.Step() != SQLITE_ROW) {
        SetLastError(L"Select an account before changing its state.");
        return false;
    }

    const std::wstring current = statement.ColumnText(0);
    const std::wstring next = current == L"Active" ? L"Suspended" : L"Active";
    std::ostringstream sql;
    sql << "UPDATE users SET status = " << QuoteSql(next) << " WHERE id = " << userId << ";";
    if (!db_.Execute(sql.str(), &lastError_)) {
        return false;
    }
    LogAudit(L"users", userId, L"ToggleStatus", L"Account status changed to " + next);
    return true;
}

bool Repository::ResetAccountPassword(int userId) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::ManageUsers, L"Reset user account password")) {
        return false;
    }

    std::ostringstream sql;
    sql << "UPDATE users SET password_hash = " << QuoteSql(DemoPasswordHash(L"demo123")) << ", status = " << QuoteSql(L"Active")
        << " WHERE id = " << userId << ";";
    if (!db_.Execute(sql.str(), &lastError_)) {
        return false;
    }
    LogAudit(L"users", userId, L"ResetPassword", L"Account password was reset to temporary value");
    return true;
}

bool Repository::CycleAccountRole(int userId) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::ManageRoles, L"Change user role")) {
        return false;
    }

    SqliteStatement statement;
    if (!db_.Prepare("SELECT role FROM users WHERE id = ?1;", &statement, &lastError_)) {
        return false;
    }
    statement.BindInt(1, userId);
    if (statement.Step() != SQLITE_ROW) {
        SetLastError(L"Select an account before changing its role.");
        return false;
    }

    const std::wstring current = statement.ColumnText(0);
    std::wstring next = L"Sales Manager";
    if (current == L"Sales Manager" || current == L"Manager" || current == L"Sales Operator" || current == L"Account Manager") {
        next = L"Head of Sales";
    } else if (current == L"Head of Sales" || current == L"Supervisor" || current == L"Regional Sales Manager") {
        next = L"Director";
    } else if (current == L"Director" || current == L"Commercial Director" || current == L"Sales Analyst") {
        next = L"Product Manager";
    } else if (current == L"Product Manager" || current == L"Procurement Controller") {
        next = L"System Administrator";
    }

    SqliteStatement update;
    if (!db_.Prepare("UPDATE users SET role = ?1 WHERE id = ?2;", &update, &lastError_)) {
        return false;
    }
    update.BindText(1, next);
    update.BindInt(2, userId);
    if (update.Step() != SQLITE_DONE) {
        SetLastError(L"Unable to change account role.");
        return false;
    }
    LogAudit(L"users", userId, L"ChangeRole", L"Account role changed to " + next);
    return true;
}

bool Repository::DeleteAccount(int userId) {
    if (!EnsureInitialized()) {
        return false;
    }
    if (!RequirePermission(access::Permission::ManageUsers, L"Delete user account")) {
        return false;
    }

    if (hasCurrentAccount_ && currentAccount_.id == userId) {
        SetLastError(L"The current signed-in account cannot delete itself.");
        return false;
    }
    if (db_.QueryScalarInt("SELECT COUNT(*) FROM users;", 0) <= 1) {
        SetLastError(L"At least one user account must remain.");
        return false;
    }

    SqliteStatement remove;
    if (!db_.Prepare("DELETE FROM users WHERE id = ?1;", &remove, &lastError_)) {
        return false;
    }
    remove.BindInt(1, userId);
    if (remove.Step() != SQLITE_DONE) {
        SetLastError(L"Unable to delete account.");
        return false;
    }
    LogAudit(L"users", userId, L"Delete", L"User account was deleted");
    return true;
}

bool Repository::AuthenticateUser(const std::wstring& username, const std::wstring& password, AccountRecord* account) {
    if (!EnsureInitialized()) {
        return false;
    }

    const std::wstring normalizedUsername = Trim(username);
    if (normalizedUsername.empty() || password.empty()) {
        SetLastError(L"Enter username and password.");
        return false;
    }

    SqliteStatement statement;
    if (!db_.Prepare(
            "SELECT id, full_name, username, email, role, password_hash, status, COALESCE(last_login, 'Never'), created_at "
            "FROM users "
            "WHERE lower(username) = lower(?1) OR lower(email) = lower(?1) "
            "LIMIT 1;",
            &statement,
            &lastError_)) {
        return false;
    }
    statement.BindText(1, normalizedUsername);

    if (statement.Step() != SQLITE_ROW) {
        SetLastError(L"Account was not found.");
        return false;
    }

    AccountRecord record{};
    record.id = statement.ColumnInt(0);
    record.fullName = statement.ColumnText(1);
    record.username = statement.ColumnText(2);
    record.email = statement.ColumnText(3);
    record.role = statement.ColumnText(4);
    const std::wstring storedHash = statement.ColumnText(5);
    record.status = statement.ColumnText(6);
    record.lastLogin = statement.ColumnText(7);
    record.createdAt = statement.ColumnText(8);

    if (record.status != L"Active") {
        SetLastError(L"This account is suspended. Unlock it in Settings.");
        return false;
    }

    const std::wstring expectedHash = DemoPasswordHash(password);
    const bool isModernHash = (storedHash == expectedHash);
    const bool isLegacySeedHash = (storedHash == L"demo_hash_2026" && password == L"demo123");
    const bool isLegacyPlainText = (storedHash == password && password == L"Temp#2026");
    const bool passwordMatches = isModernHash || isLegacySeedHash || isLegacyPlainText;
    if (!passwordMatches) {
        SetLastError(L"Incorrect username or password.");
        return false;
    }

    if (!isModernHash) {
        SqliteStatement upgrade;
        if (db_.Prepare("UPDATE users SET password_hash = ?1 WHERE id = ?2;", &upgrade, &lastError_)) {
            upgrade.BindText(1, expectedHash);
            upgrade.BindInt(2, record.id);
            upgrade.Step();
        }
    }

    SqliteStatement update;
    if (db_.Prepare("UPDATE users SET last_login = ?1 WHERE id = ?2;", &update, &lastError_)) {
        update.BindText(1, CurrentTimestamp());
        update.BindInt(2, record.id);
        update.Step();
    }

    record.lastLogin = CurrentTimestamp();
    currentAccount_ = record;
    hasCurrentAccount_ = true;
    if (account) {
        *account = record;
    }
    LogAudit(L"users", record.id, L"Login", L"User signed in");
    return true;
}

bool Repository::RegisterAccount(const std::wstring& fullName, const std::wstring& username, const std::wstring& email, const std::wstring& password) {
    if (!EnsureInitialized()) {
        return false;
    }

    const std::wstring cleanName = Trim(fullName);
    const std::wstring cleanUsername = Trim(username);
    const std::wstring cleanEmail = Trim(email);
    if (cleanName.empty() || cleanUsername.empty() || password.size() < 4) {
        SetLastError(L"Enter full name, username, and a password with at least 4 characters.");
        return false;
    }

    SqliteStatement check;
    if (!db_.Prepare("SELECT COUNT(*) FROM users WHERE lower(username) = lower(?1) OR lower(email) = lower(?2);", &check, &lastError_)) {
        return false;
    }
    check.BindText(1, cleanUsername);
    check.BindText(2, cleanEmail.empty() ? cleanUsername : cleanEmail);
    if (check.Step() == SQLITE_ROW && check.ColumnInt(0) > 0) {
        SetLastError(L"An account with this username or email already exists.");
        return false;
    }

    const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM users;", 1);
    SqliteStatement insert;
    if (!db_.Prepare(
            "INSERT INTO users(id, full_name, username, email, role, password_hash, status, last_login, created_at) "
            "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9);",
            &insert,
            &lastError_)) {
        return false;
    }
    insert.BindInt(1, nextId);
    insert.BindText(2, cleanName);
    insert.BindText(3, cleanUsername);
    insert.BindText(4, cleanEmail.empty() ? cleanUsername + L"@company.example" : cleanEmail);
    insert.BindText(5, L"Sales Manager");
    insert.BindText(6, DemoPasswordHash(password));
    insert.BindText(7, L"Active");
    insert.BindText(8, L"Never");
    insert.BindText(9, CurrentDateIso());
    if (insert.Step() != SQLITE_DONE) {
        SetLastError(L"Unable to register account.");
        return false;
    }
    LogAudit(L"users", nextId, L"Register", L"New account registered from login window");
    return true;
}

AccountRecord Repository::CurrentAccount() const {
    return currentAccount_;
}

bool Repository::HasCurrentAccount() const {
    return hasCurrentAccount_;
}

bool Repository::IsConnected() const {
    return initialized_ && db_.IsOpen();
}

const std::wstring& Repository::LastError() const {
    return lastError_;
}

bool Repository::EnsureSchema() {
    static const char* kSchemaSql = R"sql(
CREATE TABLE IF NOT EXISTS app_settings (
    setting_key TEXT PRIMARY KEY,
    setting_value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY,
    full_name TEXT NOT NULL,
    username TEXT NOT NULL UNIQUE,
    email TEXT NOT NULL,
    role TEXT NOT NULL,
    password_hash TEXT NOT NULL,
    status TEXT NOT NULL,
    last_login TEXT,
    created_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS roles (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    description TEXT NOT NULL,
    default_start_page TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS role_permissions (
    role_id INTEGER NOT NULL,
    permission TEXT NOT NULL,
    scope TEXT NOT NULL DEFAULT 'none',
    PRIMARY KEY (role_id, permission),
    FOREIGN KEY (role_id) REFERENCES roles(id)
);

CREATE TABLE IF NOT EXISTS clients (
    id INTEGER PRIMARY KEY,
    company_name TEXT NOT NULL,
    contact_name TEXT NOT NULL,
    email TEXT NOT NULL,
    phone TEXT NOT NULL,
    segment TEXT NOT NULL,
    region TEXT NOT NULL,
    is_active INTEGER NOT NULL DEFAULT 1,
    last_order_date TEXT,
    lifetime_value REAL NOT NULL DEFAULT 0,
    account_manager TEXT NOT NULL,
    manager_employee_id INTEGER
);

CREATE TABLE IF NOT EXISTS products (
    id INTEGER PRIMARY KEY,
    sku TEXT NOT NULL UNIQUE,
    name TEXT NOT NULL,
    category TEXT NOT NULL,
    stock INTEGER NOT NULL,
    reorder_level INTEGER NOT NULL,
    price REAL NOT NULL,
    cost REAL NOT NULL DEFAULT 0,
    discount_percent REAL NOT NULL DEFAULT 0,
    status TEXT NOT NULL,
    supplier TEXT NOT NULL,
    margin_percent REAL NOT NULL,
    expiration_date TEXT NOT NULL DEFAULT '2026-12-31',
    batch_code TEXT NOT NULL DEFAULT 'BATCH-DEMO'
);

CREATE TABLE IF NOT EXISTS employees (
    id INTEGER PRIMARY KEY,
    full_name TEXT NOT NULL,
    role TEXT NOT NULL,
    region TEXT NOT NULL,
    status TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS orders (
    id INTEGER PRIMARY KEY,
    order_code TEXT NOT NULL UNIQUE,
    client_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL,
    total_price REAL NOT NULL,
    order_date TEXT NOT NULL,
    status TEXT NOT NULL,
    priority TEXT NOT NULL,
    FOREIGN KEY (client_id) REFERENCES clients(id),
    FOREIGN KEY (product_id) REFERENCES products(id)
);

CREATE TABLE IF NOT EXISTS sales (
    id INTEGER PRIMARY KEY,
    sale_code TEXT NOT NULL UNIQUE,
    employee_id INTEGER NOT NULL,
    client_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    channel TEXT NOT NULL,
    quantity INTEGER NOT NULL DEFAULT 1,
    unit_price REAL NOT NULL DEFAULT 0,
    revenue REAL NOT NULL,
    total_amount REAL NOT NULL DEFAULT 0,
    margin_percent REAL NOT NULL,
    sale_date TEXT NOT NULL,
    status TEXT NOT NULL,
    payment_method TEXT NOT NULL DEFAULT 'Card',
    cancellation_reason TEXT,
    FOREIGN KEY (employee_id) REFERENCES employees(id),
    FOREIGN KEY (client_id) REFERENCES clients(id),
    FOREIGN KEY (product_id) REFERENCES products(id)
);

CREATE TABLE IF NOT EXISTS pipeline_deals (
    id INTEGER PRIMARY KEY,
    lead_name TEXT NOT NULL,
    client_id INTEGER,
    manager_id INTEGER,
    stage TEXT NOT NULL,
    value REAL NOT NULL DEFAULT 0,
    expected_close_date TEXT,
    status TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    FOREIGN KEY (client_id) REFERENCES clients(id),
    FOREIGN KEY (manager_id) REFERENCES employees(id)
);

CREATE TABLE IF NOT EXISTS tasks (
    id INTEGER PRIMARY KEY,
    title TEXT NOT NULL,
    assigned_to TEXT NOT NULL,
    due_date TEXT NOT NULL,
    status TEXT NOT NULL,
    priority TEXT NOT NULL,
    related_client_id INTEGER,
    created_at TEXT NOT NULL,
    FOREIGN KEY (related_client_id) REFERENCES clients(id)
);

CREATE TABLE IF NOT EXISTS audit_log (
    id INTEGER PRIMARY KEY,
    entity_type TEXT NOT NULL,
    entity_id INTEGER NOT NULL DEFAULT 0,
    action TEXT NOT NULL,
    user_name TEXT NOT NULL,
    details TEXT NOT NULL,
    created_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_orders_date ON orders(order_date);
CREATE INDEX IF NOT EXISTS idx_orders_status ON orders(status);
CREATE INDEX IF NOT EXISTS idx_sales_date ON sales(sale_date);
CREATE INDEX IF NOT EXISTS idx_sales_status ON sales(status);
CREATE INDEX IF NOT EXISTS idx_pipeline_stage ON pipeline_deals(stage);
CREATE INDEX IF NOT EXISTS idx_tasks_due ON tasks(due_date, status);
CREATE INDEX IF NOT EXISTS idx_audit_created ON audit_log(created_at);
)sql";

    if (!db_.Execute(kSchemaSql, &lastError_)) {
        return false;
    }
    if (!ColumnExists("products", "expiration_date")) {
        if (!db_.Execute("ALTER TABLE products ADD COLUMN expiration_date TEXT NOT NULL DEFAULT '2026-12-31';", &lastError_)) {
            return false;
        }
    }
    if (!ColumnExists("products", "batch_code")) {
        if (!db_.Execute("ALTER TABLE products ADD COLUMN batch_code TEXT NOT NULL DEFAULT 'BATCH-DEMO';", &lastError_)) {
            return false;
        }
    }
    if (!ColumnExists("products", "cost")) {
        if (!db_.Execute("ALTER TABLE products ADD COLUMN cost REAL NOT NULL DEFAULT 0;", &lastError_)) {
            return false;
        }
    }
    if (!ColumnExists("products", "discount_percent")) {
        if (!db_.Execute("ALTER TABLE products ADD COLUMN discount_percent REAL NOT NULL DEFAULT 0;", &lastError_)) {
            return false;
        }
    }
    if (!ColumnExists("clients", "manager_employee_id")) {
        if (!db_.Execute("ALTER TABLE clients ADD COLUMN manager_employee_id INTEGER;", &lastError_)) {
            return false;
        }
    }
    if (!ColumnExists("sales", "quantity")) {
        if (!db_.Execute("ALTER TABLE sales ADD COLUMN quantity INTEGER NOT NULL DEFAULT 1;", &lastError_)) {
            return false;
        }
    }
    if (!ColumnExists("sales", "unit_price")) {
        if (!db_.Execute("ALTER TABLE sales ADD COLUMN unit_price REAL NOT NULL DEFAULT 0;", &lastError_)) {
            return false;
        }
    }
    if (!ColumnExists("sales", "total_amount")) {
        if (!db_.Execute("ALTER TABLE sales ADD COLUMN total_amount REAL NOT NULL DEFAULT 0;", &lastError_)) {
            return false;
        }
    }
    if (!ColumnExists("sales", "payment_method")) {
        if (!db_.Execute("ALTER TABLE sales ADD COLUMN payment_method TEXT NOT NULL DEFAULT 'Card';", &lastError_)) {
            return false;
        }
    }
    if (!ColumnExists("sales", "cancellation_reason")) {
        if (!db_.Execute("ALTER TABLE sales ADD COLUMN cancellation_reason TEXT;", &lastError_)) {
            return false;
        }
    }
    return BackfillProductExpiryData() && BackfillExtendedData();
}

bool Repository::ColumnExists(const std::string& table, const std::string& column) {
    SqliteStatement statement;
    if (!db_.Prepare("PRAGMA table_info(" + table + ");", &statement, &lastError_)) {
        return false;
    }
    while (statement.Step() == SQLITE_ROW) {
        if (ToUtf8(statement.ColumnText(1)) == column) {
            return true;
        }
    }
    return false;
}

bool Repository::BackfillProductExpiryData() {
    for (const auto& product : kProducts) {
        std::ostringstream sql;
        sql << "UPDATE products SET "
            << "expiration_date = " << QuoteSql(product.expirationDate) << ", "
            << "batch_code = " << QuoteSql(product.batchCode)
            << " WHERE sku = " << QuoteSql(product.sku)
            << " AND (expiration_date = '2026-12-31' OR batch_code = 'BATCH-DEMO' OR expiration_date IS NULL OR batch_code IS NULL);";
        if (!db_.Execute(sql.str(), &lastError_)) {
            return false;
        }
    }
    return true;
}

bool Repository::BackfillExtendedData() {
    if (!db_.Execute(
            "UPDATE products "
            "SET cost = CASE WHEN cost <= 0 THEN ROUND(price * (1 - margin_percent / 100.0), 2) ELSE cost END, "
            "    discount_percent = CASE WHEN discount_percent < 0 THEN 0 ELSE discount_percent END;",
            &lastError_)) {
        return false;
    }
    if (!db_.Execute(
            "UPDATE sales "
            "SET quantity = CASE WHEN quantity <= 0 THEN 1 ELSE quantity END, "
            "    unit_price = CASE WHEN unit_price <= 0 THEN revenue / CASE WHEN quantity <= 0 THEN 1 ELSE quantity END ELSE unit_price END, "
            "    total_amount = CASE WHEN total_amount <= 0 THEN revenue ELSE total_amount END, "
            "    payment_method = CASE WHEN payment_method IS NULL OR payment_method = '' THEN 'Card' ELSE payment_method END;",
            &lastError_)) {
        return false;
    }
    if (!db_.Execute(
            "UPDATE clients "
            "SET manager_employee_id = (SELECT employees.id FROM employees WHERE employees.full_name = clients.account_manager LIMIT 1) "
            "WHERE manager_employee_id IS NULL;",
            &lastError_)) {
        return false;
    }
    return true;
}

bool Repository::SeedDemoData() {
    if (!db_.Execute(BuildSeedScript(), &lastError_)) {
        return false;
    }
    return true;
}

bool Repository::EnsureDefaultSettings() {
    std::ostringstream sql;
    sql << "INSERT OR IGNORE INTO app_settings(setting_key, setting_value) VALUES"
        << "(" << QuoteSql(L"theme") << "," << QuoteSql(L"Corporate Light") << "),"
        << "(" << QuoteSql(L"language") << "," << QuoteSql(L"English") << "),"
        << "(" << QuoteSql(L"date_format") << "," << QuoteSql(L"dd MMM yyyy") << "),"
        << "(" << QuoteSql(L"app_name") << "," << QuoteSql(L"SalesFlow") << "),"
        << "(" << QuoteSql(L"auto_refresh_minutes") << "," << QuoteSql(L"5") << "),"
        << "(" << QuoteSql(L"startup_page") << "," << QuoteSql(L"Dashboard") << "),"
        << "(" << QuoteSql(L"last_seeded_at") << "," << QuoteSql(CurrentTimestamp()) << ");";
    return db_.Execute(sql.str(), &lastError_);
}

bool Repository::EnsureDefaultRoles() {
    std::ostringstream sql;
    sql << "INSERT OR IGNORE INTO roles(id, name, description, default_start_page) VALUES"
        << "(1," << QuoteSql(L"System Administrator") << "," << QuoteSql(L"Users, roles, settings, and audit control") << "," << QuoteSql(L"Settings") << "),"
        << "(2," << QuoteSql(L"Director") << "," << QuoteSql(L"Executive monitoring, analytics, reports, and read-only operations") << "," << QuoteSql(L"Dashboard") << "),"
        << "(3," << QuoteSql(L"Head of Sales") << "," << QuoteSql(L"Team sales control, tasks, funnel, and operational reports") << "," << QuoteSql(L"Dashboard") << "),"
        << "(4," << QuoteSql(L"Sales Manager") << "," << QuoteSql(L"Own clients, own sales, tasks, and product catalog access") << "," << QuoteSql(L"Sales") << "),"
        << "(5," << QuoteSql(L"Product Manager") << "," << QuoteSql(L"Product catalog, prices, inventory, and catalog import/export") << "," << QuoteSql(L"Products") << ");";

    sql << "UPDATE users SET role = " << QuoteSql(L"System Administrator")
        << " WHERE role IN (" << QuoteSql(L"Administrator") << "," << QuoteSql(L"Admin") << ");";
    sql << "UPDATE users SET role = " << QuoteSql(L"Director")
        << " WHERE role IN (" << QuoteSql(L"Commercial Director") << "," << QuoteSql(L"Sales Analyst") << ");";
    sql << "UPDATE users SET role = " << QuoteSql(L"Head of Sales")
        << " WHERE role IN (" << QuoteSql(L"Supervisor") << "," << QuoteSql(L"Regional Sales Manager") << ");";
    sql << "UPDATE users SET role = " << QuoteSql(L"Sales Manager")
        << " WHERE role IN (" << QuoteSql(L"Manager") << "," << QuoteSql(L"Sales Operator") << "," << QuoteSql(L"Account Manager") << "," << QuoteSql(L"Key Account Manager") << ");";
    sql << "UPDATE users SET role = " << QuoteSql(L"Product Manager")
        << " WHERE role IN (" << QuoteSql(L"Procurement Controller") << "," << QuoteSql(L"Inventory Manager") << ");";

    const struct PermissionSeed {
        int roleId;
        const wchar_t* permission;
        const wchar_t* scope;
    } permissions[] = {
        {1, L"manage_users", L"all"}, {1, L"manage_roles", L"all"}, {1, L"manage_settings", L"all"}, {1, L"view_audit", L"all"},
        {2, L"view_dashboard", L"all"}, {2, L"view_analytics", L"all"}, {2, L"export_reports", L"all"}, {2, L"read_operational_data", L"all"},
        {3, L"view_team_sales", L"team"}, {3, L"change_sale_status", L"team"}, {3, L"manage_tasks", L"team"}, {3, L"export_operational_reports", L"team"},
        {4, L"create_sale", L"own"}, {4, L"change_sale_status", L"own"}, {4, L"create_client", L"own"}, {4, L"edit_client", L"own"}, {4, L"read_products", L"all"},
        {5, L"create_product", L"all"}, {5, L"edit_product", L"all"}, {5, L"delete_product", L"all"}, {5, L"import_export_catalog", L"all"}
    };
    for (const auto& permission : permissions) {
        sql << "INSERT OR IGNORE INTO role_permissions(role_id, permission, scope) VALUES("
            << permission.roleId << "," << QuoteSql(permission.permission) << "," << QuoteSql(permission.scope) << ");";
    }

    return db_.Execute(sql.str(), &lastError_);
}

bool Repository::EnsureDefaultAdminAccount() {
    if (db_.QueryScalarInt("SELECT COUNT(*) FROM users;", 0) > 0) {
        return true;
    }

    SqliteStatement insert;
    if (!db_.Prepare(
            "INSERT INTO users(id, full_name, username, email, role, password_hash, status, last_login, created_at) "
            "VALUES(1, ?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);",
            &insert,
            &lastError_)) {
        return false;
    }
    insert.BindText(1, L"SalesFlow Administrator");
    insert.BindText(2, L"admin");
    insert.BindText(3, L"admin@salesflow.local");
    insert.BindText(4, L"System Administrator");
    insert.BindText(5, DemoPasswordHash(L"demo123"));
    insert.BindText(6, L"Active");
    insert.BindText(7, L"Never");
    insert.BindText(8, CurrentDateIso());
    if (insert.Step() != SQLITE_DONE) {
        SetLastError(L"Unable to create default administrator account.");
        return false;
    }
    return true;
}

bool Repository::EnsureDemoAccountPasswords() {
    std::ostringstream sql;
    sql << "BEGIN IMMEDIATE;";
    for (const auto& user : kUsers) {
        sql << "INSERT OR IGNORE INTO users(id, full_name, username, email, role, password_hash, status, last_login, created_at) VALUES("
            << user.id << ","
            << QuoteSql(user.fullName) << ","
            << QuoteSql(user.username) << ","
            << QuoteSql(user.email) << ","
            << QuoteSql(user.role) << ","
            << QuoteSql(DemoPasswordHash(L"demo123")) << ","
            << QuoteSql(L"Active") << ","
            << QuoteSql(user.lastLogin) << ","
            << QuoteSql(user.createdAt) << ");";
        sql << "UPDATE users SET "
            << "full_name = " << QuoteSql(user.fullName) << ", "
            << "email = " << QuoteSql(user.email) << ", "
            << "role = " << QuoteSql(user.role) << ", "
            << "password_hash = " << QuoteSql(DemoPasswordHash(L"demo123")) << ", "
            << "status = " << QuoteSql(L"Active")
            << " WHERE lower(username) = lower(" << QuoteSql(user.username) << ");";
    }
    sql << "UPDATE users SET password_hash = " << QuoteSql(DemoPasswordHash(L"demo123"))
        << ", status = " << QuoteSql(L"Active")
        << " WHERE lower(username) IN ("
        << QuoteSql(L"admin") << ","
        << QuoteSql(L"a.petrova") << ","
        << QuoteSql(L"l.smirnov") << ","
        << QuoteSql(L"m.ivanova") << ","
        << QuoteSql(L"d.belov") << ","
        << QuoteSql(L"e.kozlova") << ","
        << QuoteSql(L"v.morozov") << ");";
    sql << "COMMIT;";
    return db_.Execute(sql.str(), &lastError_);
}

bool Repository::EnsureInitialized() {
    return initialized_ || Initialize();
}

bool Repository::RequirePermission(access::Permission permission, const std::wstring& actionName) {
    if (!hasCurrentAccount_) {
        SetLastError(L"Access denied: sign in before performing this action.");
        return false;
    }
    if (!access::HasPermission(currentAccount_, permission)) {
        SetLastError(actionName + L" is not allowed for role: " + currentAccount_.role);
        LogAudit(L"security", currentAccount_.id, L"Denied", actionName + L" denied for role " + currentAccount_.role);
        return false;
    }
    return true;
}

bool Repository::HasAnyPermission(access::Permission first, access::Permission second) const {
    return hasCurrentAccount_
        && (access::HasPermission(currentAccount_, first) || access::HasPermission(currentAccount_, second));
}

int Repository::EnsureEmployeeForCurrentAccount() {
    const std::wstring displayName = hasCurrentAccount_ && !currentAccount_.fullName.empty()
        ? currentAccount_.fullName
        : L"SalesFlow Manager";

    SqliteStatement lookup;
    if (db_.Prepare("SELECT id FROM employees WHERE lower(full_name) = lower(?1) LIMIT 1;", &lookup, nullptr)) {
        lookup.BindText(1, displayName);
        if (lookup.Step() == SQLITE_ROW) {
            return lookup.ColumnInt(0);
        }
    }

    int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM employees;", 1);
    SqliteStatement insert;
    if (!db_.Prepare("INSERT INTO employees(id, full_name, role, region, status) VALUES(?1, ?2, ?3, ?4, ?5);", &insert, &lastError_)) {
        return db_.QueryScalarInt("SELECT id FROM employees ORDER BY id ASC LIMIT 1;", 1);
    }
    insert.BindInt(1, nextId);
    insert.BindText(2, displayName);
    insert.BindText(3, hasCurrentAccount_ ? access::CanonicalRoleName(access::RoleForAccount(currentAccount_)) : L"Sales Manager");
    insert.BindText(4, L"Assigned");
    insert.BindText(5, L"On Track");
    if (insert.Step() != SQLITE_DONE) {
        SetLastError(L"Unable to create an employee profile for the signed-in account.");
        return db_.QueryScalarInt("SELECT id FROM employees ORDER BY id ASC LIMIT 1;", 1);
    }
    return nextId;
}

bool Repository::UpdateSetting(const std::wstring& key, const std::wstring& value) {
    std::ostringstream sql;
    sql << "INSERT INTO app_settings(setting_key, setting_value) VALUES("
        << QuoteSql(key) << ","
        << QuoteSql(value) << ") "
        << "ON CONFLICT(setting_key) DO UPDATE SET setting_value = excluded.setting_value;";
    return db_.Execute(sql.str(), &lastError_);
}

std::wstring Repository::ReadSetting(const std::wstring& key, const std::wstring& fallback) {
    if (!EnsureInitialized()) {
        return fallback;
    }

    SqliteStatement statement;
    if (!db_.Prepare("SELECT setting_value FROM app_settings WHERE setting_key = ?1;", &statement, &lastError_)) {
        return fallback;
    }
    statement.BindText(1, key);
    return statement.Step() == SQLITE_ROW ? statement.ColumnText(0) : fallback;
}

std::wstring Repository::EnsureExportsDirectory() const {
    std::error_code ec;
    const std::filesystem::path exportsPath = std::filesystem::path(ExecutableDirectory()) / L"exports";
    std::filesystem::create_directories(exportsPath, ec);
    return ec ? L"" : exportsPath.wstring();
}

std::wstring Repository::DatabasePath() const {
    return (std::filesystem::path(ExecutableDirectory()) / L"data" / L"sales_monitoring.db").wstring();
}

bool Repository::LogAudit(const std::wstring& entityType, int entityId, const std::wstring& action, const std::wstring& details) {
    if (!db_.IsOpen()) {
        return false;
    }

    const int nextId = db_.QueryScalarInt("SELECT COALESCE(MAX(id), 0) + 1 FROM audit_log;", 1);
    const std::wstring userName = hasCurrentAccount_
        ? (currentAccount_.username.empty() ? currentAccount_.fullName : currentAccount_.username)
        : L"system";

    SqliteStatement insert;
    if (!db_.Prepare(
            "INSERT INTO audit_log(id, entity_type, entity_id, action, user_name, details, created_at) "
            "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7);",
            &insert,
            nullptr)) {
        return false;
    }
    insert.BindInt(1, nextId);
    insert.BindText(2, entityType);
    insert.BindInt(3, entityId);
    insert.BindText(4, action);
    insert.BindText(5, userName);
    insert.BindText(6, details);
    insert.BindText(7, CurrentTimestamp());
    return insert.Step() == SQLITE_DONE;
}

void Repository::SetLastError(const std::wstring& error) {
    lastError_ = error;
}

} // namespace data
