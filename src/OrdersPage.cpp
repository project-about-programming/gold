#include "OrdersPage.h"

#include "AccessControl.h"
#include "DataRepository.h"
#include "DataEntryDialog.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <sstream>

namespace {
enum : int {
    IDC_SEARCH = 4001,
    IDC_STATUS,
    IDC_PAYMENT,
    IDC_CUSTOMER,
    IDC_FROM,
    IDC_TO,
    IDC_ADD,
    IDC_EDIT,
    IDC_ADVANCE,
    IDC_CANCEL,
    IDC_DELETE,
    IDC_REFRESH,
    IDC_EXPORT,
    IDC_TABLE
};

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

struct OrdersLayout {
    RECT title{};
    RECT kpis[6]{};
    RECT filterPanel{};
    RECT search{};
    RECT status{};
    RECT payment{};
    RECT customer{};
    RECT from{};
    RECT to{};
    RECT add{};
    RECT edit{};
    RECT advance{};
    RECT cancel{};
    RECT remove{};
    RECT refresh{};
    RECT exportButton{};
    RECT tablePanel{};
    RECT table{};
};

OrdersLayout BuildLayout(const RECT& client) {
    OrdersLayout layout{};
    const auto m = ui::GetMetrics();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);

    RECT kpiArea = ui::TakeTop(&area, (m.kpiHeight * 2) + m.sectionGap, m.sectionGap);
    RECT firstKpiRow = ui::TakeTop(&kpiArea, m.kpiHeight, m.sectionGap);
    RECT secondKpiRow = ui::TakeTop(&kpiArea, m.kpiHeight, 0);
    const auto firstKpis = ui::SplitColumns(firstKpiRow, 3, m.sectionGap);
    const auto secondKpis = ui::SplitColumns(secondKpiRow, 3, m.sectionGap);
    for (int i = 0; i < 3; ++i) {
        layout.kpis[i] = firstKpis[i];
        layout.kpis[i + 3] = secondKpis[i];
    }

    layout.filterPanel = ui::TakeTop(&area, ui::Scale(150), m.sectionGap);
    layout.tablePanel = area;

    RECT filterInner = ui::Inset(layout.filterPanel, ui::Scale(14), ui::Scale(12));
    RECT filterRow = ui::MakeRect(filterInner.left, layout.filterPanel.top + ui::Scale(44), filterInner.right, layout.filterPanel.top + ui::Scale(44) + m.controlHeight);
    layout.customer = ui::TakeRight(&filterRow, ui::Scale(170), m.compactGap);
    layout.payment = ui::TakeRight(&filterRow, ui::Scale(150), m.compactGap);
    layout.status = ui::TakeRight(&filterRow, ui::Scale(150), m.compactGap);
    layout.to = ui::TakeRight(&filterRow, ui::Scale(144), m.compactGap);
    layout.from = ui::TakeRight(&filterRow, ui::Scale(144), m.compactGap);
    layout.search = filterRow;

