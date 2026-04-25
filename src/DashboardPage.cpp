#include "DashboardPage.h"

#include "DataRepository.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>

namespace {

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

struct DashboardLayout {
    RECT title{};
    RECT kpis[4]{};
    RECT revenueChart{};
    RECT volumeChart{};
    RECT topProducts{};
    RECT recentOrders{};
    RECT lowStock{};
};

DashboardLayout BuildLayout(const RECT& client) {
    DashboardLayout layout{};
    const auto metrics = ui::GetMetrics();
    RECT area = ui::Inset(client, metrics.outerPadding, metrics.outerPadding);
    layout.title = ui::TakeTop(&area, metrics.titleBlockHeight, metrics.sectionGap);

    RECT kpiRow = ui::TakeTop(&area, metrics.kpiHeight, metrics.sectionGap);
    const auto kpiColumns = ui::SplitColumns(kpiRow, 4, metrics.sectionGap);
    for (int index = 0; index < 4 && index < static_cast<int>(kpiColumns.size()); ++index) {
        layout.kpis[index] = kpiColumns[index];
    }

    const int remainingHeight = ui::Height(area);
    const int bottomReserve = std::min(ui::Scale(260), std::max(ui::Scale(110), remainingHeight / 2));
    int chartHeight = std::clamp(remainingHeight - bottomReserve - metrics.sectionGap, ui::Scale(120), ui::Scale(232));
    if (chartHeight + metrics.sectionGap + bottomReserve > remainingHeight) {
        chartHeight = std::max(0, remainingHeight - metrics.sectionGap - bottomReserve);
    }
    RECT chartsRow = ui::TakeTop(&area, chartHeight, metrics.sectionGap);
    const auto chartColumns = ui::SplitColumns(chartsRow, 2, metrics.sectionGap);
    layout.revenueChart = chartColumns[0];
    layout.volumeChart = chartColumns[1];

    const auto bottomColumns = ui::SplitColumns(area, 2, metrics.sectionGap);
    layout.topProducts = bottomColumns[0];
    RECT rightColumn = bottomColumns[1];
    const int minPanelHeight = ui::Scale(86);
    int recentHeight = std::max(minPanelHeight, (ui::Height(rightColumn) - metrics.sectionGap) / 2);
    if (ui::Height(rightColumn) > metrics.sectionGap + minPanelHeight) {
        recentHeight = std::min(recentHeight, ui::Height(rightColumn) - metrics.sectionGap - minPanelHeight);
    }
    layout.recentOrders = ui::TakeTop(&rightColumn, recentHeight, metrics.sectionGap);
    layout.lowStock = rightColumn;
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

RECT ListBounds(const RECT& panel) {
    const auto metrics = ui::GetMetrics();
    return ui::Inset(ui::MakeRect(panel.left, panel.top + metrics.panelHeaderHeight, panel.right, panel.bottom), ui::Scale(12), ui::Scale(12));
}

} // namespace

DashboardPage::DashboardPage()
    : PageBase(L"Executive Dashboard", L"Revenue indicators, SQL-backed operational watchlists, and the latest commercial activity.") {}

const wchar_t* DashboardPage::ClassName() const { return L"NativeDashboardPage"; }

void DashboardPage::OnCreate() {
    topProducts_ = ui::CreateUiListView(hwnd_, 2001);
    recentOrders_ = ui::CreateUiListView(hwnd_, 2002);
    lowStock_ = ui::CreateUiListView(hwnd_, 2003);

    ui::AddListColumns(topProducts_, {UiText(L"Product", L"Товар"), UiText(L"Category", L"Категория"), UiText(L"Units", L"Ед."), UiText(L"Revenue", L"Выручка"), UiText(L"Margin", L"Маржа")}, {220, 130, 90, 120, 90});
    ui::AddListColumns(recentOrders_, {UiText(L"Order ID", L"Заказ"), UiText(L"Client", L"Клиент"), UiText(L"Date", L"Дата"), UiText(L"Total", L"Итого"), UiText(L"Status", L"Статус")}, {120, 220, 120, 110, 110});
    ui::AddListColumns(lowStock_, {UiText(L"Alert", L"Alert"), UiText(L"Item", L"Item"), UiText(L"Owner", L"Owner"), UiText(L"Due / Value", L"Due / Value"), UiText(L"Status", L"Status")}, {100, 240, 180, 130, 110});

    LoadSnapshot();
}

void DashboardPage::OnActivate() {
    LoadSnapshot();
}

void DashboardPage::LoadSnapshot() {
    snapshot_ = data::Repository::Instance().LoadDashboardSnapshot();
    FillTables();
}

void DashboardPage::FillTables() {
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
}

void DashboardPage::OnSize(int width, int height) {
    const DashboardLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    RECT list = ListBounds(layout.topProducts);
    ui::MoveWindowToRect(topProducts_, list);
    list = ListBounds(layout.recentOrders);
    ui::MoveWindowToRect(recentOrders_, list);
    list = ListBounds(layout.lowStock);
    ui::MoveWindowToRect(lowStock_, list);
}

void DashboardPage::OnPaint(HDC hdc, const RECT& client) {
    const DashboardLayout layout = BuildLayout(client);

    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"Performance Snapshot", L"Сводка показателей"),
        UiText(L"A compact executive overview aligned to the current SQL-backed sales, orders, products, and client dataset.",
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
}
