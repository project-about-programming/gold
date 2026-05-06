#include "UsersRolesPage.h"

#include "AccessControl.h"
#include "DataEntryDialog.h"
#include "DataRepository.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>

namespace {
enum : int {
    IDC_ADD_USER = 9901,
    IDC_TOGGLE_USER,
    IDC_RESET_PASSWORD,
    IDC_CHANGE_ROLE,
    IDC_DELETE_USER,
    IDC_USERS_TABLE,
    IDC_ROLES_TABLE
};

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

struct UsersLayout {
    RECT title{};
    RECT kpis[4]{};
    RECT toolbar{};
    RECT add{};
    RECT toggle{};
    RECT reset{};
    RECT role{};
    RECT remove{};
    RECT tablePanel{};
    RECT table{};
    RECT rolesPanel{};
    RECT rolesTable{};
};

UsersLayout BuildLayout(const RECT& client) {
    UsersLayout layout{};
    const auto m = ui::GetMetrics();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);
    RECT kpiRow = ui::TakeTop(&area, ui::Scale(116), m.sectionGap);
    const auto kpis = ui::SplitColumns(kpiRow, 4, m.sectionGap);
    for (int i = 0; i < 4; ++i) {
        layout.kpis[i] = kpis[i];
    }
    layout.toolbar = ui::TakeTop(&area, ui::Scale(68), m.sectionGap);
    auto content = ui::SplitColumns(area, 2, m.sectionGap);
    layout.tablePanel = content[0];
    layout.rolesPanel = content[1];

    RECT toolbarInner = ui::Inset(layout.toolbar, ui::Scale(18), ui::Scale(14));
    const int buttonWidth = ui::Scale(132);
    layout.add = ui::TakeLeft(&toolbarInner, buttonWidth, m.compactGap);
    layout.toggle = ui::TakeLeft(&toolbarInner, buttonWidth, m.compactGap);
    layout.reset = ui::TakeLeft(&toolbarInner, ui::Scale(152), m.compactGap);
    layout.role = ui::TakeLeft(&toolbarInner, ui::Scale(142), m.compactGap);
    layout.remove = ui::TakeLeft(&toolbarInner, buttonWidth, m.compactGap);
    layout.table = ui::Inset(ui::MakeRect(layout.tablePanel.left, layout.tablePanel.top + m.panelHeaderHeight, layout.tablePanel.right, layout.tablePanel.bottom), ui::Scale(12), ui::Scale(12));
    layout.rolesTable = ui::Inset(ui::MakeRect(layout.rolesPanel.left, layout.rolesPanel.top + m.panelHeaderHeight, layout.rolesPanel.right, layout.rolesPanel.bottom), ui::Scale(12), ui::Scale(12));
    return layout;
}

bool Can(access::Permission permission) {
    return access::HasPermission(data::Repository::Instance().CurrentAccount(), permission);
}

void SetActionVisible(HWND hwnd, bool visible) {
    ShowWindow(hwnd, visible ? SW_SHOW : SW_HIDE);
    EnableWindow(hwnd, visible ? TRUE : FALSE);
}

std::vector<ui::DataEntryOption> RoleOptions(const std::vector<data::RoleSummaryRecord>& roles) {
    std::vector<ui::DataEntryOption> options;
    for (const auto& role : roles) {
        options.push_back({role.id, role.name});
    }
    return options;
}

std::wstring RoleIdValue(const std::vector<data::RoleSummaryRecord>& roles, const std::wstring& roleName) {
    for (const auto& role : roles) {
        if (role.name == roleName) {
            return std::to_wstring(role.id);
        }
    }
    return L"";
}

void RequireSelection(HWND hwnd, const std::wstring& title) {
    MessageBoxW(hwnd, UiText(L"Select an account first.", L"Сначала выберите аккаунт.").c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}
}

UsersRolesPage::UsersRolesPage()
    : PageBase(L"Users & Roles", L"Configure user accounts, active status and role-based access.") {}

const wchar_t* UsersRolesPage::ClassName() const { return L"NativeUsersRolesPage"; }