    RECT actionRow = ui::MakeRect(filterInner.left, layout.search.bottom + ui::Scale(14), filterInner.right, layout.search.bottom + ui::Scale(14) + ui::Scale(36));
    const int actionWidth = ui::IconButtonWidth();
    layout.exportButton = ui::TakeRight(&actionRow, actionWidth, m.compactGap);
    layout.refresh = ui::TakeRight(&actionRow, actionWidth, m.compactGap);
    layout.remove = ui::TakeRight(&actionRow, actionWidth, m.compactGap);
    layout.cancel = ui::TakeRight(&actionRow, actionWidth, m.compactGap);
    layout.advance = ui::TakeRight(&actionRow, actionWidth, m.compactGap);
    layout.edit = ui::TakeRight(&actionRow, actionWidth, m.compactGap);
    layout.add = ui::TakeRight(&actionRow, actionWidth, m.compactGap);

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

int ParseIntText(const std::wstring& value, int fallback = 0) {
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

ui::DataEntryField ComboField(const std::wstring& label, const std::vector<ui::DataEntryOption>& options,
    const std::wstring& value = L"", bool required = true, bool storeId = true) {
    ui::DataEntryField field{label, L"", value, required};
    field.kind = ui::DataEntryFieldKind::Combo;
    field.options = options;
    field.storeOptionId = storeId;
    field.allowCustomValue = false;
    return field;
}

ui::DataEntryField NumberField(const std::wstring& label, const std::wstring& placeholder, const std::wstring& value, double minValue = 0.0) {
    ui::DataEntryField field{label, placeholder, value, true};
    field.kind = ui::DataEntryFieldKind::Number;
    field.minValue = minValue;
    field.hasMinValue = true;
    return field;
}

ui::DataEntryField DecimalField(const std::wstring& label, const std::wstring& placeholder, const std::wstring& value, double minValue, double maxValue, bool hasMax, bool required = false) {
    ui::DataEntryField field{label, placeholder, value, required};
    field.kind = ui::DataEntryFieldKind::Decimal;
    field.minValue = minValue;
    field.hasMinValue = true;
    field.maxValue = maxValue;
    field.hasMaxValue = hasMax;
    return field;
}

ui::DataEntryField TextField(const std::wstring& label, const std::wstring& placeholder, const std::wstring& value, bool required = true) {
    return {label, placeholder, value, required};
}

std::vector<ui::DataEntryOption> CustomerOptions(const std::vector<data::ClientRecord>& clients) {
    std::vector<ui::DataEntryOption> options;
    for (const auto& client : clients) {
        std::wstring text = client.company;
        if (!client.phone.empty() || !client.email.empty()) {
            text += L" | " + (!client.phone.empty() ? client.phone : client.email);
        }
        options.push_back({client.id, text});
    }
    return options;
}

std::vector<ui::DataEntryOption> ProductOptions(const std::vector<data::ProductRecord>& products) {
    std::vector<ui::DataEntryOption> options;
    for (const auto& product : products) {
        std::wostringstream text;
        text << product.sku << L" | " << product.name << L" | Stock: " << product.stock << L" | $" << static_cast<long long>(product.price + 0.5);
        options.push_back({product.id, text.str()});
    }
    return options;
}

std::vector<ui::DataEntryOption> PaymentMethodOptions() {
    return {
        {0, L"Cash"},
        {0, L"Card"},
        {0, L"Bank Transfer"},
        {0, L"Online"},
        {0, L"Mixed"}
    };
}

const data::ProductRecord* FindProduct(const std::vector<data::ProductRecord>& products, int id) {
    for (const auto& product : products) {
        if (product.id == id) {
            return &product;
        }
    }
    return nullptr;
}

} // namespace

OrdersPage::OrdersPage() : PageBase(L"Order Operations", L"Order lifecycle control, delivery statuses, payment state and search filters.") {}

const wchar_t* OrdersPage::ClassName() const { return L"NativeOrdersPage"; }

void OrdersPage::OnCreate() {
    searchEdit_ = ui::CreateUiEdit(hwnd_, IDC_SEARCH);
    statusCombo_ = ui::CreateUiCombo(hwnd_, IDC_STATUS);
    paymentCombo_ = ui::CreateUiCombo(hwnd_, IDC_PAYMENT);
    customerCombo_ = ui::CreateUiCombo(hwnd_, IDC_CUSTOMER);
    fromDate_ = ui::CreateUiDatePicker(hwnd_, IDC_FROM);
    toDate_ = ui::CreateUiDatePicker(hwnd_, IDC_TO);
    addButton_ = ui::CreateUiButton(hwnd_, IDC_ADD, L"New Order", ui::ButtonKind::Primary);
    editButton_ = ui::CreateUiButton(hwnd_, IDC_EDIT, L"Edit Order", ui::ButtonKind::Secondary);
    advanceButton_ = ui::CreateUiButton(hwnd_, IDC_ADVANCE, L"Advance", ui::ButtonKind::Success);
    cancelButton_ = ui::CreateUiButton(hwnd_, IDC_CANCEL, L"Cancel", ui::ButtonKind::Warning);
    deleteButton_ = ui::CreateUiButton(hwnd_, IDC_DELETE, L"Delete", ui::ButtonKind::Danger);
    refreshButton_ = ui::CreateUiButton(hwnd_, IDC_REFRESH, L"Refresh", ui::ButtonKind::Secondary);
    exportButton_ = ui::CreateUiButton(hwnd_, IDC_EXPORT, L"Export CSV", ui::ButtonKind::Primary);
    ordersTable_ = ui::CreateUiListView(hwnd_, IDC_TABLE);

    ui::SetEditCueBanner(searchEdit_, UiText(L"Search by order id, client, or product", L"Поиск по заказу, клиенту или товару"));
    ui::AddComboItems(statusCombo_, {L"All Statuses", L"Draft", L"Pending", L"Confirmed", L"Packed", L"Shipped", L"Delivered", L"Completed", L"Cancelled", L"Refunded"});
    ui::AddComboItems(paymentCombo_, {L"All Payments", L"Unpaid", L"Partial", L"Paid", L"Refunded"});
    std::vector<std::wstring> customers{L"All Customers"};
    data::ClientFilter clientFilter{};
    for (const auto& client : data::Repository::Instance().LoadClients(clientFilter)) {
        customers.push_back(client.company);
    }
    ui::AddComboItems(customerCombo_, customers);
    ui::AddListColumns(ordersTable_, {UiText(L"Order ID", L"Заказ"), UiText(L"Client Name", L"Клиент"), UiText(L"Product", L"Товар"), UiText(L"Qty", L"Кол-во"), UiText(L"Total Price", L"Сумма"), UiText(L"Date", L"Дата"), UiText(L"Status", L"Статус"), UiText(L"Priority", L"Приоритет")},
        {100, 170, 220, 70, 110, 110, 110, 90});
    for (int column = Header_GetItemCount(ListView_GetHeader(ordersTable_)) - 1; column >= 0; --column) {
        ListView_DeleteColumn(ordersTable_, column);
    }
    ui::AddListColumns(ordersTable_, {UiText(L"Order ID", L"Р—Р°РєР°Р·"), UiText(L"Customer", L"РљР»РёРµРЅС‚"), UiText(L"Date", L"Р”Р°С‚Р°"), UiText(L"Items", L"РџРѕР·РёС†РёРё"), UiText(L"Total", L"РЎСѓРјРјР°"), UiText(L"Payment", L"РћРїР»Р°С‚Р°"), UiText(L"Status", L"РЎС‚Р°С‚СѓСЃ"), UiText(L"Owner", L"РћС‚РІРµС‚СЃС‚РІРµРЅРЅС‹Р№"), UiText(L"Actions", L"Р”РµР№СЃС‚РІРёСЏ")},
        {100, 170, 120, 210, 110, 120, 120, 150, 120});
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
    SetActionVisible(editButton_, Can(access::Permission::OrdersEdit));
    SetActionVisible(advanceButton_, Can(access::Permission::ChangeOrderStatus));
    SetActionVisible(cancelButton_, Can(access::Permission::ChangeOrderStatus));
    SetActionVisible(deleteButton_, Can(access::Permission::DeleteOrder));
    SetActionVisible(exportButton_, Can(access::Permission::OrdersExport));
}

data::OrderFilter OrdersPage::BuildFilter() const {
    data::OrderFilter filter{};
    wchar_t buffer[256]{};
    GetWindowTextW(searchEdit_, buffer, 256);
    filter.search = buffer;
    GetWindowTextW(statusCombo_, buffer, 256);
    filter.status = buffer;
    GetWindowTextW(paymentCombo_, buffer, 256);
    filter.paymentStatus = buffer;
    GetWindowTextW(customerCombo_, buffer, 256);
    filter.customer = (std::wstring(buffer) == L"All Customers") ? L"" : std::wstring(buffer);
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
            row.dateDisplay,
            row.product,
            FormatMoney(row.totalPrice),
            i18n::DataText(row.paymentStatus),
            i18n::DataText(row.status),
            row.owner,
            UiText(L"View / Edit", L"РџСЂРѕСЃРјРѕС‚СЂ / РїСЂР°РІРєР°")
        });
    }
    ShowWindow(ordersTable_, orders_.empty() ? SW_HIDE : SW_SHOW);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void OrdersPage::OnSize(int width, int height) {
    const OrdersLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    ui::MoveWindowToRect(searchEdit_, layout.search);
    ui::MoveWindowToRect(statusCombo_, layout.status);
    ui::MoveWindowToRect(paymentCombo_, layout.payment);
    ui::MoveWindowToRect(customerCombo_, layout.customer);
    ui::MoveWindowToRect(fromDate_, layout.from);
    ui::MoveWindowToRect(toDate_, layout.to);
    ui::MoveWindowToRect(addButton_, layout.add);
    ui::MoveWindowToRect(editButton_, layout.edit);
    ui::MoveWindowToRect(advanceButton_, layout.advance);
    ui::MoveWindowToRect(cancelButton_, layout.cancel);
    ui::MoveWindowToRect(deleteButton_, layout.remove);
    ui::MoveWindowToRect(refreshButton_, layout.refresh);
    ui::MoveWindowToRect(exportButton_, layout.exportButton);
    ui::MoveWindowToRect(ordersTable_, layout.table);

    const int tableWidth = std::max(ui::Scale(980), ui::Width(layout.table) - ui::Scale(4));
    int widths[9]{
        ui::Scale(100), ui::Scale(170), ui::Scale(120), ui::Scale(210), ui::Scale(110),
        ui::Scale(120), ui::Scale(120), ui::Scale(150), ui::Scale(120)
    };
    int used = 0;
    for (int columnWidth : widths) {
        used += columnWidth;
    }
    if (tableWidth > used) {
        widths[3] += tableWidth - used;
    }
    for (int i = 0; i < 9; ++i) {
        ListView_SetColumnWidth(ordersTable_, i, widths[i]);
    }
}

