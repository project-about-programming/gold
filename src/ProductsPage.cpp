#include "ProductsPage.h"

#include "AccessControl.h"
#include "DataRepository.h"
#include "DataEntryDialog.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <commdlg.h>
#include <sstream>

namespace {
enum : int { IDC_SEARCH = 5001, IDC_CATEGORY, IDC_STATUS, IDC_ADD, IDC_EDIT, IDC_REMOVE, IDC_IMPORT, IDC_TABLE };

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

std::wstring CanonicalCategory(const std::wstring& value) {
    if (value == L"Все категории") return L"All Categories";
    if (value == L"Электроника") return L"Electronics";
    if (value == L"Оборудование") return L"Hardware";
    if (value == L"Автоматизация") return L"Automation";
    if (value == L"Связь") return L"Connectivity";
    if (value == L"Логистика") return L"Logistics";
    if (value == L"Аналитика") return L"Analytics";
    return value;
}

std::wstring CanonicalStatus(const std::wstring& value) {
    if (value == L"Все статусы") return L"All Statuses";
    if (value == L"Активен") return L"Active";
    if (value == L"Низкий остаток") return L"Low Stock";
    if (value == L"Снят с продажи") return L"Discontinued";
    return value;
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

struct ProductsLayout {
    RECT title{};
    RECT kpis[4]{};
    RECT filterPanel{};
    RECT search{};
    RECT category{};
    RECT status{};
    RECT add{};
    RECT edit{};
    RECT remove{};
    RECT importButton{};
    RECT tablePanel{};
    RECT table{};
};

ProductsLayout BuildLayout(const RECT& client) {
    ProductsLayout layout{};
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

    RECT inner = ui::Inset(layout.filterPanel, ui::Scale(14), ui::Scale(12));
    const int actionWidth = ui::IconButtonWidth();
    layout.remove = ui::TakeRight(&inner, actionWidth, m.compactGap);
    layout.edit = ui::TakeRight(&inner, actionWidth, m.compactGap);
    layout.add = ui::TakeRight(&inner, actionWidth, m.compactGap);
    layout.importButton = ui::TakeRight(&inner, actionWidth, m.compactGap);
    layout.status = ui::TakeRight(&inner, ui::Scale(152), m.compactGap);
    layout.category = ui::TakeRight(&inner, ui::Scale(176), m.compactGap);
    layout.search = inner;

    layout.search.top = layout.category.top = layout.status.top = layout.add.top = layout.edit.top = layout.remove.top = layout.importButton.top = layout.filterPanel.top + ui::Scale(40);
    layout.search.bottom = layout.category.bottom = layout.status.bottom = layout.search.top + m.controlHeight;
    layout.add.bottom = layout.edit.bottom = layout.remove.bottom = layout.importButton.bottom = layout.search.top + m.buttonHeight;
    layout.table = ui::Inset(ui::MakeRect(layout.tablePanel.left, layout.tablePanel.top + m.panelHeaderHeight, layout.tablePanel.right, layout.tablePanel.bottom), ui::Scale(12), ui::Scale(12));
    return layout;
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

ProductsPage::ProductsPage() : PageBase(L"Product Catalog", L"Stock levels, category segmentation, and pricing oversight for the sales team.") {}

const wchar_t* ProductsPage::ClassName() const { return L"NativeProductsPage"; }

void ProductsPage::OnCreate() {
    searchEdit_ = ui::CreateUiEdit(hwnd_, IDC_SEARCH);
    categoryCombo_ = ui::CreateUiCombo(hwnd_, IDC_CATEGORY);
    statusCombo_ = ui::CreateUiCombo(hwnd_, IDC_STATUS);
    addButton_ = ui::CreateUiButton(hwnd_, IDC_ADD, L"+", ui::ButtonKind::Primary);
    editButton_ = ui::CreateUiButton(hwnd_, IDC_EDIT, L"\u270E", ui::ButtonKind::Secondary);
    removeButton_ = ui::CreateUiButton(hwnd_, IDC_REMOVE, L"\u2715", ui::ButtonKind::Danger);
    importButton_ = ui::CreateUiButton(hwnd_, IDC_IMPORT, L"\u21E7", ui::ButtonKind::Secondary);
    productsTable_ = ui::CreateUiListView(hwnd_, IDC_TABLE);

    ui::SetEditCueBanner(searchEdit_, UiText(L"Search by sku, name, category, or supplier", L"Поиск по SKU, названию, категории или поставщику"));
    ui::AddComboItems(categoryCombo_, {
        UiText(L"All Categories", L"Все категории"),
        UiText(L"Electronics", L"Электроника"),
        UiText(L"Hardware", L"Оборудование"),
        UiText(L"Automation", L"Автоматизация"),
        UiText(L"Connectivity", L"Связь"),
        UiText(L"Logistics", L"Логистика"),
        UiText(L"Analytics", L"Аналитика")
    });
    ui::AddComboItems(statusCombo_, {
        UiText(L"All Statuses", L"Все статусы"),
        UiText(L"Active", L"Активен"),
        UiText(L"Low Stock", L"Низкий остаток"),
        UiText(L"Discontinued", L"Снят с продажи")
    });
    ui::AddListColumns(productsTable_, {
        L"SKU",
        UiText(L"Product Name", L"Название товара"),
        UiText(L"Category", L"Категория"),
        UiText(L"Stock", L"Остаток"),
        UiText(L"Reorder", L"Порог"),
        UiText(L"Price", L"Цена"),
        UiText(L"Cost", L"Cost"),
        UiText(L"Discount", L"Discount"),
        UiText(L"Status", L"Статус"),
        UiText(L"Expires", L"Срок")
    }, {96, 220, 120, 72, 72, 88, 88, 82, 120, 124});

    ApplyAccess();
    ReloadData();
}

void ProductsPage::OnActivate() {
    ApplyAccess();
    ReloadData();
}

void ProductsPage::ApplyAccess() {
    SetActionVisible(addButton_, Can(access::Permission::CreateProduct));
    SetActionVisible(editButton_, Can(access::Permission::EditProduct));
    SetActionVisible(removeButton_, Can(access::Permission::DeleteProduct));
    SetActionVisible(importButton_, Can(access::Permission::ImportExportCatalog));
}

data::ProductFilter ProductsPage::BuildFilter() const {
    data::ProductFilter filter{};
    wchar_t buffer[256]{};
    GetWindowTextW(searchEdit_, buffer, 256);
    filter.search = buffer;
    GetWindowTextW(categoryCombo_, buffer, 256);
    filter.category = CanonicalCategory(buffer);
    GetWindowTextW(statusCombo_, buffer, 256);
    filter.status = CanonicalStatus(buffer);
    return filter;
}

void ProductsPage::ReloadData() {
    products_ = data::Repository::Instance().LoadProducts(BuildFilter());
    ui::ClearListRows(productsTable_);
    for (size_t index = 0; index < products_.size(); ++index) {
        const auto& product = products_[index];
        ui::AddListRow(productsTable_, static_cast<int>(index), {
            product.sku,
            product.name,
            i18n::DataText(product.category),
            std::to_wstring(product.stock),
            std::to_wstring(product.reorderLevel),
            FormatMoney(product.price),
            FormatMoney(product.cost),
            FormatPercent(product.discountPercent),
            i18n::DataText(product.status),
            product.expirationDateDisplay
        });
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

int ProductsPage::SelectedProductIndex() const {
    return ListView_GetNextItem(productsTable_, -1, LVNI_SELECTED);
}

void ProductsPage::OnSize(int width, int height) {
    const ProductsLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    ui::MoveWindowToRect(searchEdit_, layout.search);
    ui::MoveWindowToRect(categoryCombo_, layout.category);
    ui::MoveWindowToRect(statusCombo_, layout.status);
    ui::MoveWindowToRect(addButton_, layout.add);
    ui::MoveWindowToRect(editButton_, layout.edit);
    ui::MoveWindowToRect(removeButton_, layout.remove);
    ui::MoveWindowToRect(importButton_, layout.importButton);
    ui::MoveWindowToRect(productsTable_, layout.table);
}

void ProductsPage::OnPaint(HDC hdc, const RECT& client) {
    const ProductsLayout layout = BuildLayout(client);
    int lowStock = 0;
    int active = 0;
    int stockUnits = 0;
    int discontinued = 0;
    for (const auto& product : products_) {
        lowStock += product.status == L"Low Stock" ? 1 : 0;
        active += product.status == L"Active" ? 1 : 0;
        discontinued += product.status == L"Discontinued" ? 1 : 0;
        stockUnits += product.stock;
    }

    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"Product Catalog", L"Каталог товаров"),
        UiText(L"Live SQL inventory workspace with real product commands, cleaner filters, and a table-first layout.",
               L"Рабочее пространство склада на SQLite с реальными действиями по товарам, ровными фильтрами и табличной подачей."),
        ui::Width(layout.title));

    ui::DrawKpiCard(hdc, layout.kpis[0], UiText(L"Catalog Items", L"Товарных позиций"), std::to_wstring(products_.size()), UiText(L"Rows returned by current filter", L"Строк по текущему фильтру"), theme::kBlue);
    ui::DrawKpiCard(hdc, layout.kpis[1], UiText(L"Low Stock", L"Низкий остаток"), std::to_wstring(lowStock), UiText(L"Requires purchasing attention", L"Требует внимания закупок"), theme::kAmber);
    ui::DrawKpiCard(hdc, layout.kpis[2], UiText(L"Active Items", L"Активные позиции"), std::to_wstring(active), std::to_wstring(discontinued) + UiText(L" discontinued items", L" снято с продажи"), theme::kGreen);
    ui::DrawKpiCard(hdc, layout.kpis[3], UiText(L"Stock Units", L"Единиц на складе"), std::to_wstring(stockUnits), UiText(L"Current inventory volume", L"Текущий объём склада"), theme::kAccent);

    ui::DrawRoundedPanel(hdc, layout.filterPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    DrawFilterLabel(hdc, layout.search, UiText(L"Search", L"Поиск"));
    DrawFilterLabel(hdc, layout.category, UiText(L"Category", L"Категория"));
    DrawFilterLabel(hdc, layout.status, UiText(L"Status", L"Статус"));

    ui::DrawRoundedPanel(hdc, layout.tablePanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.tablePanel.left + ui::Scale(16), layout.tablePanel.top + ui::Scale(14), UiText(L"Product Grid", L"Таблица товаров"),
        UiText(L"Add opens a data-entry window, Edit refreshes the selected record in SQL, and Remove deletes linked product data safely.",
               L"Добавить открывает окно ввода данных, Изменить обновляет выбранную запись в SQL, а Удалить безопасно удаляет связанные данные товара."),
        ui::Width(layout.tablePanel) - ui::Scale(32));
}

LRESULT ProductsPage::OnCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);

    if (id == IDC_ADD && code == BN_CLICKED) {
        std::vector<ui::DataEntryField> fields{
            {UiText(L"SKU", L"Артикул"), UiText(L"Example: PRD-001", L"Например: PRD-001"), L"", true},
            {UiText(L"Product name", L"Название товара"), UiText(L"Example: Barcode Scanner", L"Например: Сканер штрихкодов"), L"", true},
            {UiText(L"Stock amount", L"Остаток"), UiText(L"Example: 25", L"Например: 25"), L"", true},
            {UiText(L"Unit price", L"Цена за единицу"), UiText(L"Example: 149.99", L"Например: 149.99"), L"", true}
        };
        if (!ui::ShowDataEntryDialog(hwnd_, UiText(L"Add Product", L"Добавить товар"),
                UiText(L"Enter product data. SKU must be unique.", L"Введите данные товара. Артикул должен быть уникальным."),
                fields)) {
            return 0;
        }
        if (!data::Repository::Instance().CreateQuickEntry(L"Product", fields[0].value, fields[1].value, fields[2].value, fields[3].value)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Products", L"Товары").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_EDIT && code == BN_CLICKED) {
        const int index = SelectedProductIndex();
        if (index < 0 || index >= static_cast<int>(products_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select a product first.", L"Сначала выберите товар.").c_str(), UiText(L"Edit Product", L"Изменить товар").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (!data::Repository::Instance().UpdateDemoProduct(products_[index].id)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Edit Product", L"Изменить товар").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_REMOVE && code == BN_CLICKED) {
        const int index = SelectedProductIndex();
        if (index < 0 || index >= static_cast<int>(products_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select a product first.", L"Сначала выберите товар.").c_str(), UiText(L"Remove Product", L"Удалить товар").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (MessageBoxW(hwnd_, UiText(L"Delete the selected product? Linked orders and sales for this product will also be removed.", L"Удалить выбранный товар? Связанные заказы и продажи по этому товару тоже будут удалены.").c_str(), UiText(L"Remove Product", L"Удалить товар").c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES) {
            if (!data::Repository::Instance().RemoveProduct(products_[index].id)) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Remove Product", L"Удалить товар").c_str(), MB_OK | MB_ICONWARNING);
            }
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
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Import CSV", L"Import CSV").c_str(), MB_OK | MB_ICONWARNING);
            } else {
                std::wstring message = UiText(L"Imported products: ", L"Imported products: ") + std::to_wstring(imported);
                MessageBoxW(hwnd_, message.c_str(), UiText(L"Import CSV", L"Import CSV").c_str(), MB_OK | MB_ICONINFORMATION);
            }
            ReloadData();
        }
        return 0;
    }

    if ((id == IDC_SEARCH && code == EN_CHANGE) ||
        ((id == IDC_CATEGORY || id == IDC_STATUS) && code == CBN_SELCHANGE)) {
        ReloadData();
        return 0;
    }

    return 0;
}

LRESULT ProductsPage::OnDrawItem(WPARAM, LPARAM lParam) {
    ui::DrawUiButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam), ui::GetButtonKind(reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->hwndItem), false);
    return TRUE;
}
