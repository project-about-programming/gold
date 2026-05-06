#include "ProductsPage.h"

#include "AccessControl.h"
#include "DataEntryDialog.h"
#include "DataRepository.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>
#include <commdlg.h>
#include <commctrl.h>
#include <sstream>

namespace {

enum : int {
    IDC_SEARCH = 5001,
    IDC_CATEGORY,
    IDC_SUPPLIER,
    IDC_STATUS,
    IDC_EXPIRY,
    IDC_ADD,
    IDC_EDIT,
    IDC_REMOVE,
    IDC_IMPORT,
    IDC_EXPORT,
    IDC_TABLE,
    IDC_MOVEMENTS,
    IDC_ALERTS
};

constexpr int IDC_STOCK_IN = 5020;
constexpr int IDC_STOCK_OUT = 5021;
constexpr int IDC_REFRESH = 5022;

std::wstring UiText(const wchar_t* en, const wchar_t*) {
    return en;
}

std::wstring FormatMoney(double value) {
    std::wostringstream stream;
    stream << L"$" << static_cast<long long>(value + (value >= 0 ? 0.5 : -0.5));
    return stream.str();
}

std::wstring FormatPercent(double value) {
    std::wostringstream stream;
    stream.setf(std::ios::fixed);
    stream.precision(1);
    stream << value << L"%";
    return stream.str();
}

int ParseIntText(const std::wstring& value, int fallback = 0) {
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

double ParseDoubleText(const std::wstring& value, double fallback = 0.0) {
    try {
        return std::stod(value);
    } catch (...) {
        return fallback;
    }
}

std::wstring ReadWindowText(HWND hwnd) {
    wchar_t buffer[256]{};
    GetWindowTextW(hwnd, buffer, 256);
    return buffer;
}

std::wstring CanonicalComboValue(const std::wstring& value, const std::wstring& allValue) {
    return value.empty() ? allValue : value;
}

bool Can(access::Permission permission) {
    return access::HasPermission(data::Repository::Instance().CurrentAccount(), permission);
}

void SetActionVisible(HWND hwnd, bool visible) {
    ShowWindow(hwnd, SW_SHOW);
    EnableWindow(hwnd, visible ? TRUE : FALSE);
}

void ResetComboItems(HWND combo, const std::vector<std::wstring>& items) {
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    ui::AddComboItems(combo, items);
}

std::wstring FieldValue(const std::vector<ui::DataEntryField>& fields, size_t index) {
    return index < fields.size() ? fields[index].value : L"";
}

ui::DataEntryField TextField(const std::wstring& label, const std::wstring& placeholder, const std::wstring& value, bool required = true) {
    return {label, placeholder, value, required};
}

ui::DataEntryField NumberField(const std::wstring& label, const std::wstring& placeholder, const std::wstring& value, double minValue = 0.0, bool required = true) {
    ui::DataEntryField field{label, placeholder, value, required};
    field.kind = ui::DataEntryFieldKind::Number;
    field.minValue = minValue;
    field.hasMinValue = true;
    return field;
}

ui::DataEntryField DecimalField(const std::wstring& label, const std::wstring& placeholder, const std::wstring& value, double minValue, double maxValue, bool hasMax, bool required = true) {
    ui::DataEntryField field{label, placeholder, value, required};
    field.kind = ui::DataEntryFieldKind::Decimal;
    field.minValue = minValue;
    field.hasMinValue = true;
    field.maxValue = maxValue;
    field.hasMaxValue = hasMax;
    return field;
}

ui::DataEntryField ComboField(const std::wstring& label, const std::vector<ui::DataEntryOption>& options,
    const std::wstring& value, bool required = true, bool allowCustom = false, bool storeId = false) {
    ui::DataEntryField field{label, L"", value, required};
    field.kind = ui::DataEntryFieldKind::Combo;
    field.options = options;
    field.allowCustomValue = allowCustom;
    field.storeOptionId = storeId;
    return field;
}

ui::DataEntryField DateField(const std::wstring& label, const std::wstring& value, bool required = false) {
    ui::DataEntryField field{label, L"", value, required};
    field.kind = ui::DataEntryFieldKind::Date;
    return field;
}

ui::DataEntryField ReadOnlyField(const std::wstring& label, const std::wstring& value) {
    ui::DataEntryField field{label, L"", value, false};
    field.kind = ui::DataEntryFieldKind::ReadOnly;
    return field;
}

std::vector<ui::DataEntryOption> CategoryOptions(const std::vector<data::CategoryRecord>& categories) {
    std::vector<ui::DataEntryOption> options;
    for (const auto& category : categories) {
        options.push_back({category.id, category.name});
    }
    return options;
}

std::vector<ui::DataEntryOption> SupplierOptions(const std::vector<data::SupplierRecord>& suppliers) {
    std::vector<ui::DataEntryOption> options;
    for (const auto& supplier : suppliers) {
        options.push_back({supplier.id, supplier.name});
    }
    return options;
}

std::vector<ui::DataEntryOption> ProductStatusOptions() {
    return {
        {0, L"Active"},
        {0, L"Discontinued"},
        {0, L"Archived"}
    };
}

std::vector<ui::DataEntryOption> StockInReasons() {
    return {
        {0, L"Purchase"},
        {0, L"Return"},
        {0, L"Correction"},
        {0, L"Opening Balance"},
        {0, L"Other"}
    };
}

std::vector<ui::DataEntryOption> StockOutReasons() {
    return {
        {0, L"Sale"},
        {0, L"Damage"},
        {0, L"Expired"},
        {0, L"Correction"},
        {0, L"Internal Use"},
        {0, L"Other"}
    };
}

void DrawFilterLabel(HDC hdc, const RECT& control, const std::wstring& text) {
    ui::DrawTextLine(
        hdc,
        text,
        ui::MakeRect(control.left, control.top - ui::Scale(22), control.right, control.top - ui::Scale(4)),
        theme::SmallBoldFont(),
        theme::kTextSecondary,
        DT_LEFT | DT_SINGLELINE);
}

struct ProductsLayout {
    RECT title{};
    RECT kpis[6]{};
    RECT filterPanel{};
    RECT search{};
    RECT category{};
    RECT supplier{};
    RECT status{};
    RECT expiry{};
    RECT add{};
    RECT edit{};
    RECT stockIn{};
    RECT stockOut{};
    RECT remove{};
    RECT importButton{};
    RECT exportButton{};
    RECT refreshButton{};
    RECT tablePanel{};
    RECT table{};
    RECT movementPanel{};
    RECT movements{};
    RECT alertsPanel{};
    RECT alerts{};
};

ProductsLayout BuildLayout(const RECT& client) {
    ProductsLayout layout{};
    const auto m = ui::GetMetrics();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);

    const bool wideKpis = ui::Width(area) >= ui::Scale(1480);
    const int kpiHeight = wideKpis ? m.kpiHeight : ui::Scale(118);
    RECT kpiArea = ui::TakeTop(&area, wideKpis ? kpiHeight : (kpiHeight * 2 + m.compactGap), m.sectionGap);
    if (wideKpis) {
        const auto cards = ui::SplitColumns(kpiArea, 6, m.compactGap);
        for (int i = 0; i < 6; ++i) {
            layout.kpis[i] = cards[i];
        }
    } else {
        RECT kpiTop = ui::TakeTop(&kpiArea, kpiHeight, m.compactGap);
        const auto top = ui::SplitColumns(kpiTop, 3, m.compactGap);
        const auto bottom = ui::SplitColumns(kpiArea, 3, m.compactGap);
        for (int i = 0; i < 3; ++i) {
            layout.kpis[i] = top[i];
            layout.kpis[i + 3] = bottom[i];
        }
    }

    layout.filterPanel = ui::TakeTop(&area, ui::Scale(200), m.sectionGap);

    const int available = ui::Height(area);
    const int minLower = ui::Scale(96);
    const int desiredLower = ui::Scale(210);
    const int desiredTable = ui::Scale(230);
    int tableHeight = desiredTable;
    if (available > desiredTable + m.sectionGap + desiredLower) {
        tableHeight = std::min(available - m.sectionGap - minLower, std::max(desiredTable, available - m.sectionGap - desiredLower));
    } else {
        tableHeight = std::max(ui::Scale(180), available - m.sectionGap - minLower);
    }
    tableHeight = std::max(ui::Scale(120), std::min(tableHeight, std::max(0, available - m.sectionGap - ui::Scale(64))));
    layout.tablePanel = ui::TakeTop(&area, tableHeight, m.sectionGap);
    RECT lower = area;
    const auto lowerPanels = ui::SplitColumns(lower, 2, m.sectionGap);
    layout.movementPanel = lowerPanels[0];
    layout.alertsPanel = lowerPanels[1];

    RECT inner = ui::Inset(layout.filterPanel, ui::Scale(14), ui::Scale(12));
    RECT filterRow = ui::MakeRect(inner.left, layout.filterPanel.top + ui::Scale(92), inner.right, layout.filterPanel.top + ui::Scale(92) + m.controlHeight);
    layout.expiry = ui::TakeRight(&filterRow, ui::Scale(148), m.compactGap);
    layout.status = ui::TakeRight(&filterRow, ui::Scale(150), m.compactGap);
    layout.supplier = ui::TakeRight(&filterRow, ui::Scale(180), m.compactGap);
    layout.category = ui::TakeRight(&filterRow, ui::Scale(176), m.compactGap);
    layout.search = filterRow;

    RECT actionRow = ui::MakeRect(inner.left, layout.search.bottom + ui::Scale(16), inner.right, layout.search.bottom + ui::Scale(16) + m.controlHeight);
    int x = actionRow.left;
    auto nextButton = [&](int width) {
        const int right = std::min(static_cast<int>(actionRow.right), x + width);
        RECT rc = ui::MakeRect(x, actionRow.top, right, actionRow.bottom);
        x = rc.right + m.compactGap;
        return rc;
    };
    layout.add = nextButton(ui::Scale(128));
    layout.edit = nextButton(ui::Scale(92));
    layout.stockIn = nextButton(ui::Scale(108));
    layout.stockOut = nextButton(ui::Scale(116));
    layout.remove = nextButton(ui::Scale(98));
    layout.importButton = nextButton(ui::Scale(96));
    layout.exportButton = nextButton(ui::Scale(126));
    layout.refreshButton = nextButton(ui::Scale(96));

    layout.table = ui::Inset(ui::MakeRect(layout.tablePanel.left, layout.tablePanel.top + m.panelHeaderHeight, layout.tablePanel.right, layout.tablePanel.bottom), ui::Scale(12), ui::Scale(12));
    layout.movements = ui::Inset(ui::MakeRect(layout.movementPanel.left, layout.movementPanel.top + m.panelHeaderHeight, layout.movementPanel.right, layout.movementPanel.bottom), ui::Scale(12), ui::Scale(12));
    layout.alerts = ui::Inset(ui::MakeRect(layout.alertsPanel.left, layout.alertsPanel.top + m.panelHeaderHeight, layout.alertsPanel.right, layout.alertsPanel.bottom), ui::Scale(12), ui::Scale(12));
    return layout;
}

} // namespace

