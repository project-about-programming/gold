#include "ReportsPage.h"

#include "AccessControl.h"
#include "DataRepository.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>

namespace {
enum : int { IDC_DAILY = 8001, IDC_FULFILL, IDC_INVENTORY, IDC_EMPLOYEE, IDC_CLIENT, IDC_PROFIT, IDC_CANCEL, IDC_AUDIT, IDC_CSV, IDC_PDF, IDC_ARCHIVE, IDC_EXPIRY_TABLE };

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

struct ReportsLayout {
    RECT title{};
    RECT cards[4]{};
    RECT actionsPanel{};
    RECT expiryTable{};
    RECT dailyButton{};
    RECT fulfillButton{};
    RECT inventoryButton{};
    RECT employeeButton{};
    RECT clientButton{};
    RECT profitButton{};
    RECT cancelButton{};
    RECT auditButton{};
    RECT csvButton{};
    RECT pdfButton{};
    RECT archiveButton{};
};

ReportsLayout BuildLayout(const RECT& client) {
    ReportsLayout layout{};
    const auto m = ui::GetMetrics();
    const int actionWidth = ui::IconButtonWidth();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);
    RECT cardsRow = ui::TakeTop(&area, std::clamp(ui::Height(area) * 34 / 100, ui::Scale(168), ui::Scale(220)), m.sectionGap);
    const auto cards = ui::SplitColumns(cardsRow, 4, m.sectionGap);
    for (int index = 0; index < 4; ++index) {
        layout.cards[index] = cards[index];
    }
    layout.actionsPanel = area;

    layout.dailyButton = ui::MakeRect(layout.cards[0].left + ui::Scale(18), layout.cards[0].bottom - ui::Scale(54), layout.cards[0].left + ui::Scale(18) + actionWidth, layout.cards[0].bottom - ui::Scale(16));
    layout.fulfillButton = ui::MakeRect(layout.cards[1].left + ui::Scale(18), layout.cards[1].bottom - ui::Scale(54), layout.cards[1].left + ui::Scale(18) + actionWidth, layout.cards[1].bottom - ui::Scale(16));
    layout.inventoryButton = ui::MakeRect(layout.cards[2].left + ui::Scale(18), layout.cards[2].bottom - ui::Scale(54), layout.cards[2].left + ui::Scale(18) + actionWidth, layout.cards[2].bottom - ui::Scale(16));
    layout.employeeButton = ui::MakeRect(layout.cards[3].left + ui::Scale(18), layout.cards[3].bottom - ui::Scale(54), layout.cards[3].left + ui::Scale(18) + actionWidth, layout.cards[3].bottom - ui::Scale(16));

    RECT actionInner = ui::Inset(layout.actionsPanel, ui::Scale(18), ui::Scale(18));
    layout.archiveButton = ui::TakeRight(&actionInner, actionWidth, m.compactGap);
    layout.pdfButton = ui::TakeRight(&actionInner, actionWidth, m.compactGap);
    layout.csvButton = ui::TakeRight(&actionInner, actionWidth, m.compactGap);
    layout.cancelButton = ui::TakeRight(&actionInner, actionWidth, m.compactGap);
    layout.profitButton = ui::TakeRight(&actionInner, actionWidth, m.compactGap);
    layout.clientButton = ui::TakeRight(&actionInner, actionWidth, m.compactGap);
    layout.clientButton.top = layout.profitButton.top = layout.cancelButton.top = layout.csvButton.top = layout.pdfButton.top = layout.archiveButton.top = layout.actionsPanel.top + ui::Scale(18);
    layout.clientButton.bottom = layout.profitButton.bottom = layout.cancelButton.bottom = layout.csvButton.bottom = layout.pdfButton.bottom = layout.archiveButton.bottom = layout.actionsPanel.top + ui::Scale(18) + m.buttonHeight;
    layout.expiryTable = ui::MakeRect(layout.actionsPanel.left + ui::Scale(16), layout.actionsPanel.top + ui::Scale(94), layout.actionsPanel.right - ui::Scale(16), layout.actionsPanel.bottom - ui::Scale(16));
    return layout;
}

void DrawMeta(HDC hdc, const RECT& card, const std::wstring& meta) {
    ui::DrawTextLine(hdc, meta,
        ui::MakeRect(card.left + ui::Scale(18), card.top + ui::Scale(110), card.right - ui::Scale(18), card.top + ui::Scale(132)),
        theme::SmallBoldFont(), theme::kAccent, DT_LEFT | DT_SINGLELINE);
}