void OrdersPage::OnPaint(HDC hdc, const RECT& client) {
    const OrdersLayout layout = BuildLayout(client);
    int processing = 0;
    int delivered = 0;
    int highPriority = 0;
    int unpaid = 0;
    double revenue = 0.0;
    for (const auto& order : orders_) {
        processing += (order.status == L"Pending" || order.status == L"Confirmed" || order.status == L"Packed" || order.status == L"Shipped") ? 1 : 0;
        delivered += (order.status == L"Delivered" || order.status == L"Completed") ? 1 : 0;
        highPriority += order.priority == L"High" ? 1 : 0;
        unpaid += (order.paymentStatus == L"Unpaid" || order.paymentStatus == L"Partial") ? 1 : 0;
        revenue += order.totalPrice;
    }

    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"Orders", L"Заказы"),
        UiText(L"Track customer orders, statuses and payment state.", L"Контроль заказов, статусов и оплаты."),
        ui::Width(layout.title));

    ui::DrawKpiCard(hdc, layout.kpis[0], UiText(L"Visible Orders", L"Видимые заказы"), std::to_wstring(orders_.size()), UiText(L"Current filter result", L"Текущий результат фильтра"), theme::kBlue);
    ui::DrawKpiCard(hdc, layout.kpis[1], UiText(L"In Fulfillment", L"В обработке"), std::to_wstring(processing), UiText(L"Processing, confirmed, and packed", L"В обработке, подтверждено и упаковано"), theme::kGreen);
    ui::DrawKpiCard(hdc, layout.kpis[2], UiText(L"Delivered", L"Доставлено"), std::to_wstring(delivered), UiText(L"Completed deliveries in current filters", L"Завершённые доставки по текущим фильтрам"), theme::kAmber);
    ui::DrawKpiCard(hdc, layout.kpis[3], UiText(L"Total Value", L"Общая сумма"), FormatMoney(revenue), std::to_wstring(highPriority) + UiText(L" high-priority orders", L" приоритетных заказов"), theme::kAccent);

    ui::DrawKpiCard(hdc, layout.kpis[4], UiText(L"High Priority", L"High Priority"), std::to_wstring(highPriority), UiText(L"Orders needing attention", L"Orders needing attention"), theme::kWarning);
    ui::DrawKpiCard(hdc, layout.kpis[5], UiText(L"Unpaid Orders", L"Unpaid Orders"), std::to_wstring(unpaid), UiText(L"Unpaid or partially paid", L"Unpaid or partially paid"), theme::kDanger);

    ui::DrawRoundedPanel(hdc, layout.filterPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    DrawFilterLabel(hdc, layout.search, UiText(L"Search", L"Поиск"));
    DrawFilterLabel(hdc, layout.status, UiText(L"Status", L"Статус"));
    DrawFilterLabel(hdc, layout.from, UiText(L"From", L"С"));
    DrawFilterLabel(hdc, layout.to, UiText(L"To", L"По"));

    DrawFilterLabel(hdc, layout.payment, UiText(L"Payment", L"Payment"));
    DrawFilterLabel(hdc, layout.customer, UiText(L"Customer", L"Customer"));

    ui::DrawRoundedPanel(hdc, layout.tablePanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.tablePanel.left + ui::Scale(16), layout.tablePanel.top + ui::Scale(14), UiText(L"Orders Table", L"Таблица заказов"),
        UiText(L"Destructive and operational actions are separated so the flow feels deliberate and safer.", L"Операционные и опасные действия разведены, чтобы работа была безопаснее."),
        ui::Width(layout.tablePanel) - ui::Scale(32));
    if (orders_.empty()) {
        ui::DrawEmptyState(hdc, layout.table, UiText(L"No orders yet", L"Заказов пока нет"),
            UiText(L"Create a new order to start tracking customer activity.", L"Создайте заказ, чтобы начать отслеживать активность клиентов."), L"#");
    }
}

