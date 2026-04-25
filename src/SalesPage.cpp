#include "SalesPage.h"

#include "AccessControl.h"
#include "DataRepository.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <numeric>
#include <sstream>

namespace {
enum : int { IDC_SEARCH = 3001, IDC_FROM, IDC_TO, IDC_STATUS, IDC_CHANNEL, IDC_ADD, IDC_STATUS_ACTION, IDC_DELETE, IDC_REFRESH, IDC_EXPORT, IDC_TABLE };

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

struct SalesLayout {
    RECT title{};
    RECT kpis[4]{};
    RECT filterPanel{};
    RECT search{};
    RECT from{};
    RECT to{};
    RECT status{};
    RECT channel{};
    RECT add{};
    RECT statusAction{};
    RECT remove{};
    RECT refresh{};
    RECT exportButton{};
    RECT tablePanel{};
    RECT table{};
};

SalesLayout BuildLayout(const RECT& client) {
    SalesLayout layout{};
    const auto m = ui::GetMetrics();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);

    RECT kpiRow = ui::TakeTop(&area, m.kpiHeight, m.sectionGap);
    const auto kpis = ui::SplitColumns(kpiRow, 4, m.sectionGap);
    for (int i = 0; i < 4; ++i) {
        layout.kpis[i] = kpis[i];
    }

    layout.filterPanel = ui::TakeTop(&area, ui::Scale(126), m.sectionGap);
    layout.tablePanel = area;

    RECT filterInner = ui::Inset(layout.filterPanel, ui::Scale(14), ui::Scale(12));
    RECT filterRow = ui::MakeRect(filterInner.left, layout.filterPanel.top + ui::Scale(38), filterInner.right, layout.filterPanel.top + ui::Scale(38) + m.controlHeight);
    layout.channel = ui::TakeRight(&filterRow, ui::Scale(138), m.compactGap);
    layout.status = ui::TakeRight(&filterRow, ui::Scale(138), m.compactGap);
    layout.to = ui::TakeRight(&filterRow, ui::Scale(126), m.compactGap);
    layout.from = ui::TakeRight(&filterRow, ui::Scale(126), m.compactGap);
    layout.search = filterRow;

    RECT actionRow = ui::MakeRect(filterInner.left, layout.search.bottom + ui::Scale(12), filterInner.right, layout.search.bottom + ui::Scale(12) + m.buttonHeight);
    const int actionWidth = ui::IconButtonWidth();
    layout.exportButton = ui::TakeRight(&actionRow, actionWidth, m.compactGap);
    layout.refresh = ui::TakeRight(&actionRow, actionWidth, m.compactGap);
    layout.remove = ui::TakeRight(&actionRow, actionWidth, m.compactGap);
    layout.statusAction = ui::TakeRight(&actionRow, actionWidth, m.compactGap);
    layout.add = ui::TakeRight(&actionRow, actionWidth, m.compactGap);

    layout.table = ui::Inset(ui::MakeRect(layout.tablePanel.left, layout.tablePanel.top + m.panelHeaderHeight, layout.tablePanel.right, layout.tablePanel.bottom), ui::Scale(12), ui::Scale(12));
    return layout;
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

void DrawFilterLabel(HDC hdc, const RECT& control, const std::wstring& text) {
    ui::DrawTextLine(hdc, text, ui::MakeRect(control.left, control.top - ui::Scale(22), control.right, control.top - ui::Scale(4)),
        theme::SmallBoldFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE);
}

bool Can(access::Permission permission) {
    return access::HasPermission(data::Repository::Instance().CurrentAccount(), permission);
}

void SetActionVisible(HWND hwnd, bool visible) {
    ShowWindow(hwnd, SW_SHOW);
    EnableWindow(hwnd, visible ? TRUE : FALSE);
}

} // namespace

SalesPage::SalesPage() : PageBase(L"Sales Monitoring", L"Sales transactions, performance summaries, and filterable SQL-backed commercial data.") {}

