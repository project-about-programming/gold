#include "AuditLogPage.h"

#include "AccessControl.h"
#include "DataRepository.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>

namespace {
enum : int {
    IDC_REFRESH_AUDIT = 9801,
    IDC_EXPORT_AUDIT,
    IDC_AUDIT_TABLE
};

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

struct AuditLayout {
    RECT title{};
    RECT toolbar{};
    RECT refresh{};
    RECT exportButton{};
    RECT tablePanel{};
    RECT table{};
};

AuditLayout BuildLayout(const RECT& client) {
    AuditLayout layout{};
    const auto m = ui::GetMetrics();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);
    layout.toolbar = ui::TakeTop(&area, ui::Scale(68), m.sectionGap);
    layout.tablePanel = area;

    RECT toolbarInner = ui::Inset(layout.toolbar, ui::Scale(18), ui::Scale(14));
    const int exportWidth = ui::Scale(132);
    const int refreshWidth = ui::Scale(108);
    layout.exportButton = ui::TakeRight(&toolbarInner, exportWidth, m.compactGap);
    layout.refresh = ui::TakeRight(&toolbarInner, refreshWidth, m.compactGap);
    layout.table = ui::Inset(ui::MakeRect(layout.tablePanel.left, layout.tablePanel.top + m.panelHeaderHeight, layout.tablePanel.right, layout.tablePanel.bottom), ui::Scale(12), ui::Scale(12));
    return layout;
}

bool Can(access::Permission permission) {
    return access::HasPermission(data::Repository::Instance().CurrentAccount(), permission);
}
}

AuditLogPage::AuditLogPage()
    : PageBase(L"Audit Log", L"Review system activity, sign-ins, access denials and data changes.") {}

const wchar_t* AuditLogPage::ClassName() const { return L"NativeAuditLogPage"; }

void AuditLogPage::OnCreate() {
    refreshButton_ = ui::CreateUiButton(hwnd_, IDC_REFRESH_AUDIT, L"Refresh", ui::ButtonKind::Secondary);
    exportButton_ = ui::CreateUiButton(hwnd_, IDC_EXPORT_AUDIT, L"Export Log", ui::ButtonKind::Primary);
    auditTable_ = ui::CreateUiListView(hwnd_, IDC_AUDIT_TABLE);
    ui::AddListColumns(auditTable_,
        {UiText(L"Date / Time", L"Дата / время"), UiText(L"User", L"Пользователь"), UiText(L"Module", L"Модуль"),
         UiText(L"Action", L"Действие"), UiText(L"Details", L"Детали"), UiText(L"Severity", L"Уровень")},
        {160, 150, 120, 140, 360, 110});
    ReloadData();
}

void AuditLogPage::OnActivate() {
    ReloadData();
}

std::wstring AuditLogPage::SeverityFor(const data::AuditRecord& row) const {
    if (!row.severity.empty()) {
        return row.severity;
    }
    const std::wstring action = row.action;
    const std::wstring details = row.details;
    if (action == L"Denied" || details.find(L"denied") != std::wstring::npos || details.find(L"Denied") != std::wstring::npos) {
        return L"Critical";
    }
    if (action.find(L"Delete") != std::wstring::npos || action.find(L"Clear") != std::wstring::npos
        || action.find(L"Reset") != std::wstring::npos || action.find(L"ChangeRole") != std::wstring::npos) {
        return L"Warning";
    }
    return L"Info";
}

