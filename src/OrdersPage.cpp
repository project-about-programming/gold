#include "OrdersPage.h"

#include "AccessControl.h"
#include "DataRepository.h"
#include "DataEntryDialog.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <sstream>

namespace {
enum : int { IDC_SEARCH = 4001, IDC_STATUS, IDC_FROM, IDC_TO, IDC_ADD, IDC_EDIT, IDC_DELETE, IDC_TABLE };

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

struct OrdersLayout {
    RECT title{};
    RECT kpis[4]{};
    RECT filterPanel{};
    RECT search{};
    RECT status{};
    RECT from{};
    RECT to{};
    RECT add{};
    RECT advance{};
    RECT remove{};
    RECT tablePanel{};
    RECT table{};
};

OrdersLayout BuildLayout(const RECT& client) {
    OrdersLayout layout{};
    const auto m = ui::GetMetrics();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);

    RECT kpiRow = ui::TakeTop(&area, m.kpiHeight, m.sectionGap);
    const auto kpis = ui::SplitColumns(kpiRow, 4, m.sectionGap);
    for (int i = 0; i < 4; ++i) {
        layout.kpis[i] = kpis[i];
    }

    layout.filterPanel = ui::TakeTop(&area, ui::Scale(88), m.sectionGap);
    layout.tablePanel = area;

    RECT filterInner = ui::Inset(layout.filterPanel, ui::Scale(14), ui::Scale(12));
    const int actionWidth = ui::IconButtonWidth();
    layout.remove = ui::TakeRight(&filterInner, actionWidth, m.compactGap);
    layout.advance = ui::TakeRight(&filterInner, actionWidth, m.compactGap);
    layout.add = ui::TakeRight(&filterInner, actionWidth, m.compactGap);
    layout.to = ui::TakeRight(&filterInner, ui::Scale(126), m.compactGap);
    layout.from = ui::TakeRight(&filterInner, ui::Scale(126), m.compactGap);
    layout.status = ui::TakeRight(&filterInner, ui::Scale(138), m.compactGap);
    layout.search = filterInner;

    layout.search.top = layout.status.top = layout.from.top = layout.to.top = layout.add.top = layout.advance.top = layout.remove.top = layout.filterPanel.top + ui::Scale(40);
    layout.search.bottom = layout.status.bottom = layout.from.bottom = layout.to.bottom = layout.search.top + m.controlHeight;
    layout.add.bottom = layout.advance.bottom = layout.remove.bottom = layout.search.top + m.buttonHeight;

    layout.table = ui::Inset(ui::MakeRect(layout.tablePanel.left, layout.tablePanel.top + m.panelHeaderHeight, layout.tablePanel.right, layout.tablePanel.bottom), ui::Scale(12), ui::Scale(12));
    return layout;
}

std::wstring FormatMoney(double value) {
    std::wostringstream stream;
    stream << L"$" << static_cast<long long>(value + 0.5);
    return stream.str();
}