const wchar_t* SalesPage::ClassName() const { return L"NativeSalesPage"; }

void SalesPage::OnCreate() {
    searchEdit_ = ui::CreateUiEdit(hwnd_, IDC_SEARCH);
    fromDate_ = ui::CreateUiDatePicker(hwnd_, IDC_FROM);
    toDate_ = ui::CreateUiDatePicker(hwnd_, IDC_TO);
    statusCombo_ = ui::CreateUiCombo(hwnd_, IDC_STATUS);
    channelCombo_ = ui::CreateUiCombo(hwnd_, IDC_CHANNEL);
    addButton_ = ui::CreateUiButton(hwnd_, IDC_ADD, L"+", ui::ButtonKind::Primary);
    statusButton_ = ui::CreateUiButton(hwnd_, IDC_STATUS_ACTION, L"\u2713", ui::ButtonKind::Secondary);
    deleteButton_ = ui::CreateUiButton(hwnd_, IDC_DELETE, L"\u2715", ui::ButtonKind::Danger);
    refreshButton_ = ui::CreateUiButton(hwnd_, IDC_REFRESH, L"\u21BB", ui::ButtonKind::Secondary);
    exportButton_ = ui::CreateUiButton(hwnd_, IDC_EXPORT, L"\u21E9", ui::ButtonKind::Primary);
    salesTable_ = ui::CreateUiListView(hwnd_, IDC_TABLE);

    ui::SetEditCueBanner(searchEdit_, UiText(L"Search by sale id, manager, client, or product", L"Поиск по продаже, менеджеру, клиенту или товару"));
    ui::AddComboItems(statusCombo_, {L"All Statuses", L"Completed", L"Pending", L"Returned", L"Cancelled"});
    ui::AddComboItems(channelCombo_, {L"All Channels", L"Direct", L"Online", L"Partner"});
    ui::AddListColumns(salesTable_, {UiText(L"Sale ID", L"Продажа"), UiText(L"Manager", L"Менеджер"), UiText(L"Client", L"Клиент"), UiText(L"Product", L"Товар"), UiText(L"Qty", L"Кол-во"), UiText(L"Payment", L"Оплата"), UiText(L"Revenue", L"Выручка"), UiText(L"Margin", L"Маржа"), UiText(L"Date", L"Дата"), UiText(L"Status", L"Статус")},
        {100, 140, 170, 180, 58, 110, 100, 90, 110, 100});
    DateTime_SetSystemtime(fromDate_, GDT_NONE, nullptr);
    DateTime_SetSystemtime(toDate_, GDT_NONE, nullptr);

    ApplyAccess();
    ReloadData();
}

void SalesPage::OnActivate() {
    ApplyAccess();
    ReloadData();
}

void SalesPage::ApplyAccess() {
    SetActionVisible(addButton_, Can(access::Permission::CreateSale));
    SetActionVisible(statusButton_, Can(access::Permission::ChangeSaleStatus));
    SetActionVisible(deleteButton_, Can(access::Permission::DeleteSale));
    SetActionVisible(exportButton_, Can(access::Permission::ExportSales));
}

data::SalesFilter SalesPage::BuildFilter() const {
    data::SalesFilter filter{};
    wchar_t buffer[256]{};
    GetWindowTextW(searchEdit_, buffer, 256);
    filter.search = buffer;

    GetWindowTextW(statusCombo_, buffer, 128);
    filter.status = buffer;
    GetWindowTextW(channelCombo_, buffer, 128);
    filter.channel = buffer;

    ui::TryGetDatePickerIso(fromDate_, &filter.fromDateIso);
    ui::TryGetDatePickerIso(toDate_, &filter.toDateIso);
    return filter;
}