void UsersRolesPage::OnCreate() {
    addButton_ = ui::CreateUiButton(hwnd_, IDC_ADD_USER, L"Add User", ui::ButtonKind::Primary);
    toggleButton_ = ui::CreateUiButton(hwnd_, IDC_TOGGLE_USER, L"Lock / Unlock", ui::ButtonKind::Secondary);
    resetPasswordButton_ = ui::CreateUiButton(hwnd_, IDC_RESET_PASSWORD, L"Reset Password", ui::ButtonKind::Secondary);
    changeRoleButton_ = ui::CreateUiButton(hwnd_, IDC_CHANGE_ROLE, L"Change Role", ui::ButtonKind::Secondary);
    deleteButton_ = ui::CreateUiButton(hwnd_, IDC_DELETE_USER, L"Delete User", ui::ButtonKind::Danger);
    accountsTable_ = ui::CreateUiListView(hwnd_, IDC_USERS_TABLE);
    ui::AddListColumns(accountsTable_,
        {UiText(L"Name", L"Имя"), UiText(L"Username", L"Логин"), UiText(L"Role", L"Роль"),
         UiText(L"Status", L"Статус"), UiText(L"Last Login", L"Последний вход"), UiText(L"Created", L"Создано")},
        {220, 150, 190, 120, 170, 150});
    rolesTable_ = ui::CreateUiListView(hwnd_, IDC_ROLES_TABLE);
    ui::AddListColumns(rolesTable_,
        {UiText(L"Role", L"Роль"), UiText(L"Permissions", L"Права"), UiText(L"Start", L"Старт")},
        {210, 110, 120});
    ReloadData();
}

void UsersRolesPage::OnActivate() {
    ReloadData();
}