int SelectedRow(HWND listView) {
    return ListView_GetNextItem(listView, -1, LVNI_SELECTED);
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

OrdersPage::OrdersPage() : PageBase(L"Order Operations", L"Order lifecycle control with SQL-backed CRUD actions, delivery statuses, and search filters.") {}

const wchar_t* OrdersPage::ClassName() const { return L"NativeOrdersPage"; }

void OrdersPage::OnCreate() {
    searchEdit_ = ui::CreateUiEdit(hwnd_, IDC_SEARCH);
    statusCombo_ = ui::CreateUiCombo(hwnd_, IDC_STATUS);
    fromDate_ = ui::CreateUiDatePicker(hwnd_, IDC_FROM);
    toDate_ = ui::CreateUiDatePicker(hwnd_, IDC_TO);
    addButton_ = ui::CreateUiButton(hwnd_, IDC_ADD, L"+", ui::ButtonKind::Primary);
    editButton_ = ui::CreateUiButton(hwnd_, IDC_EDIT, L"\u2713", ui::ButtonKind::Secondary);
    deleteButton_ = ui::CreateUiButton(hwnd_, IDC_DELETE, L"\u2715", ui::ButtonKind::Danger);
    ordersTable_ = ui::CreateUiListView(hwnd_, IDC_TABLE);

    ui::SetEditCueBanner(searchEdit_, UiText(L"Search by order id, client, or product", L"Поиск по заказу, клиенту или товару"));
    ui::AddComboItems(statusCombo_, {L"All Statuses", L"Processing", L"Confirmed", L"Packed", L"Delivered"});
    ui::AddListColumns(ordersTable_, {UiText(L"Order ID", L"Заказ"), UiText(L"Client Name", L"Клиент"), UiText(L"Product", L"Товар"), UiText(L"Qty", L"Кол-во"), UiText(L"Total Price", L"Сумма"), UiText(L"Date", L"Дата"), UiText(L"Status", L"Статус"), UiText(L"Priority", L"Приоритет")},
        {100, 170, 220, 70, 110, 110, 110, 90});
    DateTime_SetSystemtime(fromDate_, GDT_NONE, nullptr);
    DateTime_SetSystemtime(toDate_, GDT_NONE, nullptr);

    ApplyAccess();
    ReloadData();
}

void OrdersPage::OnActivate() {
    ApplyAccess();
    ReloadData();
}

void OrdersPage::ApplyAccess() {
    SetActionVisible(addButton_, Can(access::Permission::CreateOrder));
    SetActionVisible(editButton_, Can(access::Permission::ChangeOrderStatus));
    SetActionVisible(deleteButton_, Can(access::Permission::DeleteOrder));
}

data::OrderFilter OrdersPage::BuildFilter() const {
    data::OrderFilter filter{};
    wchar_t buffer[256]{};
    GetWindowTextW(searchEdit_, buffer, 256);
    filter.search = buffer;
    GetWindowTextW(statusCombo_, buffer, 256);
    filter.status = buffer;
    ui::TryGetDatePickerIso(fromDate_, &filter.fromDateIso);
    ui::TryGetDatePickerIso(toDate_, &filter.toDateIso);
    return filter;
}

void OrdersPage::ReloadData() {
    orders_ = data::Repository::Instance().LoadOrders(BuildFilter());
    ui::ClearListRows(ordersTable_);
    for (size_t index = 0; index < orders_.size(); ++index) {
        const auto& row = orders_[index];
        ui::AddListRow(ordersTable_, static_cast<int>(index), {
            row.orderCode,
            row.client,
            row.product,
            std::to_wstring(row.quantity),
            FormatMoney(row.totalPrice),
            row.dateDisplay,
            i18n::DataText(row.status),
            i18n::DataText(row.priority)
        });
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void OrdersPage::OnSize(int width, int height) {
    const OrdersLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    ui::MoveWindowToRect(searchEdit_, layout.search);
    ui::MoveWindowToRect(statusCombo_, layout.status);
    ui::MoveWindowToRect(fromDate_, layout.from);
    ui::MoveWindowToRect(toDate_, layout.to);
    ui::MoveWindowToRect(addButton_, layout.add);
    ui::MoveWindowToRect(editButton_, layout.advance);
    ui::MoveWindowToRect(deleteButton_, layout.remove);
    ui::MoveWindowToRect(ordersTable_, layout.table);
}

void OrdersPage::OnPaint(HDC hdc, const RECT& client) {
    const OrdersLayout layout = BuildLayout(client);
    int processing = 0;
    int delivered = 0;
    int highPriority = 0;
    double revenue = 0.0;
    for (const auto& order : orders_) {
        processing += (order.status == L"Processing" || order.status == L"Confirmed" || order.status == L"Packed") ? 1 : 0;
        delivered += order.status == L"Delivered" ? 1 : 0;
        highPriority += order.priority == L"High" ? 1 : 0;
        revenue += order.totalPrice;
    }

    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"Orders Control", L"Управление заказами"),
        UiText(L"A safer order workspace with clearer actions, aligned filters, and stable resizing for the operational queue.",
               L"Более безопасное рабочее пространство заказов с понятными действиями, ровными фильтрами и устойчивым layout."),
        ui::Width(layout.title));

    ui::DrawKpiCard(hdc, layout.kpis[0], UiText(L"Visible Orders", L"Видимые заказы"), std::to_wstring(orders_.size()), UiText(L"Current filter result", L"Текущий результат фильтра"), theme::kBlue);
    ui::DrawKpiCard(hdc, layout.kpis[1], UiText(L"In Fulfillment", L"В обработке"), std::to_wstring(processing), UiText(L"Processing, confirmed, and packed", L"В обработке, подтверждено и упаковано"), theme::kGreen);
    ui::DrawKpiCard(hdc, layout.kpis[2], UiText(L"Delivered", L"Доставлено"), std::to_wstring(delivered), UiText(L"Completed deliveries in result set", L"Завершённые доставки в выборке"), theme::kAmber);
    ui::DrawKpiCard(hdc, layout.kpis[3], UiText(L"Total Value", L"Общая сумма"), FormatMoney(revenue), std::to_wstring(highPriority) + UiText(L" high-priority orders", L" приоритетных заказов"), theme::kAccent);

    ui::DrawRoundedPanel(hdc, layout.filterPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    DrawFilterLabel(hdc, layout.search, UiText(L"Search", L"Поиск"));
    DrawFilterLabel(hdc, layout.status, UiText(L"Status", L"Статус"));
    DrawFilterLabel(hdc, layout.from, UiText(L"From", L"С"));
    DrawFilterLabel(hdc, layout.to, UiText(L"To", L"По"));

    ui::DrawRoundedPanel(hdc, layout.tablePanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.tablePanel.left + ui::Scale(16), layout.tablePanel.top + ui::Scale(14), UiText(L"Orders Table", L"Таблица заказов"),
        UiText(L"Destructive and operational actions are separated so the flow feels deliberate and safer.", L"Операционные и опасные действия разведены, чтобы работа была безопаснее."),
        ui::Width(layout.tablePanel) - ui::Scale(32));
}

LRESULT OrdersPage::OnCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);

    if (id == IDC_ADD && code == BN_CLICKED) {
        std::vector<ui::DataEntryField> fields{
            {UiText(L"Client", L"Клиент"), UiText(L"Client ID or exact company name", L"ID клиента или точное название компании"), L"", true},
            {UiText(L"Product", L"Товар"), UiText(L"Product ID, SKU, or exact product name", L"ID, артикул или точное название товара"), L"", true},
            {UiText(L"Quantity", L"Количество"), UiText(L"Example: 3", L"Например: 3"), L"", true},
            {UiText(L"Status", L"Статус"), UiText(L"Processing / Confirmed / Packed / Delivered", L"Processing / Confirmed / Packed / Delivered"), L"", false}
        };
        if (!ui::ShowDataEntryDialog(hwnd_, UiText(L"Add Order", L"Добавить заказ"),
                UiText(L"Create an order from existing client and product records.", L"Создание заказа из уже существующих клиента и товара."),
                fields)) {
            return 0;
        }
        if (!data::Repository::Instance().CreateQuickEntry(L"Order", fields[0].value, fields[1].value, fields[2].value, fields[3].value)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Add Order", L"Добавить заказ").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_EDIT && code == BN_CLICKED) {
        const int index = SelectedRow(ordersTable_);
        if (index < 0 || index >= static_cast<int>(orders_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select an order first.", L"Сначала выберите заказ.").c_str(), UiText(L"Advance Status", L"Сменить статус").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (!data::Repository::Instance().AdvanceOrderStatus(orders_[index].id)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Advance Status", L"Сменить статус").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_DELETE && code == BN_CLICKED) {
        const int index = SelectedRow(ordersTable_);
        if (index < 0 || index >= static_cast<int>(orders_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select an order first.", L"Сначала выберите заказ.").c_str(), UiText(L"Delete Order", L"Удалить заказ").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (MessageBoxW(hwnd_, UiText(L"Delete the selected order from the demo database?", L"Удалить выбранный заказ из демо-базы?").c_str(), UiText(L"Delete Order", L"Удалить заказ").c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES) {
            if (!data::Repository::Instance().DeleteOrder(orders_[index].id)) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Delete Order", L"Удалить заказ").c_str(), MB_OK | MB_ICONWARNING);
            }
            ReloadData();
        }
        return 0;
    }

    if ((id == IDC_SEARCH && code == EN_CHANGE) ||
        (id == IDC_STATUS && code == CBN_SELCHANGE)) {
        ReloadData();
    }

    return 0;
}

LRESULT OrdersPage::OnDrawItem(WPARAM, LPARAM lParam) {
    ui::DrawUiButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam), ui::GetButtonKind(reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->hwndItem), false);
    return TRUE;
}

LRESULT OrdersPage::OnNotify(WPARAM wParam, LPARAM lParam) {
    const auto* header = reinterpret_cast<const NMHDR*>(lParam);
    if (header && (header->idFrom == IDC_FROM || header->idFrom == IDC_TO) && header->code == DTN_DATETIMECHANGE) {
        ReloadData();
        return 0;
    }
    return PageBase::OnNotify(wParam, lParam);
}
