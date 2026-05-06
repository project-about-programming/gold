#include "DashboardPage.h"

#include "AccessControl.h"
#include "DataRepository.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>
#include <cwctype>
#include <initializer_list>

namespace {
enum : int {
    IDC_PERIOD_TODAY = 2101,
    IDC_PERIOD_WEEK,
    IDC_PERIOD_MONTH,
    IDC_PERIOD_YEAR
};

constexpr COLORREF kDashboardBorder = RGB(221, 230, 240);
constexpr COLORREF kDashboardGrid = RGB(229, 234, 241);
constexpr COLORREF kDashboardMutedText = RGB(100, 116, 139);
constexpr COLORREF kDashboardDarkText = RGB(15, 23, 42);

COLORREF KpiIconFill(COLORREF accent) {
    if (accent == RGB(5, 150, 105)) {
        return RGB(236, 253, 245);
    }
    if (accent == RGB(217, 119, 6)) {
        return RGB(255, 247, 237);
    }
    return RGB(239, 246, 255);
}

COLORREF KpiIconBorder(COLORREF accent) {
    if (accent == RGB(5, 150, 105)) {
        return RGB(167, 243, 208);
    }
    if (accent == RGB(217, 119, 6)) {
        return RGB(254, 215, 170);
    }
    return RGB(219, 234, 254);
}

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

struct DashboardLayout {
    RECT periodRow{};
    RECT periodButtons[4]{};
    RECT kpis[4]{};
    RECT revenueChart{};
    RECT volumeChart{};
    RECT topProducts{};
    RECT recentOrders{};
    RECT lowStock{};
};

struct SystemDashboardLayout {
    RECT kpis[6]{};
    RECT auditEvents{};
    RECT alerts{};
    RECT accessOverview{};
    RECT databaseStatus{};
};

DashboardLayout BuildLayout(const RECT& client) {
    DashboardLayout layout{};
    const int padding = ui::Scale(24);
    const int gap = ui::Scale(18);
    RECT area = ui::Inset(client, padding, padding);
    layout.periodRow = ui::TakeTop(&area, ui::Scale(40), gap);
    const int periodWidth = ui::Scale(82);
    const int periodGap = ui::Scale(8);
    int periodLeft = layout.periodRow.left;
    for (int i = 0; i < 4; ++i) {
        layout.periodButtons[i] = RECT{ periodLeft, layout.periodRow.top, periodLeft + periodWidth, layout.periodRow.top + ui::Scale(36) };
        periodLeft += periodWidth + periodGap;
    }

    RECT kpiRow = ui::TakeTop(&area, ui::Scale(132), gap);
    const auto kpiColumns = ui::SplitColumns(kpiRow, 4, ui::Scale(16));
    for (int index = 0; index < 4 && index < static_cast<int>(kpiColumns.size()); ++index) {
        layout.kpis[index] = kpiColumns[index];
    }

    const int remainingHeight = ui::Height(area);
    const int bottomReserve = std::min(ui::Scale(300), std::max(ui::Scale(150), remainingHeight / 2));
    int chartHeight = std::clamp(remainingHeight - bottomReserve - gap, ui::Scale(180), ui::Scale(260));
    if (chartHeight + gap + bottomReserve > remainingHeight) {
        chartHeight = std::max(0, remainingHeight - gap - bottomReserve);
    }
    RECT chartsRow = ui::TakeTop(&area, chartHeight, gap);
    const auto chartColumns = ui::SplitColumns(chartsRow, 2, ui::Scale(16));
    layout.revenueChart = chartColumns[0];
    layout.volumeChart = chartColumns[1];

    const auto bottomColumns = ui::SplitColumns(area, 2, ui::Scale(16));
    layout.topProducts = bottomColumns[0];
    RECT rightColumn = bottomColumns[1];
    const int minPanelHeight = ui::Scale(86);
    int recentHeight = std::max(minPanelHeight, (ui::Height(rightColumn) - gap) / 2);
    if (ui::Height(rightColumn) > gap + minPanelHeight) {
        recentHeight = std::min(recentHeight, ui::Height(rightColumn) - gap - minPanelHeight);
    }
    layout.recentOrders = ui::TakeTop(&rightColumn, recentHeight, gap);
    layout.lowStock = rightColumn;
    return layout;
}

SystemDashboardLayout BuildSystemLayout(const RECT& client) {
    SystemDashboardLayout layout{};
    const int padding = ui::Scale(24);
    const int gap = ui::Scale(18);
    RECT area = ui::Inset(client, padding, padding);

    RECT firstKpiRow = ui::TakeTop(&area, ui::Scale(126), ui::Scale(14));
    const auto firstKpis = ui::SplitColumns(firstKpiRow, 3, ui::Scale(16));
    RECT secondKpiRow = ui::TakeTop(&area, ui::Scale(126), gap);
    const auto secondKpis = ui::SplitColumns(secondKpiRow, 3, ui::Scale(16));
    for (int index = 0; index < 3; ++index) {
        layout.kpis[index] = firstKpis[index];
        layout.kpis[index + 3] = secondKpis[index];
    }

    RECT topPanels = ui::TakeTop(&area, std::clamp(ui::Height(area) / 2, ui::Scale(190), ui::Scale(260)), gap);
    const auto top = ui::SplitColumns(topPanels, 2, ui::Scale(16));
    layout.auditEvents = top[0];
    layout.alerts = top[1];

    const auto bottom = ui::SplitColumns(area, 2, ui::Scale(16));
    layout.accessOverview = bottom[0];
    layout.databaseStatus = bottom[1];
    return layout;
}

double MinValue(const std::vector<ui::SeriesPoint>& points, double fallback) {
    if (points.empty()) {
        return fallback;
    }
    double result = points.front().value;
    for (const auto& point : points) {
        result = std::min(result, point.value);
    }
    return result * 0.9;
}

double MaxValue(const std::vector<ui::SeriesPoint>& points, double fallback) {
    if (points.empty()) {
        return fallback;
    }
    double result = points.front().value;
    for (const auto& point : points) {
        result = std::max(result, point.value);
    }
    return result * 1.08;
}

bool HasUsefulSeries(const std::vector<ui::SeriesPoint>& points) {
    return std::any_of(points.begin(), points.end(), [](const ui::SeriesPoint& point) {
        return point.value > 0.01;
    });
}

bool LooksZero(const std::wstring& value) {
    for (wchar_t ch : value) {
        if (std::iswdigit(ch) && ch != L'0') {
            return false;
        }
    }
    return true;
}

std::wstring CaptionForMetric(const std::wstring& value, const std::wstring& nonZero, const std::wstring& zero) {
    return LooksZero(value) ? zero : nonZero;
}

RECT ListBounds(const RECT& panel) {
    return ui::Inset(ui::MakeRect(panel.left, panel.top + ui::Scale(58), panel.right, panel.bottom), ui::Scale(12), ui::Scale(12));
}

void ResetListColumns(HWND list, const std::vector<std::wstring>& titles, const std::vector<int>& widths) {
    while (Header_GetItemCount(ListView_GetHeader(list)) > 0) {
        ListView_DeleteColumn(list, 0);
    }
    ui::AddListColumns(list, titles, widths);
    ui::ApplyStandardTableStyle(list);
}

void DrawDashboardPanel(HDC hdc, const RECT& rc, const std::wstring& title, const std::wstring& subtitle) {
    ui::DrawRoundedPanel(hdc, rc, RGB(255, 255, 255), kDashboardBorder, ui::Scale(10), false);
    ui::DrawTextLine(hdc, title, ui::MakeRect(rc.left + ui::Scale(18), rc.top + ui::Scale(14), rc.right - ui::Scale(18), rc.top + ui::Scale(38)),
        theme::BodyBoldFont(), kDashboardDarkText, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    if (!subtitle.empty()) {
        ui::DrawTextLine(hdc, subtitle, ui::MakeRect(rc.left + ui::Scale(18), rc.top + ui::Scale(36), rc.right - ui::Scale(18), rc.top + ui::Scale(56)),
            theme::SmallFont(), kDashboardMutedText, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    }
}

void DrawEmptyState(HDC hdc, const RECT& rc, const std::wstring& icon, const std::wstring& title, const std::wstring& subtitle) {
    const int boxWidth = std::min(ui::Scale(360), std::max(ui::Scale(180), ui::Width(rc) - ui::Scale(36)));
    const int boxHeight = ui::Scale(116);
    RECT box{ rc.left + (ui::Width(rc) - boxWidth) / 2, rc.top + (ui::Height(rc) - boxHeight) / 2,
        rc.left + (ui::Width(rc) + boxWidth) / 2, rc.top + (ui::Height(rc) + boxHeight) / 2 };
    ui::DrawTextLine(hdc, icon, ui::MakeRect(box.left, box.top, box.right, box.top + ui::Scale(32)),
        theme::BodyBoldFont(), RGB(148, 163, 184), DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    ui::DrawTextLine(hdc, title, ui::MakeRect(box.left, box.top + ui::Scale(42), box.right, box.top + ui::Scale(68)),
        theme::BodyBoldFont(), RGB(51, 65, 85), DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    ui::DrawTextLine(hdc, subtitle, ui::MakeRect(box.left, box.top + ui::Scale(72), box.right, box.bottom),
        theme::SmallFont(), RGB(100, 116, 139), DT_CENTER | DT_WORDBREAK | DT_END_ELLIPSIS);
}

int DashboardTableWidth(const RECT& listBounds) {
    return std::max(ui::Scale(220), ui::Width(listBounds) - GetSystemMetrics(SM_CXVSCROLL) - ui::Scale(26));
}

void SetWeightedColumns(HWND list, int width, std::initializer_list<int> weights) {
    int total = 0;
    for (int weight : weights) {
        total += weight;
    }
    int used = 0;
    int index = 0;
    const int count = static_cast<int>(weights.size());
    for (int weight : weights) {
        const int columnWidth = (index == count - 1)
            ? std::max(0, width - used)
            : std::max(0, width * weight / std::max(1, total));
        ListView_SetColumnWidth(list, index, columnWidth);
        used += columnWidth;
        ++index;
    }
    ShowScrollBar(list, SB_HORZ, FALSE);
}

void DrawDashboardKpiCard(HDC hdc, const RECT& rc, const std::wstring& title, const std::wstring& value,
    const std::wstring& caption, const std::wstring& trend, const std::wstring& icon, COLORREF accent) {
    ui::DrawRoundedPanel(hdc, rc, RGB(255, 255, 255), kDashboardBorder, ui::Scale(10), false);
    RECT accentBar{ rc.left + ui::Scale(16), rc.top + ui::Scale(14), rc.left + ui::Scale(82), rc.top + ui::Scale(18) };
    ui::FillRectColor(hdc, accentBar, accent);
    RECT iconBox{ rc.right - ui::Scale(52), rc.top + ui::Scale(18), rc.right - ui::Scale(20), rc.top + ui::Scale(50) };
    ui::DrawRoundedPanel(hdc, iconBox, KpiIconFill(accent), KpiIconBorder(accent), ui::Scale(8), false);
    ui::DrawTextLine(hdc, icon, iconBox, theme::BodyBoldFont(), accent, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    const int left = rc.left + ui::Scale(16);
    const int right = rc.right - ui::Scale(18);
    ui::DrawTextLine(hdc, title, ui::MakeRect(left, rc.top + ui::Scale(30), rc.right - ui::Scale(68), rc.top + ui::Scale(50)),
        theme::SmallBoldFont(), kDashboardMutedText, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    ui::DrawTextLine(hdc, value, ui::MakeRect(left, rc.top + ui::Scale(54), right, rc.top + ui::Scale(84)),
        theme::KpiValueFont(), kDashboardDarkText, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    ui::DrawTextLine(hdc, caption, ui::MakeRect(left, rc.top + ui::Scale(89), right, rc.top + ui::Scale(108)),
        theme::SmallFont(), kDashboardMutedText, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    if (!trend.empty()) {
        ui::DrawTextLine(hdc, trend, ui::MakeRect(left, rc.top + ui::Scale(111), right, rc.top + ui::Scale(129)),
            theme::SmallBoldFont(), accent, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    }
}

void DrawDashboardLineChart(HDC hdc, const RECT& rc, const std::vector<ui::SeriesPoint>& points) {
    DrawDashboardPanel(hdc, rc, UiText(L"Rolling Revenue Trend", L"Динамика выручки"),
        UiText(L"Monthly revenue based on sales records.", L"Помесячная выручка по данным продаж."));
    RECT plot{ rc.left + ui::Scale(36), rc.top + ui::Scale(74), rc.right - ui::Scale(24), rc.bottom - ui::Scale(34) };
    if (!HasUsefulSeries(points) || ui::Height(plot) < ui::Scale(70)) {
        DrawEmptyState(hdc, ui::MakeRect(rc.left + ui::Scale(18), rc.top + ui::Scale(58), rc.right - ui::Scale(18), rc.bottom - ui::Scale(18)),
            L"↗", UiText(L"No revenue data yet", L"Данных о выручке пока нет"),
            UiText(L"Create sales orders to see revenue trends here.", L"Создайте продажи или заказы, чтобы увидеть динамику."));
        return;
    }

    const double minValue = MinValue(points, 0.0);
    const double maxValue = MaxValue(points, 1.0);
    HPEN gridPen = CreatePen(PS_SOLID, 1, kDashboardGrid);
    HGDIOBJ oldPen = SelectObject(hdc, gridPen);
    for (int i = 0; i < 4; ++i) {
        const int y = plot.top + (ui::Height(plot) * i / 3);
        MoveToEx(hdc, plot.left, y, nullptr);
        LineTo(hdc, plot.right, y);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);

    HPEN linePen = CreatePen(PS_SOLID, ui::Scale(3), RGB(37, 99, 235));
    oldPen = SelectObject(hdc, linePen);
    for (size_t i = 0; i < points.size(); ++i) {
        const double norm = std::clamp((points[i].value - minValue) / std::max(1.0, maxValue - minValue), 0.0, 1.0);
        const int x = plot.left + static_cast<int>((ui::Width(plot) - ui::Scale(18)) * i / std::max<size_t>(1, points.size() - 1)) + ui::Scale(9);
        const int y = plot.bottom - static_cast<int>((ui::Height(plot) - ui::Scale(16)) * norm) - ui::Scale(8);
        if (i == 0) {
            MoveToEx(hdc, x, y, nullptr);
        } else {
            LineTo(hdc, x, y);
        }
        RECT label{ x - ui::Scale(20), plot.bottom + ui::Scale(8), x + ui::Scale(20), plot.bottom + ui::Scale(26) };
        ui::DrawTextLine(hdc, points[i].label, label, theme::SmallFont(), kDashboardMutedText, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(linePen);
}

void DrawDashboardBarChart(HDC hdc, const RECT& rc, const std::vector<ui::SeriesPoint>& values) {
    DrawDashboardPanel(hdc, rc, UiText(L"Monthly Order Volume", L"Объём заказов"),
        UiText(L"Orders grouped by reporting month.", L"Заказы по отчётным месяцам."));
    RECT plot{ rc.left + ui::Scale(32), rc.top + ui::Scale(74), rc.right - ui::Scale(24), rc.bottom - ui::Scale(34) };
    if (!HasUsefulSeries(values) || ui::Height(plot) < ui::Scale(70)) {
        DrawEmptyState(hdc, ui::MakeRect(rc.left + ui::Scale(18), rc.top + ui::Scale(58), rc.right - ui::Scale(18), rc.bottom - ui::Scale(18)),
            L"▥", UiText(L"No order volume data yet", L"Данных по заказам пока нет"),
            UiText(L"Orders will appear here after sales are recorded.", L"Заказы появятся здесь после добавления продаж."));
        return;
    }

    double maxValue = 1.0;
    for (const auto& value : values) {
        maxValue = std::max(maxValue, value.value);
    }
    HPEN gridPen = CreatePen(PS_SOLID, 1, kDashboardGrid);
    HGDIOBJ oldPen = SelectObject(hdc, gridPen);
    for (int i = 0; i < 4; ++i) {
        const int y = plot.top + (ui::Height(plot) * i / 3);
        MoveToEx(hdc, plot.left, y, nullptr);
        LineTo(hdc, plot.right, y);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);

    const int slot = std::max(ui::Scale(12), ui::Width(plot) / static_cast<int>(values.size()));
    for (size_t i = 0; i < values.size(); ++i) {
        const int x1 = plot.left + static_cast<int>(i) * slot + ui::Scale(8);
        const int x2 = std::min(static_cast<int>(plot.right), x1 + std::max(ui::Scale(8), slot - ui::Scale(16)));
        const int barHeight = static_cast<int>((ui::Height(plot) - ui::Scale(24)) * (values[i].value / maxValue));
        RECT bar{ x1, plot.bottom - barHeight - ui::Scale(8), x2, plot.bottom - ui::Scale(8) };
        ui::FillRectColor(hdc, bar, RGB(5, 150, 105));
        RECT label{ x1 - ui::Scale(8), plot.bottom + ui::Scale(6), x2 + ui::Scale(8), plot.bottom + ui::Scale(24) };
        ui::DrawTextLine(hdc, values[i].label, label, theme::SmallFont(), kDashboardMutedText, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }
}

} // namespace

DashboardPage::DashboardPage()
    : PageBase(L"Executive Dashboard", L"Revenue indicators, operational watchlists, and the latest commercial activity.") {}

const wchar_t* DashboardPage::ClassName() const { return L"NativeDashboardPage"; }

void DashboardPage::OnCreate() {
    periodButtons_[0] = ui::CreateUiButton(hwnd_, IDC_PERIOD_TODAY, L"Today", ui::ButtonKind::Secondary);
    periodButtons_[1] = ui::CreateUiButton(hwnd_, IDC_PERIOD_WEEK, L"Week", ui::ButtonKind::Secondary);
    periodButtons_[2] = ui::CreateUiButton(hwnd_, IDC_PERIOD_MONTH, L"Month", ui::ButtonKind::Primary);
    periodButtons_[3] = ui::CreateUiButton(hwnd_, IDC_PERIOD_YEAR, L"Year", ui::ButtonKind::Secondary);

    topProducts_ = ui::CreateUiListView(hwnd_, 2001);
    recentOrders_ = ui::CreateUiListView(hwnd_, 2002);
    lowStock_ = ui::CreateUiListView(hwnd_, 2003);

    ui::AddListColumns(topProducts_, {UiText(L"Product", L"Товар"), UiText(L"Category", L"Категория"), UiText(L"Units", L"Ед."), UiText(L"Revenue", L"Выручка"), UiText(L"Margin", L"Маржа")}, {220, 130, 90, 120, 90});
    ui::AddListColumns(recentOrders_, {UiText(L"Order ID", L"Заказ"), UiText(L"Client", L"Клиент"), UiText(L"Date", L"Дата"), UiText(L"Total", L"Итого"), UiText(L"Status", L"Статус")}, {120, 220, 120, 110, 110});
    ui::AddListColumns(lowStock_, {UiText(L"Alert", L"Alert"), UiText(L"Item", L"Item"), UiText(L"Owner", L"Owner"), UiText(L"Due / Value", L"Due / Value"), UiText(L"Status", L"Status")}, {100, 240, 180, 130, 110});

    LoadSnapshot();
    UpdatePeriodButtons();
}

void DashboardPage::OnActivate() {
    LoadSnapshot();
}

bool DashboardPage::IsSystemDashboard() const {
    if (!data::Repository::Instance().HasCurrentAccount()) {
        return false;
    }
    const auto account = data::Repository::Instance().CurrentAccount();
    const auto role = access::RoleForAccount(account);
    return role == access::AppRole::SystemAdministrator
        && !access::HasPermission(account, access::Permission::SalesView)
        && !access::HasPermission(account, access::Permission::OrdersView)
        && !access::HasPermission(account, access::Permission::InventoryView)
        && !access::HasPermission(account, access::Permission::CustomersView)
        && !access::HasPermission(account, access::Permission::AnalyticsView);
}

void DashboardPage::LoadSnapshot() {
    const bool newSystemMode = IsSystemDashboard();
    if (newSystemMode != systemDashboard_) {
        systemDashboard_ = newSystemMode;
        if (systemDashboard_) {
            ResetListColumns(topProducts_,
                {UiText(L"Date / Time", L"Date / Time"), UiText(L"User", L"User"), UiText(L"Action", L"Action"), UiText(L"Severity", L"Severity")},
                {160, 150, 180, 110});
            ResetListColumns(recentOrders_,
                {UiText(L"Alert", L"Alert"), UiText(L"Owner", L"Owner"), UiText(L"Due", L"Due"), UiText(L"Priority", L"Priority")},
                {220, 180, 130, 110});
            ResetListColumns(lowStock_,
                {UiText(L"Role", L"Role"), UiText(L"Permissions", L"Permissions"), UiText(L"Start Page", L"Start Page")},
                {220, 120, 150});
        } else {
            ResetListColumns(topProducts_, {UiText(L"Product", L"Product"), UiText(L"Category", L"Category"), UiText(L"Units", L"Units"), UiText(L"Revenue", L"Revenue"), UiText(L"Margin", L"Margin")}, {220, 130, 90, 120, 90});
            ResetListColumns(recentOrders_, {UiText(L"Order ID", L"Order ID"), UiText(L"Client", L"Client"), UiText(L"Date", L"Date"), UiText(L"Total", L"Total"), UiText(L"Status", L"Status")}, {120, 220, 120, 110, 110});
            ResetListColumns(lowStock_, {UiText(L"Alert", L"Alert"), UiText(L"Item", L"Item"), UiText(L"Owner", L"Owner"), UiText(L"Due / Value", L"Due / Value"), UiText(L"Status", L"Status")}, {100, 240, 180, 130, 110});
        }
    }

    if (systemDashboard_) {
        database_ = data::Repository::Instance().LoadDatabaseOverview();
        accounts_ = data::Repository::Instance().LoadAccounts();
        roles_ = data::Repository::Instance().LoadRoleSummaries();
        auditEvents_ = data::Repository::Instance().LoadAuditLog(8);
        systemAlerts_ = data::Repository::Instance().LoadOpenTasks();
        snapshot_ = {};
    } else {
        snapshot_ = data::Repository::Instance().LoadDashboardSnapshot();
        database_ = {};
        accounts_.clear();
        roles_.clear();
        auditEvents_.clear();
        systemAlerts_.clear();
    }
    FillTables();
}

void DashboardPage::FillTables() {
    if (systemDashboard_) {
        ui::ClearListRows(topProducts_);
        for (size_t index = 0; index < auditEvents_.size(); ++index) {
            const auto& row = auditEvents_[index];
            ui::AddListRow(topProducts_, static_cast<int>(index), {
                row.createdAt,
                row.userName,
                row.action,
                row.severity.empty() ? L"Info" : row.severity
            });
        }

        ui::ClearListRows(recentOrders_);
        for (size_t index = 0; index < systemAlerts_.size(); ++index) {
            const auto& row = systemAlerts_[index];
            ui::AddListRow(recentOrders_, static_cast<int>(index), {
                row.title,
                row.assignedTo,
                row.dueDateDisplay,
                row.priority
            });
        }

        ui::ClearListRows(lowStock_);
        for (size_t index = 0; index < roles_.size(); ++index) {
            const auto& row = roles_[index];
            ui::AddListRow(lowStock_, static_cast<int>(index), {
                row.name,
                std::to_wstring(row.permissionCount),
                row.defaultStartPage
            });
        }
        ShowWindow(topProducts_, auditEvents_.empty() ? SW_HIDE : SW_SHOW);
        ShowWindow(recentOrders_, systemAlerts_.empty() ? SW_HIDE : SW_SHOW);
        ShowWindow(lowStock_, roles_.empty() ? SW_HIDE : SW_SHOW);
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }

    ui::ClearListRows(topProducts_);
    for (size_t index = 0; index < snapshot_.topProducts.size(); ++index) {
        ui::AddListRow(topProducts_, static_cast<int>(index), snapshot_.topProducts[index]);
    }

    ui::ClearListRows(recentOrders_);
    for (size_t index = 0; index < snapshot_.recentOrders.size(); ++index) {
        auto row = snapshot_.recentOrders[index];
        if (row.size() >= 5) {
            row[4] = i18n::DataText(row[4]);
        }
        ui::AddListRow(recentOrders_, static_cast<int>(index), row);
    }

    ui::ClearListRows(lowStock_);
    for (size_t index = 0; index < snapshot_.lowStock.size(); ++index) {
        auto row = snapshot_.lowStock[index];
        if (row.size() >= 5) {
            row[4] = i18n::DataText(row[4]);
        }
        ui::AddListRow(lowStock_, static_cast<int>(index), row);
    }
    ShowWindow(topProducts_, snapshot_.topProducts.empty() ? SW_HIDE : SW_SHOW);
    ShowWindow(recentOrders_, snapshot_.recentOrders.empty() ? SW_HIDE : SW_SHOW);
    ShowWindow(lowStock_, snapshot_.lowStock.empty() ? SW_HIDE : SW_SHOW);
}

void DashboardPage::OnSize(int width, int height) {
    if (systemDashboard_) {
        const SystemDashboardLayout layout = BuildSystemLayout(ui::MakeRect(0, 0, width, height));
        for (HWND button : periodButtons_) {
            ShowWindow(button, SW_HIDE);
        }

        RECT list = ListBounds(layout.auditEvents);
        ui::MoveWindowToRect(topProducts_, list);
        SetWeightedColumns(topProducts_, DashboardTableWidth(list), {26, 24, 30, 20});

        list = ListBounds(layout.alerts);
        ui::MoveWindowToRect(recentOrders_, list);
        SetWeightedColumns(recentOrders_, DashboardTableWidth(list), {38, 28, 18, 16});

        list = ListBounds(layout.accessOverview);
        ui::MoveWindowToRect(lowStock_, list);
        SetWeightedColumns(lowStock_, DashboardTableWidth(list), {50, 20, 30});
        return;
    }

    const DashboardLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    for (int i = 0; i < 4; ++i) {
        ShowWindow(periodButtons_[i], SW_SHOW);
        ui::MoveWindowToRect(periodButtons_[i], layout.periodButtons[i]);
    }
    RECT list = ListBounds(layout.topProducts);
    ui::MoveWindowToRect(topProducts_, list);
    SetWeightedColumns(topProducts_, DashboardTableWidth(list), {32, 20, 12, 22, 14});

    list = ListBounds(layout.recentOrders);
    ui::MoveWindowToRect(recentOrders_, list);
    SetWeightedColumns(recentOrders_, DashboardTableWidth(list), {18, 30, 18, 16, 18});

    list = ListBounds(layout.lowStock);
    ui::MoveWindowToRect(lowStock_, list);
    SetWeightedColumns(lowStock_, DashboardTableWidth(list), {12, 30, 20, 24, 14});
}

void DashboardPage::OnPaint(HDC hdc, const RECT& client) {
    if (systemDashboard_) {
        const SystemDashboardLayout layout = BuildSystemLayout(client);
        ui::FillRectColor(hdc, client, theme::kWindowBackground);

        const int activeUsers = static_cast<int>(std::count_if(accounts_.begin(), accounts_.end(), [](const data::AccountRecord& account) {
            return account.status == L"Active";
        }));
        const int failedLogins = static_cast<int>(std::count_if(auditEvents_.begin(), auditEvents_.end(), [](const data::AuditRecord& row) {
            return row.action == L"LoginFailed" || row.action == L"LoginBlocked";
        }));

        DrawDashboardKpiCard(hdc, layout.kpis[0], UiText(L"Database Status", L"Database Status"),
            database_.connected ? L"Online" : L"Offline",
            UiText(L"Database connection active", L"Database connection active"),
            database_.engineVersion.empty() ? L"SQLite workspace" : database_.engineVersion,
            L"DB", database_.connected ? RGB(5, 150, 105) : RGB(220, 38, 38));
        DrawDashboardKpiCard(hdc, layout.kpis[1], UiText(L"User Accounts", L"User Accounts"),
            std::to_wstring(static_cast<int>(accounts_.size())),
            UiText(L"Registered user accounts", L"Registered user accounts"),
            std::to_wstring(activeUsers) + L" active",
            L"U", RGB(37, 99, 235));
        DrawDashboardKpiCard(hdc, layout.kpis[2], UiText(L"Active Roles", L"Active Roles"),
            std::to_wstring(static_cast<int>(roles_.size())),
            UiText(L"Role configuration status", L"Role configuration status"),
            UiText(L"Permission matrix loaded", L"Permission matrix loaded"),
            L"R", RGB(124, 58, 237));
        DrawDashboardKpiCard(hdc, layout.kpis[3], UiText(L"Audit Events", L"Audit Events"),
            std::to_wstring(static_cast<int>(auditEvents_.size())),
            UiText(L"Recent audit activity", L"Recent audit activity"),
            UiText(L"Latest security entries", L"Latest security entries"),
            L"A", RGB(37, 99, 235));
        DrawDashboardKpiCard(hdc, layout.kpis[4], UiText(L"Failed Logins", L"Failed Logins"),
            std::to_wstring(failedLogins),
            failedLogins == 0 ? UiText(L"No failed sign-ins in latest events", L"No failed sign-ins in latest events")
                              : UiText(L"Review recent access warnings", L"Review recent access warnings"),
            UiText(L"Access monitoring", L"Access monitoring"),
            L"!", failedLogins == 0 ? RGB(5, 150, 105) : RGB(217, 119, 6));
        DrawDashboardKpiCard(hdc, layout.kpis[5], UiText(L"System Alerts", L"System Alerts"),
            std::to_wstring(static_cast<int>(systemAlerts_.size())),
            systemAlerts_.empty() ? UiText(L"No alerts requiring attention", L"No alerts requiring attention")
                                  : UiText(L"Alerts requiring attention", L"Alerts requiring attention"),
            UiText(L"Task and access watchlist", L"Task and access watchlist"),
            L"⚑", systemAlerts_.empty() ? RGB(5, 150, 105) : RGB(217, 119, 6));

        DrawDashboardPanel(hdc, layout.auditEvents, UiText(L"Recent Audit Events", L"Recent Audit Events"),
            UiText(L"Latest sign-ins, denied actions and system changes.", L"Latest sign-ins, denied actions and system changes."));
        if (auditEvents_.empty()) {
            DrawEmptyState(hdc, ListBounds(layout.auditEvents), L"◎", UiText(L"No audit events yet", L"No audit events yet"),
                UiText(L"Security and system actions will appear here.", L"Security and system actions will appear here."));
        }

        DrawDashboardPanel(hdc, layout.alerts, UiText(L"System Alerts", L"System Alerts"),
            UiText(L"Open tasks and warnings that need administrator attention.", L"Open tasks and warnings that need administrator attention."));
        if (systemAlerts_.empty()) {
            DrawEmptyState(hdc, ListBounds(layout.alerts), L"✓", UiText(L"No system alerts", L"No system alerts"),
                UiText(L"Everything is running normally.", L"Everything is running normally."));
        }

        DrawDashboardPanel(hdc, layout.accessOverview, UiText(L"User Access Overview", L"User Access Overview"),
            UiText(L"Roles and permission counts currently configured.", L"Roles and permission counts currently configured."));
        if (roles_.empty()) {
            DrawEmptyState(hdc, ListBounds(layout.accessOverview), L"◉", UiText(L"No roles configured", L"No roles configured"),
                UiText(L"Roles are initialized automatically when the database starts.", L"Roles are initialized automatically when the database starts."));
        }

        DrawDashboardPanel(hdc, layout.databaseStatus, UiText(L"Database Workspace / Backup Status", L"Database Workspace / Backup Status"),
            UiText(L"SQLite workspace, backup readiness and dataset health.", L"SQLite workspace, backup readiness and dataset health."));
        const RECT dbText = ListBounds(layout.databaseStatus);
        const std::vector<std::wstring> lines = {
            L"Engine: " + (database_.engineVersion.empty() ? L"SQLite" : database_.engineVersion),
            L"Users: " + std::to_wstring(database_.usersCount) + L"   Customers: " + std::to_wstring(database_.clientsCount),
            L"Inventory items: " + std::to_wstring(database_.productsCount) + L"   Orders: " + std::to_wstring(database_.ordersCount),
            L"Sales records: " + std::to_wstring(database_.salesCount),
            L"Backup: available from Settings for permitted administrators"
        };
        int y = dbText.top + ui::Scale(8);
        for (const auto& line : lines) {
            ui::DrawTextLine(hdc, line, ui::MakeRect(dbText.left + ui::Scale(8), y, dbText.right - ui::Scale(8), y + ui::Scale(24)),
                theme::BodyFont(), kDashboardDarkText, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
            y += ui::Scale(28);
        }
        return;
    }

    const DashboardLayout layout = BuildLayout(client);

    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawTextLine(hdc, UiText(L"Period", L"Период"),
        ui::MakeRect(layout.periodRow.left, layout.periodRow.top - ui::Scale(2), layout.periodRow.right, layout.periodRow.top + ui::Scale(16)),
        theme::SmallBoldFont(), kDashboardMutedText, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    const std::wstring revenueCaption = CaptionForMetric(snapshot_.metrics.totalRevenue,
        UiText(L"Revenue for selected period", L"Revenue for selected period"),
        UiText(L"No revenue recorded yet", L"Выручка пока не записана"));
    const std::wstring revenueTrend = LooksZero(snapshot_.metrics.totalRevenue) ? L"0% this month" : snapshot_.metrics.revenueCaption;
    const std::wstring ordersCaption = CaptionForMetric(snapshot_.metrics.totalOrders,
        snapshot_.metrics.ordersCaption,
        UiText(L"No orders awaiting shipment", L"Нет заказов на отгрузку"));
    const std::wstring productsCaption = CaptionForMetric(snapshot_.metrics.productsSold,
        snapshot_.metrics.productsCaption,
        UiText(L"No inventory items sold yet", L"Проданных товаров пока нет"));
    const std::wstring clientsCaption = CaptionForMetric(snapshot_.metrics.activeClients,
        snapshot_.metrics.activeClients + UiText(L" active customers", L" активных клиентов"),
        UiText(L"No active customers yet", L"Активных клиентов пока нет"));

    DrawDashboardKpiCard(hdc, layout.kpis[0], UiText(L"Total Revenue", L"Общая выручка"),
        snapshot_.metrics.totalRevenue, revenueCaption, revenueTrend, L"$", RGB(37, 99, 235));
    DrawDashboardKpiCard(hdc, layout.kpis[1], UiText(L"Total Orders", L"Всего заказов"),
        snapshot_.metrics.totalOrders, ordersCaption, UiText(L"Orders awaiting processing", L"Orders awaiting processing"), L"↗", RGB(5, 150, 105));
    DrawDashboardKpiCard(hdc, layout.kpis[2], UiText(L"Items Sold", L"Продано товаров"),
        snapshot_.metrics.productsSold, productsCaption, UiText(L"Products sold this period", L"Products sold this period"), L"▥", RGB(217, 119, 6));
    DrawDashboardKpiCard(hdc, layout.kpis[3], UiText(L"Active Customers", L"Активные клиенты"),
        snapshot_.metrics.activeClients, clientsCaption, UiText(L"Active customer accounts", L"Active customer accounts"), L"◉", RGB(37, 99, 235));

    DrawDashboardLineChart(hdc, layout.revenueChart, snapshot_.revenueTrend);
    DrawDashboardBarChart(hdc, layout.volumeChart, snapshot_.monthlyOrderVolume);

    DrawDashboardPanel(hdc, layout.topProducts, UiText(L"Top Selling Items", L"Лидеры продаж"),
        UiText(L"Best-performing inventory items ranked by sold units and revenue.", L"Лучшие товары по продажам и выручке."));
    if (snapshot_.topProducts.empty()) {
        DrawEmptyState(hdc, ListBounds(layout.topProducts), L"▦", UiText(L"No product sales yet", L"Продаж товаров пока нет"),
            UiText(L"Best-selling inventory items will appear after completed sales.", L"Лидеры продаж появятся после завершённых продаж."));
    }

    DrawDashboardPanel(hdc, layout.recentOrders, UiText(L"Recent Orders", L"Последние заказы"),
        UiText(L"Latest orders from the order ledger.", L"Последние заказы из журнала."));
    if (snapshot_.recentOrders.empty()) {
        DrawEmptyState(hdc, ListBounds(layout.recentOrders), L"□", UiText(L"No recent orders", L"Последних заказов нет"),
            UiText(L"New orders will appear here automatically.", L"Новые заказы появятся здесь автоматически."));
    }

    DrawDashboardPanel(hdc, layout.lowStock, UiText(L"Operational Notifications", L"Операционные уведомления"),
        UiText(L"Tasks, stock risks and stalled deals requiring attention.", L"Задачи, складские риски и зависшие сделки."));
    if (snapshot_.lowStock.empty()) {
        DrawEmptyState(hdc, ListBounds(layout.lowStock), L"✓", UiText(L"No operational alerts", L"Операционных предупреждений нет"),
            UiText(L"Everything is running normally.", L"Всё работает штатно."));
    }
    return;
#if 0
    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"Performance Snapshot", L"Сводка показателей"),
        UiText(L"A compact executive overview aligned to the current sales, orders, products, and client dataset.",
               L"Краткая управленческая сводка по текущим данным продаж, заказов, товаров и клиентов из SQL."),
        ui::Width(layout.title));

    ui::DrawKpiCard(hdc, layout.kpis[0], UiText(L"Total Revenue", L"Общая выручка"), snapshot_.metrics.totalRevenue, snapshot_.metrics.revenueCaption, theme::kBlue);
    ui::DrawKpiCard(hdc, layout.kpis[1], UiText(L"Total Orders", L"Всего заказов"), snapshot_.metrics.totalOrders, snapshot_.metrics.ordersCaption, theme::kGreen);
    ui::DrawKpiCard(hdc, layout.kpis[2], UiText(L"Products Sold", L"Продано товаров"), snapshot_.metrics.productsSold, snapshot_.metrics.productsCaption, theme::kAmber);
    ui::DrawKpiCard(hdc, layout.kpis[3], UiText(L"Active Clients", L"Активные клиенты"), snapshot_.metrics.activeClients, snapshot_.metrics.clientsCaption, theme::kAccent);

    ui::DrawLineChartPanel(hdc, layout.revenueChart, UiText(L"Rolling Revenue Trend", L"Динамика выручки"),
        UiText(L"Last twelve reporting months aggregated directly from SQL sales data.",
               L"Последние двенадцать отчётных месяцев по данным продаж из SQL."),
        snapshot_.revenueTrend, theme::kBlue, MinValue(snapshot_.revenueTrend, 100.0), MaxValue(snapshot_.revenueTrend, 500.0), L"$", L"k");

    ui::DrawBarChartPanel(hdc, layout.volumeChart, UiText(L"Monthly Order Volume", L"Объём заказов по месяцам"),
        UiText(L"Monthly order intake generated from the database-backed order register.",
               L"Месячный объём заказов по данным регистра заказов в базе."),
        snapshot_.monthlyOrderVolume, theme::kGreen, MaxValue(snapshot_.monthlyOrderVolume, 150.0), L"", L"");

    ui::DrawRoundedPanel(hdc, layout.topProducts, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.topProducts.left + ui::Scale(16), layout.topProducts.top + ui::Scale(14),
        UiText(L"Top Selling Products", L"Лидеры продаж"),
        UiText(L"Best-performing products ranked by sold units and recognized revenue.", L"Лучшие товары по количеству продаж и выручке."),
        ui::Width(layout.topProducts) - ui::Scale(32));

    ui::DrawRoundedPanel(hdc, layout.recentOrders, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.recentOrders.left + ui::Scale(16), layout.recentOrders.top + ui::Scale(14),
        UiText(L"Recent Orders", L"Последние заказы"),
        UiText(L"Latest transactions currently present in the SQL order ledger.", L"Последние операции, доступные в SQL-реестре заказов."),
        ui::Width(layout.recentOrders) - ui::Scale(32));

    ui::DrawRoundedPanel(hdc, layout.lowStock, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.lowStock.left + ui::Scale(16), layout.lowStock.top + ui::Scale(14),
        UiText(L"Operational Notifications", L"Operational Notifications"),
        UiText(L"Overdue tasks, low stock, and stalled deals requiring attention.", L"Overdue tasks, low stock, and stalled deals requiring attention."),
        ui::Width(layout.lowStock) - ui::Scale(32));
#endif
}

void DashboardPage::UpdatePeriodButtons() {
    for (int i = 0; i < 4; ++i) {
        ui::SetButtonKind(periodButtons_[i], i == activePeriod_ ? ui::ButtonKind::Primary : ui::ButtonKind::Secondary);
        InvalidateRect(periodButtons_[i], nullptr, TRUE);
    }
}

LRESULT DashboardPage::OnCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    if (id >= IDC_PERIOD_TODAY && id <= IDC_PERIOD_YEAR) {
        activePeriod_ = id - IDC_PERIOD_TODAY;
        UpdatePeriodButtons();
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    }
    return 0;
}

LRESULT DashboardPage::OnDrawItem(WPARAM, LPARAM lParam) {
    auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
    ui::DrawUiButton(draw, ui::GetButtonKind(draw->hwndItem), false);
    return TRUE;
}