ProductsPage::ProductsPage()
    : PageBase(L"Inventory", L"Stock levels, category segmentation and pricing oversight.") {}

const wchar_t* ProductsPage::ClassName() const { return L"NativeProductsPage"; }

void ProductsPage::OnCreate() {
    searchEdit_ = ui::CreateUiEdit(hwnd_, IDC_SEARCH);
    categoryCombo_ = ui::CreateUiCombo(hwnd_, IDC_CATEGORY);
    supplierCombo_ = ui::CreateUiCombo(hwnd_, IDC_SUPPLIER);
    statusCombo_ = ui::CreateUiCombo(hwnd_, IDC_STATUS);
    expiryCombo_ = ui::CreateUiCombo(hwnd_, IDC_EXPIRY);
    addButton_ = ui::CreateUiButton(hwnd_, IDC_ADD, L"Add Product", ui::ButtonKind::Primary);
    editButton_ = ui::CreateUiButton(hwnd_, IDC_EDIT, L"Edit", ui::ButtonKind::Secondary);
    stockInButton_ = ui::CreateUiButton(hwnd_, IDC_STOCK_IN, L"Stock In", ui::ButtonKind::Success);
    stockOutButton_ = ui::CreateUiButton(hwnd_, IDC_STOCK_OUT, L"Stock Out", ui::ButtonKind::Warning);
    removeButton_ = ui::CreateUiButton(hwnd_, IDC_REMOVE, L"Delete", ui::ButtonKind::Danger);
    importButton_ = ui::CreateUiButton(hwnd_, IDC_IMPORT, L"Import", ui::ButtonKind::Secondary);
    exportButton_ = ui::CreateUiButton(hwnd_, IDC_EXPORT, L"Export CSV", ui::ButtonKind::Secondary);
    refreshButton_ = ui::CreateUiButton(hwnd_, IDC_REFRESH, L"Refresh", ui::ButtonKind::Secondary);
    productsTable_ = ui::CreateUiListView(hwnd_, IDC_TABLE);
    movementsTable_ = ui::CreateUiListView(hwnd_, IDC_MOVEMENTS);
    alertsTable_ = ui::CreateUiListView(hwnd_, IDC_ALERTS);

    ui::SetEditCueBanner(searchEdit_, L"Search by SKU, product, category, or supplier");
    ui::AddComboItems(statusCombo_, { L"All Statuses", L"In Stock", L"Low Stock", L"Out of Stock", L"Expired", L"Discontinued" });
    ui::AddComboItems(expiryCombo_, { L"All", L"Valid", L"Expiring Soon", L"Expired" });

    ui::AddListColumns(productsTable_, {
        L"SKU", L"Product Name", L"Category", L"Supplier", L"Stock", L"Reorder Level",
        L"Unit Price", L"Cost", L"Margin", L"Discount", L"Status", L"Expires", L"Actions"
    }, {96, 210, 126, 160, 70, 98, 92, 84, 84, 86, 118, 116, 92});
    ui::AddListColumns(movementsTable_, {
        L"Date / Time", L"SKU", L"Product", L"Type", L"Qty", L"Previous", L"New", L"Reason", L"Reference", L"User"
    }, {132, 92, 190, 92, 64, 74, 74, 128, 110, 100});
    ui::AddListColumns(alertsTable_, {
        L"Alert", L"Product", L"Message", L"Severity", L"Status"
    }, {128, 180, 280, 100, 86});

    ReloadLookups();
    ApplyAccess();
    ReloadData();
}