LRESULT OrdersPage::OnCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);

    if (id == IDC_ADD && code == BN_CLICKED) {
        data::ClientFilter clientFilter{};
        const auto clients = data::Repository::Instance().LoadClients(clientFilter);
        data::ProductFilter productFilter{};
        const auto products = data::Repository::Instance().LoadProducts(productFilter);
        std::vector<ui::DataEntryField> fields{
            ComboField(UiText(L"Customer", L"Customer"), CustomerOptions(clients)),
            ComboField(UiText(L"Product", L"Product"), ProductOptions(products)),
            NumberField(UiText(L"Quantity", L"Quantity"), UiText(L"Example: 3", L"Example: 3"), L"", 1),
            DecimalField(UiText(L"Discount %", L"Discount %"), UiText(L"Optional, 0-100", L"Optional, 0-100"), L"0", 0, 100, true),
            ComboField(UiText(L"Payment Method", L"Payment Method"), PaymentMethodOptions(), L"Cash", false, false),
            DecimalField(UiText(L"Payment Amount", L"Payment Amount"), UiText(L"Leave empty for unpaid order", L"Leave empty for unpaid order"), L"0", 0, 0, false, false),
            TextField(UiText(L"Notes", L"Notes"), UiText(L"Optional order note", L"Optional order note"), L"", false)
        };
        if (!ui::ShowDataEntryDialog(hwnd_, UiText(L"Add Order", L"Add Order"),
                UiText(L"Create an order from existing customer and inventory records.", L"Create an order from existing customer and inventory records."),
                fields)) {
            return 0;
        }
        const int productId = ParseIntText(fields[1].value, 0);
        const int quantity = ParseIntText(fields[2].value, 0);
        const data::ProductRecord* selectedProduct = FindProduct(products, productId);
        if (selectedProduct && selectedProduct->stock <= 0) {
            MessageBoxW(hwnd_, L"Selected product is out of stock.", L"Add Order", MB_OK | MB_ICONWARNING);
            return 0;
        }
        if (selectedProduct && quantity > selectedProduct->stock) {
            MessageBoxW(hwnd_, L"Quantity cannot exceed available stock.", L"Add Order", MB_OK | MB_ICONWARNING);
            return 0;
        }
        if (!data::Repository::Instance().CreateOrder(fields[0].value, fields[1].value, fields[2].value, fields[3].value, fields[4].value, fields[5].value, fields[6].value)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Add Order", MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }
#if 0
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
        if (!data::Repository::Instance().CreateOrder(fields[0].value, fields[1].value, fields[2].value, L"0", L"Cash", L"0", fields[3].value)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Add Order", L"Добавить заказ").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

#endif
    if (id == IDC_EDIT && code == BN_CLICKED) {
        const int index = SelectedRow(ordersTable_);
        if (index < 0 || index >= static_cast<int>(orders_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select an order first.", L"Select an order first.").c_str(), UiText(L"Edit Order", L"Edit Order").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        const auto& order = orders_[index];
        data::ClientFilter clientFilter{};
        const auto clients = data::Repository::Instance().LoadClients(clientFilter);
        data::ProductFilter productFilter{};
        const auto products = data::Repository::Instance().LoadProducts(productFilter);
        std::vector<ui::DataEntryField> fields{
            ComboField(UiText(L"Customer", L"Customer"), CustomerOptions(clients), std::to_wstring(order.customerId)),
            ComboField(UiText(L"Product", L"Product"), ProductOptions(products), std::to_wstring(order.productId)),
            NumberField(UiText(L"Quantity", L"Quantity"), UiText(L"Example: 3", L"Example: 3"), std::to_wstring(std::max(1, order.quantity)), 1),
            DecimalField(UiText(L"Discount %", L"Discount %"), UiText(L"Optional, 0-100", L"Optional, 0-100"), std::to_wstring(order.discountPercent), 0, 100, true),
            ComboField(UiText(L"Payment Method", L"Payment Method"), PaymentMethodOptions(), order.paymentMethod, false, false),
            DecimalField(UiText(L"Payment Amount", L"Payment Amount"), UiText(L"Paid amount", L"Paid amount"), std::to_wstring(order.paymentAmount), 0, 0, false, false),
            TextField(UiText(L"Notes", L"Notes"), UiText(L"Optional order note", L"Optional order note"), order.notes, false)
        };
        if (!ui::ShowDataEntryDialog(hwnd_, UiText(L"Edit Order", L"Edit Order"),
                UiText(L"Update the customer, selected inventory item, payment and notes for this draft order.", L"Update the customer, selected inventory item, payment and notes for this draft order."),
                fields)) {
            return 0;
        }
        const int productId = ParseIntText(fields[1].value, 0);
        const int quantity = ParseIntText(fields[2].value, 0);
        const data::ProductRecord* selectedProduct = FindProduct(products, productId);
        if (selectedProduct && quantity > selectedProduct->stock) {
            MessageBoxW(hwnd_, L"Quantity cannot exceed available stock.", L"Edit Order", MB_OK | MB_ICONWARNING);
            return 0;
        }
        if (!data::Repository::Instance().UpdateOrder(order.id, fields[0].value, fields[1].value, fields[2].value, fields[3].value, fields[4].value, fields[5].value, fields[6].value)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Edit Order", MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_ADVANCE && code == BN_CLICKED) {
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

    if (id == IDC_CANCEL && code == BN_CLICKED) {
        const int index = SelectedRow(ordersTable_);
        if (index < 0 || index >= static_cast<int>(orders_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select an order first.", L"Select an order first.").c_str(), UiText(L"Cancel Order", L"Cancel Order").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (MessageBoxW(hwnd_, UiText(L"Cancel the selected order?", L"Cancel the selected order?").c_str(), UiText(L"Cancel Order", L"Cancel Order").c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES) {
            if (!data::Repository::Instance().CancelOrder(orders_[index].id)) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Cancel Order", L"Cancel Order").c_str(), MB_OK | MB_ICONWARNING);
            }
            ReloadData();
        }
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

    if (id == IDC_REFRESH && code == BN_CLICKED) {
        ReloadData();
        return 0;
    }

    if (id == IDC_EXPORT && code == BN_CLICKED) {
        const std::wstring filePath = data::Repository::Instance().ExportOrdersCsv(BuildFilter());
        if (filePath.empty()) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Export CSV", L"Export CSV").c_str(), MB_OK | MB_ICONWARNING);
        } else {
            MessageBoxW(hwnd_, filePath.c_str(), UiText(L"CSV Export Created", L"CSV Export Created").c_str(), MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    if ((id == IDC_SEARCH && code == EN_CHANGE) ||
        ((id == IDC_STATUS || id == IDC_PAYMENT || id == IDC_CUSTOMER) && code == CBN_SELCHANGE)) {
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