void AuditLogPage::ReloadData() {
    ShowWindow(exportButton_, Can(access::Permission::AuditExport) ? SW_SHOW : SW_HIDE);
    EnableWindow(exportButton_, Can(access::Permission::AuditExport) ? TRUE : FALSE);
    rows_ = Can(access::Permission::AuditView) ? data::Repository::Instance().LoadAuditLog(160) : std::vector<data::AuditRecord>{};
    ui::ClearListRows(auditTable_);
    for (size_t index = 0; index < rows_.size(); ++index) {
        const auto& row = rows_[index];
        ui::AddListRow(auditTable_, static_cast<int>(index), {
            row.createdAt,
            row.userName,
            row.entityType,
            row.action,
            row.details,
            SeverityFor(row)
        });
    }
    ShowWindow(auditTable_, rows_.empty() ? SW_HIDE : SW_SHOW);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void AuditLogPage::OnSize(int width, int height) {
    const AuditLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    ui::MoveWindowToRect(refreshButton_, layout.refresh);
    ui::MoveWindowToRect(exportButton_, layout.exportButton);
    ui::MoveWindowToRect(auditTable_, layout.table);

    const int tableWidth = std::max(ui::Scale(760), ui::Width(layout.table) - ui::Scale(6));
    const int dateWidth = tableWidth * 16 / 100;
    const int userWidth = tableWidth * 14 / 100;
    const int moduleWidth = tableWidth * 12 / 100;
    const int actionWidth = tableWidth * 14 / 100;
    const int severityWidth = ui::Scale(116);
    const int detailsWidth = std::max(ui::Scale(220), tableWidth - dateWidth - userWidth - moduleWidth - actionWidth - severityWidth);
    ListView_SetColumnWidth(auditTable_, 0, dateWidth);
    ListView_SetColumnWidth(auditTable_, 1, userWidth);
    ListView_SetColumnWidth(auditTable_, 2, moduleWidth);
    ListView_SetColumnWidth(auditTable_, 3, actionWidth);
    ListView_SetColumnWidth(auditTable_, 4, detailsWidth);
    ListView_SetColumnWidth(auditTable_, 5, severityWidth);
}

void AuditLogPage::OnPaint(HDC hdc, const RECT& client) {
    const AuditLayout layout = BuildLayout(client);
    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"Audit Log", L"Журнал действий"),
        UiText(L"Security events, access checks and important system changes.", L"События безопасности, проверки доступа и важные изменения системы."),
        ui::Width(layout.title));

    ui::DrawRoundedPanel(hdc, layout.toolbar, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(14), false);
    ui::DrawTextLine(hdc, UiText(L"System activity monitor", L"Мониторинг действий системы"),
        ui::MakeRect(layout.toolbar.left + ui::Scale(18), layout.toolbar.top + ui::Scale(14), layout.refresh.left - ui::Scale(16), layout.toolbar.top + ui::Scale(38)),
        theme::BodyBoldFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    ui::DrawTextLine(hdc, UiText(L"Latest entries are shown first. Access denials are highlighted as critical.", L"Последние записи показаны первыми. Отказы доступа отмечаются как критичные."),
        ui::MakeRect(layout.toolbar.left + ui::Scale(18), layout.toolbar.top + ui::Scale(38), layout.refresh.left - ui::Scale(16), layout.toolbar.bottom - ui::Scale(10)),
        theme::SmallFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

    ui::DrawRoundedPanel(hdc, layout.tablePanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.tablePanel.left + ui::Scale(16), layout.tablePanel.top + ui::Scale(14),
        UiText(L"Audit Events", L"События аудита"), UiText(L"Who did what, where and when.", L"Кто, что, где и когда сделал."),
        ui::Width(layout.tablePanel) - ui::Scale(32));
    if (rows_.empty()) {
        ui::DrawEmptyState(hdc, layout.table, UiText(L"No audit events yet", L"Событий аудита пока нет"),
            UiText(L"Security and system changes will appear here.", L"События безопасности и системные изменения появятся здесь."), L"\u25CF");
    }
}

LRESULT AuditLogPage::OnCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);
    if (code != BN_CLICKED) {
        return 0;
    }
    if (id == IDC_REFRESH_AUDIT) {
        ReloadData();
        return 0;
    }
    if (id == IDC_EXPORT_AUDIT) {
        const std::wstring path = data::Repository::Instance().ExportAuditLogReport();
        if (path.empty()) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Export Audit Log", MB_OK | MB_ICONWARNING);
        } else {
            MessageBoxW(hwnd_, (L"Audit log exported:\n" + path).c_str(), L"Export Audit Log", MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }
    return 0;
}

LRESULT AuditLogPage::OnDrawItem(WPARAM, LPARAM lParam) {
    auto* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
    ui::DrawUiButton(dis, ui::GetButtonKind(dis->hwndItem), false);
    return TRUE;
}