void ProductsPage::OnActivate() {
    ReloadLookups();
    ApplyAccess();
    ReloadData();
}

void ProductsPage::ApplyAccess() {
    SetActionVisible(addButton_, Can(access::Permission::InventoryCreate));
    SetActionVisible(editButton_, Can(access::Permission::InventoryEdit));
    SetActionVisible(stockInButton_, Can(access::Permission::InventoryStockIn));
    SetActionVisible(stockOutButton_, Can(access::Permission::InventoryStockOut));
    SetActionVisible(removeButton_, Can(access::Permission::InventoryDelete));
    SetActionVisible(importButton_, Can(access::Permission::InventoryExport));
    SetActionVisible(exportButton_, Can(access::Permission::InventoryExport));
}

void ProductsPage::ReloadLookups() {
    categories_ = data::Repository::Instance().LoadCategories();
    suppliers_ = data::Repository::Instance().LoadSuppliers();

    std::vector<std::wstring> categoryItems{ L"All Categories" };
    for (const auto& category : categories_) {
        categoryItems.push_back(category.name);
    }
    ResetComboItems(categoryCombo_, categoryItems);

    std::vector<std::wstring> supplierItems{ L"All Suppliers" };
    for (const auto& supplier : suppliers_) {
        supplierItems.push_back(supplier.name);
    }
    ResetComboItems(supplierCombo_, supplierItems);
}