void DrawReportCard(HDC hdc, const RECT& card, const std::wstring& title, const std::wstring& description, const std::wstring& meta) {
    ui::DrawRoundedPanel(hdc, card, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawTextLine(hdc, title, ui::MakeRect(card.left + ui::Scale(18), card.top + ui::Scale(18), card.right - ui::Scale(18), card.top + ui::Scale(44)),
        theme::BodyBoldFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    ui::DrawTextLine(hdc, description, ui::MakeRect(card.left + ui::Scale(18), card.top + ui::Scale(48), card.right - ui::Scale(18), card.top + ui::Scale(108)),
        theme::SmallFont(), theme::kTextSecondary, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);
    DrawMeta(hdc, card, meta);
}

void ShowExportResult(HWND hwnd, const std::wstring& title, const std::wstring& prefix, const std::wstring& path) {
    if (path.empty()) {
        MessageBoxW(hwnd, data::Repository::Instance().LastError().c_str(), title.c_str(), MB_OK | MB_ICONWARNING);
    } else {
        MessageBoxW(hwnd, (prefix + path).c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
    }
}

bool Can(access::Permission permission) {
    return access::HasPermission(data::Repository::Instance().CurrentAccount(), permission);
}

void SetActionVisible(HWND hwnd, bool visible) {
    ShowWindow(hwnd, SW_SHOW);
    EnableWindow(hwnd, visible ? TRUE : FALSE);
}

} // namespace

ReportsPage::ReportsPage() : PageBase(L"Reporting Center", L"Scheduled reporting templates, export actions, and archive workflows backed by SQLite.") {}

const wchar_t* ReportsPage::ClassName() const { return L"NativeReportsPage"; }

void ReportsPage::OnCreate() {
    dailyButton_ = ui::CreateUiButton(hwnd_, IDC_DAILY, L"Generate", ui::ButtonKind::Primary);
    fulfillButton_ = ui::CreateUiButton(hwnd_, IDC_FULFILL, L"Generate", ui::ButtonKind::Primary);
    inventoryButton_ = ui::CreateUiButton(hwnd_, IDC_INVENTORY, L"Refresh", ui::ButtonKind::Primary);
    employeeButton_ = ui::CreateUiButton(hwnd_, IDC_EMPLOYEE, L"Generate", ui::ButtonKind::Primary);
    clientButton_ = ui::CreateUiButton(hwnd_, IDC_CLIENT, L"Customer", ui::ButtonKind::Secondary);
    profitButton_ = ui::CreateUiButton(hwnd_, IDC_PROFIT, L"Profit", ui::ButtonKind::Secondary);
    cancelButton_ = ui::CreateUiButton(hwnd_, IDC_CANCEL, L"Returns", ui::ButtonKind::Secondary);
    auditButton_ = ui::CreateUiButton(hwnd_, IDC_AUDIT, L"Audit", ui::ButtonKind::Secondary);
    csvButton_ = ui::CreateUiButton(hwnd_, IDC_CSV, L"Export CSV", ui::ButtonKind::Primary);
    pdfButton_ = ui::CreateUiButton(hwnd_, IDC_PDF, L"Export PDF", ui::ButtonKind::Secondary);
    archiveButton_ = ui::CreateUiButton(hwnd_, IDC_ARCHIVE, L"Archive", ui::ButtonKind::Secondary);
    expiryTable_ = ui::CreateUiListView(hwnd_, IDC_EXPIRY_TABLE);
    ui::AddListColumns(expiryTable_,
        {L"SKU", UiText(L"Product", L"Товар"), UiText(L"Category", L"Категория"), UiText(L"Expires", L"Срок"), UiText(L"Days", L"Дни"), UiText(L"Risk", L"Риск"), UiText(L"Supplier", L"Поставщик"), UiText(L"Batch", L"Партия")},
        {92, 220, 130, 126, 70, 90, 180, 120});
    ApplyAccess();
    ReloadData();
}

void ReportsPage::OnActivate() {
    ApplyAccess();
    ReloadData();
}

void ReportsPage::ApplyAccess() {
    const bool reports = Can(access::Permission::ExportReports);
    const bool catalog = Can(access::Permission::ImportExportCatalog);
    SetActionVisible(dailyButton_, reports);
    SetActionVisible(fulfillButton_, reports);
    SetActionVisible(employeeButton_, Can(access::Permission::ViewEmployeePerformance));
    SetActionVisible(clientButton_, reports);
    SetActionVisible(profitButton_, reports);
    SetActionVisible(cancelButton_, reports);
    SetActionVisible(pdfButton_, reports);
    SetActionVisible(archiveButton_, reports);
    SetActionVisible(inventoryButton_, reports || catalog);
    SetActionVisible(csvButton_, reports || catalog);
    ShowWindow(auditButton_, SW_HIDE);
    EnableWindow(auditButton_, FALSE);
    ShowWindow(expiryTable_, (reports || catalog) ? SW_SHOW : SW_HIDE);
}

void ReportsPage::ReloadData() {
    expiryRows_ = data::Repository::Instance().LoadProductExpiryReport();
    if (!expiryTable_) {
        return;
    }
    ui::ClearListRows(expiryTable_);
    for (size_t index = 0; index < expiryRows_.size(); ++index) {
        const auto& row = expiryRows_[index];
        ui::AddListRow(expiryTable_, static_cast<int>(index), {
            row.sku,
            row.product,
            row.category,
            row.expirationDateDisplay,
            std::to_wstring(row.daysRemaining),
            i18n::DataText(row.status),
            row.supplier,
            row.batchCode
        });
    }
    const bool canSeeExpiry = Can(access::Permission::ExportReports) || Can(access::Permission::ImportExportCatalog);
    ShowWindow(expiryTable_, (!expiryRows_.empty() && canSeeExpiry) ? SW_SHOW : SW_HIDE);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void ReportsPage::OnSize(int width, int height) {
    const ReportsLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    ui::MoveWindowToRect(dailyButton_, layout.dailyButton);
    ui::MoveWindowToRect(fulfillButton_, layout.fulfillButton);
    ui::MoveWindowToRect(inventoryButton_, layout.inventoryButton);
    ui::MoveWindowToRect(employeeButton_, layout.employeeButton);
    ui::MoveWindowToRect(clientButton_, layout.clientButton);
    ui::MoveWindowToRect(profitButton_, layout.profitButton);
    ui::MoveWindowToRect(cancelButton_, layout.cancelButton);
    ui::MoveWindowToRect(auditButton_, layout.auditButton);
    ui::MoveWindowToRect(csvButton_, layout.csvButton);
    ui::MoveWindowToRect(pdfButton_, layout.pdfButton);
    ui::MoveWindowToRect(archiveButton_, layout.archiveButton);
    ui::MoveWindowToRect(expiryTable_, layout.expiryTable);
}

void ReportsPage::OnPaint(HDC hdc, const RECT& client) {
    const ReportsLayout layout = BuildLayout(client);
    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"Reports", L"Отчёты"),
        UiText(L"Generate business reports and export summaries.", L"Формирование бизнес-отчётов и экспорт сводок."),
        ui::Width(layout.title));

    DrawReportCard(hdc, layout.cards[0], UiText(L"Daily Sales Summary", L"Сводка продаж за день"),
        UiText(L"Turnover, channel mix, and manager contribution captured for the latest available reporting day.",
               L"Оборот, каналы и вклад менеджеров по последнему доступному отчётному дню."),
        UiText(L"Output: TXT  |  Frequency: Daily", L"Формат: TXT  |  Частота: Ежедневно"));
    DrawReportCard(hdc, layout.cards[1], UiText(L"Order Fulfillment Report", L"Отчёт по исполнению заказов"),
        UiText(L"Shipment progress, delay hotspots, priority queue, and open pipeline value for operations leadership.",
               L"Ход отгрузок, задержки, приоритетная очередь и стоимость открытого pipeline для операционного блока."),
        UiText(L"Output: TXT  |  Frequency: Operational", L"Формат: TXT  |  Частота: Оперативно"));
    DrawReportCard(hdc, layout.cards[2], UiText(L"Inventory Risk Report", L"Отчёт по рискам склада"),
        UiText(L"Low stock exposure, reorder urgency, product expiration dates, batch codes, and supplier dependency.",
               L"Низкие остатки, срочность перезаказа, сроки годности, партии и зависимость от поставщиков."),
        UiText(L"Output: Live SQL / CSV  |  Frequency: Weekly", L"Формат: Live SQL / CSV  |  Частота: Еженедельно"));
    DrawReportCard(hdc, layout.cards[3], UiText(L"Employee Performance Report", L"Отчёт по сотрудникам"),
        UiText(L"Revenue, completed deals, and average margin for each sales manager in the current dataset.",
               L"Выручка, завершённые сделки и средняя маржа по каждому менеджеру в текущем наборе данных."),
        UiText(L"Output: TXT  |  Frequency: Monthly", L"Формат: TXT  |  Частота: Ежемесячно"));

    ui::DrawRoundedPanel(hdc, layout.actionsPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.actionsPanel.left + ui::Scale(16), layout.actionsPanel.top + ui::Scale(14), UiText(L"Product Expiration Watchlist", L"Контроль сроков годности"),
        UiText(L"Live SQL watchlist for products close to expiry. CSV export, PDF queue manifest, and archive snapshots are active.",
               L"Живой SQL-список товаров с близким сроком годности. CSV-экспорт, PDF-очередь и архивные снимки работают."),
        ui::Width(layout.actionsPanel) - ui::Scale(420));
    ui::DrawTextLine(hdc, UiText(L"Rows are sorted by expiration date so critical batches appear first.", L"Строки отсортированы по сроку годности, поэтому критичные партии находятся выше."),
        ui::MakeRect(layout.actionsPanel.left + ui::Scale(18), layout.actionsPanel.top + ui::Scale(70), layout.actionsPanel.right - ui::Scale(320), layout.actionsPanel.top + ui::Scale(94)),
        theme::SmallFont(), theme::kTextSecondary, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);
    if (expiryRows_.empty()) {
        ui::DrawEmptyState(hdc, layout.expiryTable, UiText(L"No expiration risks", L"Рисков срока годности нет"),
            UiText(L"Products close to expiration will appear here.", L"Товары с близким сроком годности появятся здесь."), L"\u2713");
    }
}

LRESULT ReportsPage::OnCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);
    if (code != BN_CLICKED) {
        return 0;
    }
    if (id == IDC_DAILY) {
        ShowExportResult(hwnd_, UiText(L"Daily Sales Summary", L"Сводка продаж"),
            UiText(L"Daily sales report exported:\n", L"Дневной отчёт экспортирован:\n"),
            data::Repository::Instance().ExportDailySalesSummaryReport());
        return 0;
    }
    if (id == IDC_FULFILL) {
        ShowExportResult(hwnd_, UiText(L"Fulfillment Report", L"Отчёт по исполнению"),
            UiText(L"Fulfillment report exported:\n", L"Отчёт по исполнению экспортирован:\n"),
            data::Repository::Instance().ExportFulfillmentReport());
        return 0;
    }
    if (id == IDC_INVENTORY) {
        ReloadData();
        InvalidateRect(hwnd_, nullptr, FALSE);
        MessageBoxW(hwnd_, UiText(L"Product expiration report refreshed from SQLite.", L"Отчёт по срокам годности обновлён из SQLite.").c_str(), UiText(L"Reports", L"Отчёты").c_str(), MB_OK | MB_ICONINFORMATION);
        return 0;
    }
    if (id == IDC_EMPLOYEE) {
        ShowExportResult(hwnd_, UiText(L"Employee Performance", L"Отчёт по сотрудникам"),
            UiText(L"Employee performance report exported:\n", L"Отчёт по сотрудникам экспортирован:\n"),
            data::Repository::Instance().ExportEmployeePerformanceReport());
        return 0;
    }
    if (id == IDC_CSV) {
        ShowExportResult(hwnd_, UiText(L"Export CSV", L"Экспорт CSV"),
            UiText(L"Product expiration CSV exported:\n", L"CSV по срокам годности экспортирован:\n"),
            data::Repository::Instance().ExportProductExpiryCsv());
        return 0;
    }
    if (id == IDC_CLIENT) {
        ShowExportResult(hwnd_, UiText(L"Client Report", L"Client Report"),
            UiText(L"Client report exported:\n", L"Client report exported:\n"),
            data::Repository::Instance().ExportClientReport());
        return 0;
    }
    if (id == IDC_PROFIT) {
        ShowExportResult(hwnd_, UiText(L"Product Profit Report", L"Product Profit Report"),
            UiText(L"Product profit report exported:\n", L"Product profit report exported:\n"),
            data::Repository::Instance().ExportProductProfitReport());
        return 0;
    }
    if (id == IDC_CANCEL) {
        ShowExportResult(hwnd_, UiText(L"Cancellations and Returns", L"Cancellations and Returns"),
            UiText(L"Cancellations and returns report exported:\n", L"Cancellations and returns report exported:\n"),
            data::Repository::Instance().ExportCancellationReport());
        return 0;
    }
    if (id == IDC_AUDIT) {
        return 0;
    }
    if (id == IDC_PDF) {
        ShowExportResult(hwnd_, UiText(L"PDF Queue", L"PDF-очередь"),
            UiText(L"PDF queue manifest created:\n", L"Манифест PDF-очереди создан:\n"),
            data::Repository::Instance().ExportPdfQueueManifest());
        return 0;
    }
    if (id == IDC_ARCHIVE) {
        ShowExportResult(hwnd_, UiText(L"Archive Exports", L"Архив экспортов"),
            UiText(L"Export snapshot archived to:\n", L"Снимок экспортов сохранён в:\n"),
            data::Repository::Instance().ArchiveExportsSnapshot());
        return 0;
    }
    return 0;
}

LRESULT ReportsPage::OnDrawItem(WPARAM, LPARAM lParam) {
    ui::DrawUiButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam), ui::GetButtonKind(reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->hwndItem), false);
    return TRUE;
}