void SalesPage::ReloadData() {
    sales_ = data::Repository::Instance().LoadSales(BuildFilter());
    ui::ClearListRows(salesTable_);
    for (size_t index = 0; index < sales_.size(); ++index) {
        const auto& row = sales_[index];
        ui::AddListRow(salesTable_, static_cast<int>(index), {
            row.saleCode,
            row.manager,
            row.client,
            row.product,
            std::to_wstring(row.quantity),
            row.paymentMethod,
            FormatMoney(row.revenue),
            FormatPercent(row.marginPercent),
            row.dateDisplay,
            i18n::DataText(row.status)
        });
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void SalesPage::OnSize(int width, int height) {
    const SalesLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    ui::MoveWindowToRect(searchEdit_, layout.search);
    ui::MoveWindowToRect(fromDate_, layout.from);
    ui::MoveWindowToRect(toDate_, layout.to);
    ui::MoveWindowToRect(statusCombo_, layout.status);
    ui::MoveWindowToRect(channelCombo_, layout.channel);
    ui::MoveWindowToRect(addButton_, layout.add);
    ui::MoveWindowToRect(statusButton_, layout.statusAction);
    ui::MoveWindowToRect(deleteButton_, layout.remove);
    ui::MoveWindowToRect(refreshButton_, layout.refresh);
    ui::MoveWindowToRect(exportButton_, layout.exportButton);
    ui::MoveWindowToRect(salesTable_, layout.table);
}

void SalesPage::OnPaint(HDC hdc, const RECT& client) {
    const SalesLayout layout = BuildLayout(client);
    const double totalRevenue = std::accumulate(sales_.begin(), sales_.end(), 0.0,
        [](double sum, const data::SalesRecord& item) { return sum + item.revenue; });
    const double avgTicket = sales_.empty() ? 0.0 : totalRevenue / static_cast<double>(sales_.size());
    int completedDeals = 0;
    double avgMargin = 0.0;
    for (const auto& sale : sales_) {
        completedDeals += sale.status == L"Completed" ? 1 : 0;
        avgMargin += sale.marginPercent;
    }
    if (!sales_.empty()) {
        avgMargin /= static_cast<double>(sales_.size());
    }
    const double completionRate = sales_.empty() ? 0.0 : (completedDeals * 100.0 / static_cast<double>(sales_.size()));

    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"Sales Register", L"Реестр продаж"),
        UiText(L"A denser, filterable transaction workspace with consistent controls and a live SQL-backed data grid.",
               L"Плотный рабочий экран с фильтрами, едиными контролами и живой SQL-таблицей."),
        ui::Width(layout.title));

    ui::DrawKpiCard(hdc, layout.kpis[0], UiText(L"Filtered Revenue", L"Выручка по фильтру"), FormatMoney(totalRevenue), UiText(L"Current result set", L"Текущий набор результатов"), theme::kBlue);
    ui::DrawKpiCard(hdc, layout.kpis[1], UiText(L"Average Ticket", L"Средний чек"), FormatMoney(avgTicket), UiText(L"Across active query rows", L"По активным строкам выборки"), theme::kGreen);
    ui::DrawKpiCard(hdc, layout.kpis[2], UiText(L"Completed Deals", L"Закрытые сделки"), std::to_wstring(completedDeals), FormatPercent(completionRate) + UiText(L" completion rate", L" уровень завершения"), theme::kAmber);
    ui::DrawKpiCard(hdc, layout.kpis[3], UiText(L"Average Margin", L"Средняя маржа"), FormatPercent(avgMargin), std::to_wstring(sales_.size()) + UiText(L" rows loaded", L" строк загружено"), theme::kAccent);

    ui::DrawRoundedPanel(hdc, layout.filterPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    DrawFilterLabel(hdc, layout.search, UiText(L"Search", L"Поиск"));
    DrawFilterLabel(hdc, layout.from, UiText(L"From", L"С"));
    DrawFilterLabel(hdc, layout.to, UiText(L"To", L"По"));
    DrawFilterLabel(hdc, layout.status, UiText(L"Status", L"Статус"));
    DrawFilterLabel(hdc, layout.channel, UiText(L"Channel", L"Канал"));

    ui::DrawRoundedPanel(hdc, layout.tablePanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.tablePanel.left + ui::Scale(16), layout.tablePanel.top + ui::Scale(14), UiText(L"Sales Transactions", L"Транзакции продаж"),
        UiText(L"Search, status, date, and channel filters are aligned into one consistent toolbar system.",
               L"Поиск, статус, даты и каналы выровнены в единую панель фильтров."),
        ui::Width(layout.tablePanel) - ui::Scale(32));
    ui::DrawTextLine(hdc, UiText(L"Result rows: ", L"Строк найдено: ") + std::to_wstring(sales_.size()),
        ui::MakeRect(layout.tablePanel.right - ui::Scale(220), layout.tablePanel.top + ui::Scale(20), layout.tablePanel.right - ui::Scale(18), layout.tablePanel.top + ui::Scale(40)),
        theme::SmallBoldFont(), theme::kTextSecondary, DT_RIGHT | DT_SINGLELINE);
}

LRESULT SalesPage::OnCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);

    if (id == IDC_ADD && code == BN_CLICKED) {
        if (!data::Repository::Instance().AddDemoSale()) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Add Sale", L"Add Sale").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_STATUS_ACTION && code == BN_CLICKED) {
        const int index = ListView_GetNextItem(salesTable_, -1, LVNI_SELECTED);
        if (index < 0 || index >= static_cast<int>(sales_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select a sale first.", L"Select a sale first.").c_str(), UiText(L"Change Status", L"Change Status").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (!data::Repository::Instance().AdvanceSaleStatus(sales_[index].id)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Change Status", L"Change Status").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_DELETE && code == BN_CLICKED) {
        const int index = ListView_GetNextItem(salesTable_, -1, LVNI_SELECTED);
        if (index < 0 || index >= static_cast<int>(sales_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select a sale first.", L"Select a sale first.").c_str(), UiText(L"Delete Sale", L"Delete Sale").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (MessageBoxW(hwnd_, UiText(L"Delete the selected sale?", L"Delete the selected sale?").c_str(), UiText(L"Delete Sale", L"Delete Sale").c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES) {
            if (!data::Repository::Instance().DeleteSale(sales_[index].id)) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Delete Sale", L"Delete Sale").c_str(), MB_OK | MB_ICONWARNING);
            }
            ReloadData();
        }
        return 0;
    }

    if (id == IDC_REFRESH && code == BN_CLICKED) {
        ReloadData();
        return 0;
    }

    if (id == IDC_EXPORT && code == BN_CLICKED) {
        const std::wstring filePath = data::Repository::Instance().ExportSalesCsv(BuildFilter());
        if (filePath.empty()) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Export CSV", L"Экспорт CSV").c_str(), MB_OK | MB_ICONWARNING);
        } else {
            MessageBoxW(hwnd_, filePath.c_str(), UiText(L"CSV Export Created", L"CSV экспорт создан").c_str(), MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    if ((id == IDC_SEARCH && code == EN_CHANGE) ||
        ((id == IDC_STATUS || id == IDC_CHANNEL) && code == CBN_SELCHANGE)) {
        ReloadData();
    }

    return 0;
}

LRESULT SalesPage::OnDrawItem(WPARAM, LPARAM lParam) {
    ui::DrawUiButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam), ui::GetButtonKind(reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->hwndItem), false);
    return TRUE;
}

LRESULT SalesPage::OnNotify(WPARAM wParam, LPARAM lParam) {
    const auto* header = reinterpret_cast<const NMHDR*>(lParam);
    if (header && (header->idFrom == IDC_FROM || header->idFrom == IDC_TO) && header->code == DTN_DATETIMECHANGE) {
        ReloadData();
        return 0;
    }
    return PageBase::OnNotify(wParam, lParam);
}