data::ProductFilter ProductsPage::BuildFilter() const {
    data::ProductFilter filter{};
    filter.search = ReadWindowText(searchEdit_);
    filter.category = CanonicalComboValue(ReadWindowText(categoryCombo_), L"All Categories");
    filter.supplier = CanonicalComboValue(ReadWindowText(supplierCombo_), L"All Suppliers");
    filter.status = CanonicalComboValue(ReadWindowText(statusCombo_), L"All Statuses");
    filter.expiryStatus = CanonicalComboValue(ReadWindowText(expiryCombo_), L"All");
    return filter;
}

void ProductsPage::ReloadData() {
    const int currentIndex = SelectedProductIndex();
    if (currentIndex >= 0 && currentIndex < static_cast<int>(products_.size())) {
        selectedProductId_ = products_[currentIndex].id;
    }

    const auto filter = BuildFilter();
    products_ = data::Repository::Instance().LoadProducts(filter);
    summary_ = data::Repository::Instance().LoadInventorySummary(filter);

    ui::ClearListRows(productsTable_);
    int selectedRow = -1;
    for (size_t index = 0; index < products_.size(); ++index) {
        const auto& product = products_[index];
        if (product.id == selectedProductId_) {
            selectedRow = static_cast<int>(index);
        }
        ui::AddListRow(productsTable_, static_cast<int>(index), {
            product.sku,
            product.name,
            i18n::DataText(product.category),
            i18n::DataText(product.supplier),
            std::to_wstring(product.stock),
            std::to_wstring(product.reorderLevel),
            FormatMoney(product.price),
            FormatMoney(product.cost),
            FormatPercent(product.marginPercent),
            FormatPercent(product.discountPercent),
            i18n::DataText(product.status),
            product.expirationDateDisplay,
            L"View / Edit"
        });
    }
    if (!products_.empty()) {
        if (selectedRow < 0) {
            selectedRow = 0;
            selectedProductId_ = products_.front().id;
        }
        ListView_SetItemState(productsTable_, selectedRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(productsTable_, selectedRow, FALSE);
    } else {
        selectedProductId_ = 0;
    }
    ShowWindow(productsTable_, products_.empty() ? SW_HIDE : SW_SHOW);
    ReloadMovementAndAlerts();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void ProductsPage::ReloadMovementAndAlerts() {
    const int index = SelectedProductIndex();
    const int productId = (index >= 0 && index < static_cast<int>(products_.size())) ? products_[index].id : selectedProductId_;
    movements_ = data::Repository::Instance().LoadStockMovements(productId, 80);
    alerts_ = data::Repository::Instance().LoadInventoryAlerts(true, 80);

    ui::ClearListRows(movementsTable_);
    for (size_t row = 0; row < movements_.size(); ++row) {
        const auto& movement = movements_[row];
        ui::AddListRow(movementsTable_, static_cast<int>(row), {
            movement.createdAt,
            movement.sku,
            movement.product,
            movement.movementType,
            std::to_wstring(movement.quantity),
            std::to_wstring(movement.previousQuantity),
            std::to_wstring(movement.newQuantity),
            movement.reason,
            movement.reference,
            movement.userName
        });
    }
    ui::ClearListRows(alertsTable_);
    for (size_t row = 0; row < alerts_.size(); ++row) {
        const auto& alert = alerts_[row];
        ui::AddListRow(alertsTable_, static_cast<int>(row), {
            alert.alertType,
            alert.product,
            alert.message,
            alert.severity,
            alert.status
        });
    }
    ShowWindow(movementsTable_, movements_.empty() ? SW_HIDE : SW_SHOW);
    ShowWindow(alertsTable_, alerts_.empty() ? SW_HIDE : SW_SHOW);
}

int ProductsPage::SelectedProductIndex() const {
    return ListView_GetNextItem(productsTable_, -1, LVNI_SELECTED);
}

data::ProductRecord ProductsPage::BuildProductFromDialog(const std::vector<ui::DataEntryField>& fields, const data::ProductRecord* existing) const {
    data::ProductRecord product = existing ? *existing : data::ProductRecord{};
    product.sku = FieldValue(fields, 0);
    product.name = FieldValue(fields, 1);
    product.category = FieldValue(fields, 2);
    product.supplier = FieldValue(fields, 3);
    product.stock = ParseIntText(FieldValue(fields, 4), -1);
    product.reorderLevel = ParseIntText(FieldValue(fields, 5), -1);
    product.price = ParseDoubleText(FieldValue(fields, 6), -1.0);
    product.cost = ParseDoubleText(FieldValue(fields, 7), -1.0);
    product.discountPercent = ParseDoubleText(FieldValue(fields, 8), 0.0);
    product.status = FieldValue(fields, 9).empty() ? L"Active" : FieldValue(fields, 9);
    product.expirationDateIso = FieldValue(fields, 10).empty() ? L"2026-12-31" : FieldValue(fields, 10);
    product.description = existing ? existing->description : L"";
    product.batchCode = existing && !existing->batchCode.empty() ? existing->batchCode : L"MANUAL";
    return product;
}

void ProductsPage::OnSize(int width, int height) {
    const ProductsLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    ui::MoveWindowToRect(searchEdit_, layout.search);
    ui::MoveWindowToRect(categoryCombo_, layout.category);
    ui::MoveWindowToRect(supplierCombo_, layout.supplier);
    ui::MoveWindowToRect(statusCombo_, layout.status);
    ui::MoveWindowToRect(expiryCombo_, layout.expiry);
    ui::MoveWindowToRect(addButton_, layout.add);
    ui::MoveWindowToRect(editButton_, layout.edit);
    ui::MoveWindowToRect(stockInButton_, layout.stockIn);
    ui::MoveWindowToRect(stockOutButton_, layout.stockOut);
    ui::MoveWindowToRect(removeButton_, layout.remove);
    ui::MoveWindowToRect(importButton_, layout.importButton);
    ui::MoveWindowToRect(exportButton_, layout.exportButton);
    ui::MoveWindowToRect(refreshButton_, layout.refreshButton);
    ui::MoveWindowToRect(productsTable_, layout.table);
    ui::MoveWindowToRect(movementsTable_, layout.movements);
    ui::MoveWindowToRect(alertsTable_, layout.alerts);

    const int productWidth = std::max(ui::Scale(1320), ui::Width(layout.table) - ui::Scale(4));
    const int productWeights[] = {8, 17, 10, 13, 6, 8, 8, 7, 7, 7, 10, 9, 9};
    const int productTotal = 119;
    int used = 0;
    for (int i = 0; i < 13; ++i) {
        const int widthPart = (i == 12) ? productWidth - used : productWidth * productWeights[i] / productTotal;
        ListView_SetColumnWidth(productsTable_, i, std::max(ui::Scale(i == 1 ? 150 : 70), widthPart));
        used += widthPart;
    }

    const int movementWidth = std::max(ui::Scale(1080), ui::Width(layout.movements) - ui::Scale(4));
    const int movementWeights[] = {14, 9, 18, 10, 7, 8, 7, 13, 9, 5};
    const int movementTotal = 100;
    used = 0;
    for (int i = 0; i < 10; ++i) {
        const int widthPart = (i == 9) ? movementWidth - used : movementWidth * movementWeights[i] / movementTotal;
        ListView_SetColumnWidth(movementsTable_, i, std::max(ui::Scale(62), widthPart));
        used += widthPart;
    }

    const int alertWidth = std::max(ui::Scale(520), ui::Width(layout.alerts) - ui::Scale(4));
    const int alertWeights[] = {18, 25, 37, 10, 10};
    const int alertTotal = 100;
    used = 0;
    for (int i = 0; i < 5; ++i) {
        const int widthPart = (i == 4) ? alertWidth - used : alertWidth * alertWeights[i] / alertTotal;
        ListView_SetColumnWidth(alertsTable_, i, std::max(ui::Scale(72), widthPart));
        used += widthPart;
    }
}

void ProductsPage::OnPaint(HDC hdc, const RECT& client) {
    const ProductsLayout layout = BuildLayout(client);
    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, L"Inventory",
        L"Stock levels, category segmentation and pricing oversight.", ui::Width(layout.title));

    ui::DrawKpiCard(hdc, layout.kpis[0], L"Catalog Items", std::to_wstring(summary_.catalogItems), L"Products in catalog", theme::kBlue);
    ui::DrawKpiCard(hdc, layout.kpis[1], L"Low Stock", std::to_wstring(summary_.lowStock), L"Requires purchasing attention", theme::kAmber);
    ui::DrawKpiCard(hdc, layout.kpis[2], L"Out of Stock", std::to_wstring(summary_.outOfStock), L"Unavailable items", theme::kDanger);
    ui::DrawKpiCard(hdc, layout.kpis[3], L"Stock Value", FormatMoney(summary_.stockValue), L"Inventory value by cost", theme::kAccent);
    ui::DrawKpiCard(hdc, layout.kpis[4], L"Active Items", std::to_wstring(summary_.activeItems), L"Items currently sellable", theme::kGreen);
    ui::DrawKpiCard(hdc, layout.kpis[5], L"Expiring Soon", std::to_wstring(summary_.expiringSoon), L"Expiry watchlist", theme::kAmber);

    ui::DrawRoundedPanel(hdc, layout.filterPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawPanelHeader(hdc, layout.filterPanel, L"Inventory Controls",
        L"Search and filter inventory items by category, supplier, stock status and expiry.");
    DrawFilterLabel(hdc, layout.search, L"Search");
    DrawFilterLabel(hdc, layout.category, L"Category");
    DrawFilterLabel(hdc, layout.supplier, L"Supplier");
    DrawFilterLabel(hdc, layout.status, L"Stock Status");
    DrawFilterLabel(hdc, layout.expiry, L"Expiry");

    ui::DrawRoundedPanel(hdc, layout.tablePanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawPanelHeader(hdc, layout.tablePanel, L"Product Grid", L"Catalog, pricing, stock level, margin and expiry control.");
    if (products_.empty()) {
        ui::DrawEmptyState(hdc, layout.table, L"No products in inventory",
            L"Add products to start tracking stock levels, pricing and movement history.", L"\u25A3");
    }

    ui::DrawRoundedPanel(hdc, layout.movementPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawPanelHeader(hdc, layout.movementPanel, L"Stock Movement History", L"Stock in and stock out operations for the selected product.");
    if (movements_.empty()) {
        ui::DrawEmptyState(hdc, layout.movements, L"No stock movements yet",
            L"Stock in and stock out operations will appear here.", L"\u21C5");
    }

    ui::DrawRoundedPanel(hdc, layout.alertsPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawPanelHeader(hdc, layout.alertsPanel, L"Inventory Alerts", L"Low stock, out of stock and expiry warnings.");
    if (alerts_.empty()) {
        ui::DrawEmptyState(hdc, layout.alerts, L"No inventory alerts",
            L"Stock levels are currently healthy.", L"\u2713");
    }
}

LRESULT ProductsPage::OnCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);

    if (id == IDC_ADD && code == BN_CLICKED) {
        std::vector<ui::DataEntryField> fields{
            TextField(L"SKU", L"Example: SKU-NEW", L"", true),
            TextField(L"Product Name", L"Example: Barcode Scanner", L"", true),
            ComboField(L"Category", CategoryOptions(categories_), L"Hardware", true, true, false),
            ComboField(L"Supplier", SupplierOptions(suppliers_), L"Unassigned Supplier", false, true, false),
            NumberField(L"Stock Quantity", L"Example: 25", L"0", 0),
            NumberField(L"Reorder Level", L"Example: 10", L"5", 0),
            DecimalField(L"Unit Price", L"Example: 149.99", L"0", 0, 0, false),
            DecimalField(L"Cost Price", L"Example: 90.00", L"0", 0, 0, false),
            DecimalField(L"Discount %", L"0 - 100", L"0", 0, 100, true, false),
            ComboField(L"Product Status", ProductStatusOptions(), L"Active", true, false, false),
            DateField(L"Expiry Date", L"2026-12-31", false)
        };
        if (ui::ShowDataEntryDialog(hwnd_, L"Add Product", L"Create a catalog item with stock, supplier and pricing data.", fields)) {
            const auto product = BuildProductFromDialog(fields, nullptr);
            if (!data::Repository::Instance().SaveProduct(product, true)) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Add Product", MB_OK | MB_ICONWARNING);
            }
            ReloadLookups();
            ReloadData();
        }
        return 0;
    }

    if (id == IDC_EDIT && code == BN_CLICKED) {
        const int index = SelectedProductIndex();
        if (index < 0 || index >= static_cast<int>(products_.size())) {
            MessageBoxW(hwnd_, L"Select a product first.", L"Edit Product", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        const auto& current = products_[index];
        std::vector<ui::DataEntryField> fields{
            TextField(L"SKU", L"Example: SKU-NEW", current.sku, true),
            TextField(L"Product Name", L"Example: Barcode Scanner", current.name, true),
            ComboField(L"Category", CategoryOptions(categories_), current.category, true, true, false),
            ComboField(L"Supplier", SupplierOptions(suppliers_), current.supplier, false, true, false),
            NumberField(L"Stock Quantity", L"Example: 25", std::to_wstring(current.stock), 0),
            NumberField(L"Reorder Level", L"Example: 10", std::to_wstring(current.reorderLevel), 0),
            DecimalField(L"Unit Price", L"Example: 149.99", std::to_wstring(current.price), 0, 0, false),
            DecimalField(L"Cost Price", L"Example: 90.00", std::to_wstring(current.cost), 0, 0, false),
            DecimalField(L"Discount %", L"0 - 100", std::to_wstring(current.discountPercent), 0, 100, true, false),
            ComboField(L"Product Status", ProductStatusOptions(), current.status == L"Discontinued" || current.status == L"Archived" ? current.status : L"Active", true, false, false),
            DateField(L"Expiry Date", current.expirationDateIso, false)
        };
        if (ui::ShowDataEntryDialog(hwnd_, L"Edit Product", L"Update product catalog, stock, supplier and pricing data.", fields)) {
            const auto product = BuildProductFromDialog(fields, &current);
            if (!data::Repository::Instance().SaveProduct(product, false)) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Edit Product", MB_OK | MB_ICONWARNING);
            }
            ReloadLookups();
            ReloadData();
        }
        return 0;
    }

    if ((id == IDC_STOCK_IN || id == IDC_STOCK_OUT) && code == BN_CLICKED) {
        const int index = SelectedProductIndex();
        if (index < 0 || index >= static_cast<int>(products_.size())) {
            MessageBoxW(hwnd_, L"Select a product first.", L"Stock Movement", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        const bool incoming = id == IDC_STOCK_IN;
        const auto& product = products_[index];
        std::vector<ui::DataEntryField> fields{
            ReadOnlyField(L"Product", product.sku + L" | " + product.name),
            ReadOnlyField(L"Current Stock", std::to_wstring(product.stock)),
            NumberField(L"Quantity", L"Example: 10", L"", 1),
            ComboField(L"Reason", incoming ? StockInReasons() : StockOutReasons(), incoming ? L"Purchase" : L"Sale", true, false, false),
            TextField(L"Reference", L"Invoice, order or note number", L"", false)
        };
        const std::wstring title = incoming ? L"Stock In" : L"Stock Out";
        const std::wstring subtitle = product.name + L" | current stock: " + std::to_wstring(product.stock);
        if (ui::ShowDataEntryDialog(hwnd_, title, subtitle, fields)) {
            const int quantity = ParseIntText(FieldValue(fields, 2), 0);
            if (quantity <= 0) {
                data::Repository::Instance().LogAccessDenied(L"inventory", L"Rejected stock movement with non-positive quantity for " + product.name);
                MessageBoxW(hwnd_, L"Quantity must be greater than zero.", title.c_str(), MB_OK | MB_ICONWARNING);
                return 0;
            }
            if (!incoming && quantity > product.stock) {
                data::Repository::Instance().LogAccessDenied(L"inventory", L"Rejected Stock Out over current stock for " + product.name);
                MessageBoxW(hwnd_, L"Stock Out cannot remove more units than currently available.", title.c_str(), MB_OK | MB_ICONWARNING);
                return 0;
            }
            if (!data::Repository::Instance().AdjustProductStock(product.id, incoming ? quantity : -quantity, FieldValue(fields, 3), FieldValue(fields, 4))) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), title.c_str(), MB_OK | MB_ICONWARNING);
            }
            selectedProductId_ = product.id;
            ReloadData();
        }
        return 0;
    }

    if (id == IDC_REMOVE && code == BN_CLICKED) {
        const int index = SelectedProductIndex();
        if (index < 0 || index >= static_cast<int>(products_.size())) {
            MessageBoxW(hwnd_, L"Select a product first.", L"Delete Product", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (MessageBoxW(hwnd_, L"Delete the selected product? Linked orders, sales, alerts and movement history will also be removed.", L"Delete Product", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            if (!data::Repository::Instance().RemoveProduct(products_[index].id)) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Delete Product", MB_OK | MB_ICONWARNING);
            }
            selectedProductId_ = 0;
            ReloadData();
        }
        return 0;
    }

    if (id == IDC_IMPORT && code == BN_CLICKED) {
        wchar_t fileName[MAX_PATH]{};
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd_;
        ofn.lpstrFilter = L"CSV files (*.csv)\0*.csv\0All files (*.*)\0*.*\0";
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        ofn.lpstrTitle = L"Import products from CSV";
        if (GetOpenFileNameW(&ofn)) {
            int imported = 0;
            if (!data::Repository::Instance().ImportProductsCsv(fileName, &imported)) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Import CSV", MB_OK | MB_ICONWARNING);
            } else {
                MessageBoxW(hwnd_, (L"Imported products: " + std::to_wstring(imported)).c_str(), L"Import CSV", MB_OK | MB_ICONINFORMATION);
            }
            ReloadLookups();
            ReloadData();
        }
        return 0;
    }

    if (id == IDC_EXPORT && code == BN_CLICKED) {
        const std::wstring path = data::Repository::Instance().ExportInventoryCsv(BuildFilter());
        if (path.empty()) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Export CSV", MB_OK | MB_ICONWARNING);
        } else {
            MessageBoxW(hwnd_, (L"Inventory exported:\n" + path).c_str(), L"Export CSV", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }

    if (id == IDC_REFRESH && code == BN_CLICKED) {
        ReloadLookups();
        ReloadData();
        return 0;
    }

    if ((id == IDC_SEARCH && code == EN_CHANGE) ||
        ((id == IDC_CATEGORY || id == IDC_SUPPLIER || id == IDC_STATUS || id == IDC_EXPIRY) && code == CBN_SELCHANGE)) {
        ReloadData();
        return 0;
    }

    return 0;
}

LRESULT ProductsPage::OnDrawItem(WPARAM, LPARAM lParam) {
    ui::DrawUiButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam), ui::GetButtonKind(reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->hwndItem), false);
    return TRUE;
}

LRESULT ProductsPage::OnNotify(WPARAM wParam, LPARAM lParam) {
    const auto* header = reinterpret_cast<const NMHDR*>(lParam);
    if (header && header->hwndFrom == productsTable_ && header->code == LVN_ITEMCHANGED) {
        const auto* item = reinterpret_cast<const NMLISTVIEW*>(lParam);
        if ((item->uChanged & LVIF_STATE) && (item->uNewState & LVIS_SELECTED)) {
            if (item->iItem >= 0 && item->iItem < static_cast<int>(products_.size())) {
                selectedProductId_ = products_[item->iItem].id;
            }
            ReloadMovementAndAlerts();
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
    }
    return PageBase::OnNotify(wParam, lParam);
}