void UsersRolesPage::ReloadData() {
    SetActionVisible(addButton_, Can(access::Permission::UsersCreate));
    SetActionVisible(toggleButton_, Can(access::Permission::UsersDisable));
    SetActionVisible(resetPasswordButton_, Can(access::Permission::UsersEdit));
    SetActionVisible(changeRoleButton_, Can(access::Permission::RolesEdit));
    SetActionVisible(deleteButton_, Can(access::Permission::UsersDelete));

    accounts_ = Can(access::Permission::UsersView) ? data::Repository::Instance().LoadAccounts() : std::vector<data::AccountRecord>{};
    roles_ = Can(access::Permission::RolesView) ? data::Repository::Instance().LoadRoleSummaries() : std::vector<data::RoleSummaryRecord>{};
    ui::ClearListRows(accountsTable_);
    for (size_t index = 0; index < accounts_.size(); ++index) {
        const auto& account = accounts_[index];
        ui::AddListRow(accountsTable_, static_cast<int>(index), {
            account.fullName,
            account.username,
            i18n::DataText(account.role),
            i18n::DataText(account.status),
            i18n::DataText(account.lastLogin),
            account.createdAt
        });
    }
    ui::ClearListRows(rolesTable_);
    for (size_t index = 0; index < roles_.size(); ++index) {
        const auto& role = roles_[index];
        ui::AddListRow(rolesTable_, static_cast<int>(index), {
            role.name,
            std::to_wstring(role.permissionCount),
            role.defaultStartPage
        });
    }
    ShowWindow(accountsTable_, accounts_.empty() ? SW_HIDE : SW_SHOW);
    ShowWindow(rolesTable_, roles_.empty() ? SW_HIDE : SW_SHOW);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

int UsersRolesPage::SelectedAccountIndex() const {
    return ListView_GetNextItem(accountsTable_, -1, LVNI_SELECTED);
}

void UsersRolesPage::OnSize(int width, int height) {
    const UsersLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    ui::MoveWindowToRect(addButton_, layout.add);
    ui::MoveWindowToRect(toggleButton_, layout.toggle);
    ui::MoveWindowToRect(resetPasswordButton_, layout.reset);
    ui::MoveWindowToRect(changeRoleButton_, layout.role);
    ui::MoveWindowToRect(deleteButton_, layout.remove);
    ui::MoveWindowToRect(accountsTable_, layout.table);
    ui::MoveWindowToRect(rolesTable_, layout.rolesTable);

    const int tableWidth = std::max(ui::Scale(760), ui::Width(layout.table) - ui::Scale(6));
    const int nameWidth = tableWidth * 24 / 100;
    const int usernameWidth = tableWidth * 14 / 100;
    const int roleWidth = tableWidth * 18 / 100;
    const int statusWidth = tableWidth * 12 / 100;
    const int loginWidth = tableWidth * 18 / 100;
    const int createdWidth = std::max(ui::Scale(130), tableWidth - nameWidth - usernameWidth - roleWidth - statusWidth - loginWidth);
    ListView_SetColumnWidth(accountsTable_, 0, nameWidth);
    ListView_SetColumnWidth(accountsTable_, 1, usernameWidth);
    ListView_SetColumnWidth(accountsTable_, 2, roleWidth);
    ListView_SetColumnWidth(accountsTable_, 3, statusWidth);
    ListView_SetColumnWidth(accountsTable_, 4, loginWidth);
    ListView_SetColumnWidth(accountsTable_, 5, createdWidth);

    const int rolesWidth = std::max(ui::Scale(360), ui::Width(layout.rolesTable) - ui::Scale(6));
    ListView_SetColumnWidth(rolesTable_, 0, rolesWidth * 52 / 100);
    ListView_SetColumnWidth(rolesTable_, 1, rolesWidth * 22 / 100);
    ListView_SetColumnWidth(rolesTable_, 2, rolesWidth - (rolesWidth * 52 / 100) - (rolesWidth * 22 / 100));
}

void UsersRolesPage::OnPaint(HDC hdc, const RECT& client) {
    const UsersLayout layout = BuildLayout(client);
    const int total = static_cast<int>(accounts_.size());
    const int active = static_cast<int>(std::count_if(accounts_.begin(), accounts_.end(), [](const auto& account) {
        return account.status == L"Active";
    }));
    const int locked = total - active;
    const int admins = static_cast<int>(std::count_if(accounts_.begin(), accounts_.end(), [](const auto& account) {
        return access::RoleForAccount(account) == access::AppRole::SystemAdministrator
            || access::RoleForAccount(account) == access::AppRole::SuperAdmin;
    }));

    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"Users & Roles", L"Пользователи и роли"),
        UiText(L"System account lifecycle, password reset, active status and role assignment.", L"Управление аккаунтами, паролями, статусом активности и ролями."),
        ui::Width(layout.title));

    ui::DrawKpiCard(hdc, layout.kpis[0], UiText(L"Total Users", L"Всего пользователей"), std::to_wstring(total),
        UiText(L"Accounts in SQLite", L"Аккаунты в SQLite"), theme::kBlue);
    ui::DrawKpiCard(hdc, layout.kpis[1], UiText(L"Active", L"Активные"), std::to_wstring(active),
        UiText(L"Can sign in", L"Могут входить"), theme::kGreen);
    ui::DrawKpiCard(hdc, layout.kpis[2], UiText(L"Locked", L"Заблокированы"), std::to_wstring(locked),
        UiText(L"Access disabled", L"Доступ отключён"), theme::kDanger);
    ui::DrawKpiCard(hdc, layout.kpis[3], UiText(L"Admins", L"Администраторы"), std::to_wstring(admins),
        UiText(L"System-level accounts", L"Системные аккаунты"), theme::kAmber);

    ui::DrawRoundedPanel(hdc, layout.toolbar, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(14), false);
    ui::DrawTextLine(hdc, UiText(L"Account actions", L"Действия с аккаунтами"),
        ui::MakeRect(layout.remove.right + ui::Scale(16), layout.toolbar.top + ui::Scale(14), layout.toolbar.right - ui::Scale(18), layout.toolbar.top + ui::Scale(38)),
        theme::BodyBoldFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    ui::DrawTextLine(hdc, UiText(L"Select a row before editing, resetting password, changing role or deleting.", L"Перед изменением, сбросом пароля, сменой роли или удалением выберите строку."),
        ui::MakeRect(layout.remove.right + ui::Scale(16), layout.toolbar.top + ui::Scale(38), layout.toolbar.right - ui::Scale(18), layout.toolbar.bottom - ui::Scale(10)),
        theme::SmallFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

    ui::DrawRoundedPanel(hdc, layout.tablePanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.tablePanel.left + ui::Scale(16), layout.tablePanel.top + ui::Scale(14),
        UiText(L"Account Registry", L"Реестр аккаунтов"), UiText(L"Role changes are logged in Audit Log.", L"Смена ролей записывается в журнал аудита."),
        ui::Width(layout.tablePanel) - ui::Scale(32));
    if (accounts_.empty()) {
        ui::DrawEmptyState(hdc, layout.table, UiText(L"No user accounts", L"Аккаунтов нет"),
            UiText(L"Add users to configure role-based access.", L"Добавьте пользователей для настройки ролевого доступа."), L"\u25CF");
    }
    ui::DrawRoundedPanel(hdc, layout.rolesPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.rolesPanel.left + ui::Scale(16), layout.rolesPanel.top + ui::Scale(14),
        UiText(L"Role Matrix", L"Role Matrix"), UiText(L"Read-only permission summary for each role.", L"Read-only permission summary for each role."),
        ui::Width(layout.rolesPanel) - ui::Scale(32));
    if (roles_.empty()) {
        ui::DrawEmptyState(hdc, layout.rolesTable, UiText(L"No roles available", L"No roles available"),
            UiText(L"Role records are created during database initialization.", L"Role records are created during database initialization."), L"\u25C9");
    }
}

LRESULT UsersRolesPage::OnCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);
    if (code != BN_CLICKED) {
        return 0;
    }
    if (id == IDC_ADD_USER) {
        if (!data::Repository::Instance().AddDemoAccount()) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Add User", MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    const int index = SelectedAccountIndex();
    if (index < 0 || index >= static_cast<int>(accounts_.size())) {
        RequireSelection(hwnd_, L"Users & Roles");
        return 0;
    }

    if (id == IDC_TOGGLE_USER) {
        if (!data::Repository::Instance().ToggleAccountStatus(accounts_[index].id)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Lock / Unlock", MB_OK | MB_ICONWARNING);
        }
    } else if (id == IDC_RESET_PASSWORD) {
        if (!data::Repository::Instance().ResetAccountPassword(accounts_[index].id)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Reset Password", MB_OK | MB_ICONWARNING);
        } else {
            MessageBoxW(hwnd_, L"Password was reset to demo123.", L"Reset Password", MB_OK | MB_ICONINFORMATION);
        }
    } else if (id == IDC_CHANGE_ROLE) {
        std::vector<ui::DataEntryField> fields{
            {L"Role", L"", RoleIdValue(roles_, accounts_[index].role), true, ui::DataEntryFieldKind::Combo, RoleOptions(roles_), true, false}
        };
        if (!ui::ShowDataEntryDialog(hwnd_, L"Change Role", L"Select a role from the configured role list.", fields)) {
            return 0;
        }
        int roleId = 0;
        try {
            roleId = std::stoi(fields[0].value);
        } catch (...) {
            roleId = 0;
        }
        if (!data::Repository::Instance().SetAccountRole(accounts_[index].id, roleId)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Change Role", MB_OK | MB_ICONWARNING);
        }
    } else if (id == IDC_DELETE_USER) {
        if (MessageBoxW(hwnd_, L"Delete the selected account?", L"Delete User", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            if (!data::Repository::Instance().DeleteAccount(accounts_[index].id)) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Delete User", MB_OK | MB_ICONWARNING);
            }
        }
    }
    ReloadData();
    return 0;
}

LRESULT UsersRolesPage::OnDrawItem(WPARAM, LPARAM lParam) {
    auto* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
    ui::DrawUiButton(dis, ui::GetButtonKind(dis->hwndItem), false);
    return TRUE;
}
