#include "EmployeesPage.h"

#include "DataRepository.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>
#include <numeric>
#include <sstream>

namespace {

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

std::wstring FormatMoney(double value) {
    std::wostringstream stream;
    stream << L"$" << static_cast<long long>(value + 0.5);
    return stream.str();
}

std::wstring FormatPercent(double value) {
    std::wostringstream stream;
    stream.setf(std::ios::fixed);
    stream.precision(1);
    stream << value << L"%";
    return stream.str();
}

struct EmployeesLayout {
    RECT title{};
    RECT kpis[4]{};
    RECT chart{};
    RECT tablePanel{};
    RECT table{};
};

EmployeesLayout BuildLayout(const RECT& client) {
    EmployeesLayout layout{};
    const auto m = ui::GetMetrics();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);
    RECT kpiRow = ui::TakeTop(&area, m.kpiHeight, m.sectionGap);
    const auto kpis = ui::SplitColumns(kpiRow, 4, m.sectionGap);
    for (int i = 0; i < 4; ++i) {
        layout.kpis[i] = kpis[i];
    }
    layout.chart = ui::TakeTop(&area, std::clamp(ui::Height(area) / 2, ui::Scale(170), ui::Scale(230)), m.sectionGap);
    layout.tablePanel = area;
    layout.table = ui::Inset(ui::MakeRect(layout.tablePanel.left, layout.tablePanel.top + m.panelHeaderHeight, layout.tablePanel.right, layout.tablePanel.bottom), ui::Scale(12), ui::Scale(12));
    return layout;
}

} // namespace

EmployeesPage::EmployeesPage() : PageBase(L"Employee Performance", L"Manager productivity, commercial contribution, and team revenue tracking.") {}

const wchar_t* EmployeesPage::ClassName() const { return L"NativeEmployeesPage"; }

void EmployeesPage::OnCreate() {
    employeesTable_ = ui::CreateUiListView(hwnd_, 7001);
    ui::AddListColumns(employeesTable_, {UiText(L"Employee", L"Сотрудник"), UiText(L"Role", L"Роль"), UiText(L"Completed Sales", L"Продаж"), UiText(L"Revenue", L"Выручка"), UiText(L"Avg Margin", L"Ср. маржа"), UiText(L"Conversion", L"Конверсия"), UiText(L"Status", L"Статус")}, {150, 180, 120, 110, 100, 100, 140});
    ReloadData();
}

void EmployeesPage::OnActivate() {
    ReloadData();
}

void EmployeesPage::ReloadData() {
    employees_ = data::Repository::Instance().LoadEmployeePerformance();
    ui::ClearListRows(employeesTable_);
    for (size_t index = 0; index < employees_.size(); ++index) {
        const auto& row = employees_[index];
        ui::AddListRow(employeesTable_, static_cast<int>(index), {
            row.fullName,
            i18n::DataText(row.role),
            std::to_wstring(row.completedSales),
            FormatMoney(row.revenue),
            FormatPercent(row.avgMarginPercent),
            FormatPercent(row.conversionPercent),
            i18n::DataText(row.status)
        });
    }
    ShowWindow(employeesTable_, employees_.empty() ? SW_HIDE : SW_SHOW);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void EmployeesPage::OnSize(int width, int height) {
    const EmployeesLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    ui::MoveWindowToRect(employeesTable_, layout.table);
}

void EmployeesPage::OnPaint(HDC hdc, const RECT& client) {
    const EmployeesLayout layout = BuildLayout(client);
    const int activeManagers = static_cast<int>(employees_.size());
    const int completedSales = std::accumulate(employees_.begin(), employees_.end(), 0,
        [](int sum, const data::EmployeePerformanceRecord& item) { return sum + item.completedSales; });
    const double teamRevenue = std::accumulate(employees_.begin(), employees_.end(), 0.0,
        [](double sum, const data::EmployeePerformanceRecord& item) { return sum + item.revenue; });
    const auto bestIt = std::max_element(employees_.begin(), employees_.end(),
        [](const auto& left, const auto& right) { return left.conversionPercent < right.conversionPercent; });
    const double bestConversion = bestIt == employees_.end() ? 0.0 : bestIt->conversionPercent;

    std::vector<ui::SeriesPoint> chartValues;
    for (const auto& row : employees_) {
        std::wstring label = row.fullName;
        const auto space = label.find(L' ');
        if (space != std::wstring::npos && space + 1 < label.size()) {
            label = label.substr(0, 1) + L"." + label.substr(space + 1, 1) + L".";
        }
        chartValues.push_back({row.revenue / 1000.0, label});
    }
    if (chartValues.empty()) {
        chartValues.push_back({0.0, L"-"});
    }
    double maxRevenue = 100.0;
    for (const auto& point : chartValues) {
        maxRevenue = std::max(maxRevenue, point.value + 50.0);
    }

    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"Employees", L"Сотрудники"),
        UiText(L"Manage staff accounts, roles and departments.", L"Управление сотрудниками, ролями и отделами."),
        ui::Width(layout.title));

    ui::DrawKpiCard(hdc, layout.kpis[0], UiText(L"Active Managers", L"Активные менеджеры"), std::to_wstring(activeManagers), UiText(L"Loaded from SQL employees table", L"Загружено из таблицы сотрудников"), theme::kBlue);
    ui::DrawKpiCard(hdc, layout.kpis[1], UiText(L"Completed Sales", L"Закрытые продажи"), std::to_wstring(completedSales), UiText(L"Completed sales in SQL", L"Завершённые продажи в SQL"), theme::kGreen);
    ui::DrawKpiCard(hdc, layout.kpis[2], UiText(L"Team Revenue", L"Выручка команды"), FormatMoney(teamRevenue), UiText(L"Current database total", L"Итог по текущей базе"), theme::kAmber);
    ui::DrawKpiCard(hdc, layout.kpis[3], UiText(L"Best Conversion", L"Лучшая конверсия"), FormatPercent(bestConversion), bestIt == employees_.end() ? L"-" : bestIt->fullName, theme::kAccent);

    ui::DrawBarChartPanel(hdc, layout.chart, UiText(L"Revenue by Employee", L"Выручка по сотрудникам"), L"", chartValues, theme::kBlue, maxRevenue, L"$", L"k");

    ui::DrawRoundedPanel(hdc, layout.tablePanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.tablePanel.left + ui::Scale(16), layout.tablePanel.top + ui::Scale(14), UiText(L"Employees Table", L"Таблица сотрудников"), L"", ui::Width(layout.tablePanel) - ui::Scale(32));
    if (employees_.empty()) {
        ui::DrawEmptyState(hdc, layout.table, UiText(L"No employees found", L"Сотрудники не найдены"),
            UiText(L"Add staff accounts to manage system access.", L"Добавьте сотрудников для управления доступом."), L"\u25CF");
    }
}
