#include "SettingsPage.h"

#include "AccessControl.h"
#include "AppMessages.h"
#include "DataRepository.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>

namespace {
enum : int {
    IDC_THEME = 9001, IDC_LANGUAGE, IDC_DATE, IDC_COMPANY, IDC_REFRESH, IDC_STARTUP,
    IDC_DB_PATH, IDC_DB_ENGINE, IDC_DB_USERS, IDC_DB_ORDERS,
    IDC_TEST, IDC_SAVE, IDC_RESET,
    IDC_ADD_ACCOUNT, IDC_TOGGLE_ACCOUNT, IDC_RESET_PASSWORD, IDC_ROLE_ACCOUNT, IDC_DELETE_ACCOUNT, IDC_ACCOUNTS
};

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

struct SettingsLayout {
    RECT title{};
    RECT kpis[4]{};
    RECT prefsPanel{};
    RECT dbPanel{};
    RECT accountsPanel{};
    RECT accountsTable{};
    RECT theme{};
    RECT language{};
    RECT date{};
    RECT company{};
    RECT refresh{};
    RECT startup{};
    RECT dbPath{};
    RECT dbEngine{};
    RECT dbUsers{};
    RECT dbOrders{};
    RECT test{};
    RECT save{};
    RECT reseed{};
    RECT addAccount{};
    RECT toggleAccount{};
    RECT resetPassword{};
    RECT roleAccount{};
    RECT deleteAccount{};
};

SettingsLayout BuildLayout(const RECT& client) {
    SettingsLayout layout{};
    const auto m = ui::GetMetrics();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);

    RECT kpiRow = ui::TakeTop(&area, ui::Scale(116), m.sectionGap);
    const auto kpis = ui::SplitColumns(kpiRow, 4, m.sectionGap);
    for (int i = 0; i < 4; ++i) {
        layout.kpis[i] = kpis[i];
    }

    const int topPanelsHeight = std::clamp(ui::Height(area) * 42 / 100, ui::Scale(300), ui::Scale(360));
    RECT topPanelsArea = ui::TakeTop(&area, topPanelsHeight, m.sectionGap);
    const auto topPanels = ui::SplitColumns(topPanelsArea, 2, m.sectionGap);
    layout.prefsPanel = topPanels[0];
    layout.dbPanel = topPanels[1];
    layout.accountsPanel = area;
    const int controlHeight = m.controlHeight;

    RECT prefsInner = ui::Inset(layout.prefsPanel, ui::Scale(20), ui::Scale(78));
    const auto prefsCols = ui::SplitColumns(ui::MakeRect(prefsInner.left, prefsInner.top, prefsInner.right, prefsInner.bottom), 2, m.sectionGap);
    const int fieldGap = ui::Scale(26);
    layout.theme = ui::MakeRect(prefsCols[0].left, prefsCols[0].top, prefsCols[0].right, prefsCols[0].top + controlHeight);
    layout.language = ui::MakeRect(prefsCols[1].left, prefsCols[1].top, prefsCols[1].right, prefsCols[1].top + controlHeight);
    layout.date = ui::MakeRect(prefsCols[0].left, layout.theme.bottom + fieldGap, prefsCols[0].right, layout.theme.bottom + fieldGap + controlHeight);
    layout.startup = ui::MakeRect(prefsCols[1].left, layout.language.bottom + fieldGap, prefsCols[1].right, layout.language.bottom + fieldGap + controlHeight);
    layout.company = ui::MakeRect(prefsInner.left, layout.date.bottom + fieldGap, prefsInner.right, layout.date.bottom + fieldGap + controlHeight);
    layout.refresh = ui::MakeRect(prefsInner.left, layout.company.bottom + fieldGap, prefsCols[0].right, layout.company.bottom + fieldGap + controlHeight);

    RECT dbInner = ui::Inset(layout.dbPanel, ui::Scale(20), ui::Scale(78));
    layout.dbPath = ui::MakeRect(dbInner.left, dbInner.top, dbInner.right, dbInner.top + controlHeight);
    layout.dbEngine = ui::MakeRect(dbInner.left, layout.dbPath.bottom + fieldGap, dbInner.right, layout.dbPath.bottom + fieldGap + controlHeight);
    const auto dbStats = ui::SplitColumns(ui::MakeRect(dbInner.left, layout.dbEngine.bottom + fieldGap, dbInner.right, layout.dbEngine.bottom + fieldGap + controlHeight), 2, m.compactGap);
    layout.dbUsers = dbStats[0];
    layout.dbOrders = dbStats[1];

    const int dbButtonsTop = layout.dbPanel.bottom - ui::Scale(58);
    const RECT dbButtonsRow = ui::MakeRect(layout.dbPanel.left + ui::Scale(20), dbButtonsTop, layout.dbPanel.right - ui::Scale(20), dbButtonsTop + m.buttonHeight);
    const auto dbButtons = ui::SplitColumns(dbButtonsRow, 3, m.compactGap);
    layout.test = dbButtons[0];
    layout.save = dbButtons[1];
    layout.reseed = dbButtons[2];

    const int accountButtonsTop = layout.accountsPanel.top + ui::Scale(60);
    const RECT accountButtonsRow = ui::MakeRect(layout.accountsPanel.left + ui::Scale(16), accountButtonsTop, layout.accountsPanel.right - ui::Scale(16), accountButtonsTop + m.buttonHeight);
    const auto accountButtons = ui::SplitColumns(accountButtonsRow, 5, m.compactGap);
    layout.addAccount = accountButtons[0];
    layout.toggleAccount = accountButtons[1];
    layout.resetPassword = accountButtons[2];
    layout.roleAccount = accountButtons[3];
    layout.deleteAccount = accountButtons[4];
    layout.accountsTable = ui::Inset(ui::MakeRect(layout.accountsPanel.left, layout.addAccount.bottom + ui::Scale(16), layout.accountsPanel.right, layout.accountsPanel.bottom), ui::Scale(12), ui::Scale(12));
    return layout;
}

void SelectComboText(HWND combo, const std::wstring& text) {
    const int count = static_cast<int>(SendMessageW(combo, CB_GETCOUNT, 0, 0));
    for (int index = 0; index < count; ++index) {
        wchar_t buffer[128]{};
        SendMessageW(combo, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(buffer));
        if (text == buffer) {
            SendMessageW(combo, CB_SETCURSEL, index, 0);
            return;
        }
    }
}

void DrawLabel(HDC hdc, const RECT& field, const std::wstring& text) {
    ui::DrawTextLine(hdc, text, ui::MakeRect(field.left, field.top - ui::Scale(22), field.right, field.top - ui::Scale(4)),
        theme::SmallBoldFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE);
}

bool CanManageAccounts() {
    return access::HasPermission(data::Repository::Instance().CurrentAccount(), access::Permission::ManageUsers);
}

bool Can(access::Permission permission) {
    return access::HasPermission(data::Repository::Instance().CurrentAccount(), permission);
}

void SetActionVisible(HWND hwnd, bool visible) {
    ShowWindow(hwnd, SW_SHOW);
    EnableWindow(hwnd, visible ? TRUE : FALSE);
}

bool RequireAccountAdmin(HWND hwnd, const std::wstring& title) {
    if (CanManageAccounts()) {
        return true;
    }
    MessageBoxW(hwnd, UiText(L"Only system administrators can manage user accounts and roles.", L"Only system administrators can manage user accounts and roles.").c_str(),
        title.c_str(), MB_OK | MB_ICONWARNING);
    return false;
}

} // namespace

SettingsPage::SettingsPage() : PageBase(L"System Settings", L"Application preferences, SQLite status, and account access management.") {}

const wchar_t* SettingsPage::ClassName() const { return L"NativeSettingsPage"; }

void SettingsPage::OnCreate() {
    themeCombo_ = ui::CreateUiCombo(hwnd_, IDC_THEME);
    languageCombo_ = ui::CreateUiCombo(hwnd_, IDC_LANGUAGE);
    dateCombo_ = ui::CreateUiCombo(hwnd_, IDC_DATE);
    companyEdit_ = ui::CreateUiEdit(hwnd_, IDC_COMPANY, L"");
    refreshEdit_ = ui::CreateUiEdit(hwnd_, IDC_REFRESH, L"");
    startupCombo_ = ui::CreateUiCombo(hwnd_, IDC_STARTUP);

    dbPathEdit_ = ui::CreateUiEdit(hwnd_, IDC_DB_PATH, L"");
    dbEngineEdit_ = ui::CreateUiEdit(hwnd_, IDC_DB_ENGINE, L"");
    dbUsersEdit_ = ui::CreateUiEdit(hwnd_, IDC_DB_USERS, L"");
    dbOrdersEdit_ = ui::CreateUiEdit(hwnd_, IDC_DB_ORDERS, L"");

    testButton_ = ui::CreateUiButton(hwnd_, IDC_TEST, L"\u2699", ui::ButtonKind::Secondary);
    saveButton_ = ui::CreateUiButton(hwnd_, IDC_SAVE, L"\u2713", ui::ButtonKind::Primary);
    resetButton_ = ui::CreateUiButton(hwnd_, IDC_RESET, L"\u2715", ui::ButtonKind::Danger);
    addAccountButton_ = ui::CreateUiButton(hwnd_, IDC_ADD_ACCOUNT, L"+", ui::ButtonKind::Primary);
    toggleAccountButton_ = ui::CreateUiButton(hwnd_, IDC_TOGGLE_ACCOUNT, L"\u25C9", ui::ButtonKind::Secondary);
    resetPasswordButton_ = ui::CreateUiButton(hwnd_, IDC_RESET_PASSWORD, L"*", ui::ButtonKind::Secondary);
    roleAccountButton_ = ui::CreateUiButton(hwnd_, IDC_ROLE_ACCOUNT, L"\u25C6", ui::ButtonKind::Secondary);
    deleteAccountButton_ = ui::CreateUiButton(hwnd_, IDC_DELETE_ACCOUNT, L"\u2715", ui::ButtonKind::Danger);

    accountsTable_ = ui::CreateUiListView(hwnd_, IDC_ACCOUNTS);
    ui::AddListColumns(accountsTable_, {UiText(L"Name", L"Имя"), UiText(L"Username", L"Логин"), UiText(L"Role", L"Роль"), UiText(L"Status", L"Статус"), UiText(L"Last Login", L"Последний вход"), UiText(L"Created", L"Создано")}, {190, 130, 170, 110, 140, 110});

    ui::AddComboItems(themeCombo_, {L"Corporate Light", L"Slate Contrast", L"Presentation Mode"});
    ui::AddComboItems(languageCombo_, {L"English", L"Russian", L"German"});
    ui::AddComboItems(dateCombo_, {L"dd MMM yyyy", L"yyyy-MM-dd", L"MM/dd/yyyy"});
    ui::AddComboItems(startupCombo_, {L"Dashboard", L"Sales", L"Orders", L"Analytics", L"Settings"});
    SendMessageW(dbPathEdit_, EM_SETREADONLY, TRUE, 0);
    SendMessageW(dbEngineEdit_, EM_SETREADONLY, TRUE, 0);
    SendMessageW(dbUsersEdit_, EM_SETREADONLY, TRUE, 0);
    SendMessageW(dbOrdersEdit_, EM_SETREADONLY, TRUE, 0);

    ApplyAccess();
    ReloadData();
}

void SettingsPage::OnActivate() {
    ApplyAccess();
    ReloadData();
}

void SettingsPage::ApplyAccess() {
    const bool settings = Can(access::Permission::ManageSystemSettings);
    const bool users = Can(access::Permission::ManageUsers);
    const bool roles = Can(access::Permission::ManageRoles);
    SetActionVisible(testButton_, settings);
    SetActionVisible(saveButton_, settings);
    SetActionVisible(resetButton_, Can(access::Permission::ClearBusinessData));
    SetActionVisible(addAccountButton_, users);
    SetActionVisible(toggleAccountButton_, users);
    SetActionVisible(resetPasswordButton_, users);
    SetActionVisible(roleAccountButton_, roles);
    SetActionVisible(deleteAccountButton_, users);
    EnableWindow(themeCombo_, settings ? TRUE : FALSE);
    EnableWindow(languageCombo_, settings ? TRUE : FALSE);
    EnableWindow(dateCombo_, settings ? TRUE : FALSE);
    EnableWindow(companyEdit_, settings ? TRUE : FALSE);
    EnableWindow(refreshEdit_, settings ? TRUE : FALSE);
    EnableWindow(startupCombo_, settings ? TRUE : FALSE);
}

void SettingsPage::ReloadData() {
    settings_ = data::Repository::Instance().LoadSettings();
    overview_ = data::Repository::Instance().LoadDatabaseOverview();
    accounts_ = data::Repository::Instance().LoadAccounts();

    SetWindowTextW(companyEdit_, settings_.appName.c_str());
    SetWindowTextW(refreshEdit_, settings_.autoRefreshMinutes.c_str());
    SetWindowTextW(dbPathEdit_, overview_.databasePath.c_str());
    SetWindowTextW(dbEngineEdit_, overview_.engineVersion.c_str());
    SetWindowTextW(dbUsersEdit_, std::to_wstring(overview_.usersCount).c_str());
    SetWindowTextW(dbOrdersEdit_, std::to_wstring(overview_.ordersCount).c_str());

    SelectComboText(themeCombo_, settings_.theme);
    SelectComboText(languageCombo_, settings_.language);
    SelectComboText(dateCombo_, settings_.dateFormat);
    SelectComboText(startupCombo_, settings_.startupPage);

    FillAccounts();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void SettingsPage::FillAccounts() {
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
}

int SettingsPage::SelectedAccountIndex() const {
    return ListView_GetNextItem(accountsTable_, -1, LVNI_SELECTED);
}

void SettingsPage::OnSize(int width, int height) {
    const SettingsLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    ui::MoveWindowToRect(themeCombo_, layout.theme);
    ui::MoveWindowToRect(languageCombo_, layout.language);
    ui::MoveWindowToRect(dateCombo_, layout.date);
    ui::MoveWindowToRect(companyEdit_, layout.company);
    ui::MoveWindowToRect(refreshEdit_, layout.refresh);
    ui::MoveWindowToRect(startupCombo_, layout.startup);

    ui::MoveWindowToRect(dbPathEdit_, layout.dbPath);
    ui::MoveWindowToRect(dbEngineEdit_, layout.dbEngine);
    ui::MoveWindowToRect(dbUsersEdit_, layout.dbUsers);
    ui::MoveWindowToRect(dbOrdersEdit_, layout.dbOrders);

    ui::MoveWindowToRect(testButton_, layout.test);
    ui::MoveWindowToRect(saveButton_, layout.save);
    ui::MoveWindowToRect(resetButton_, layout.reseed);

    ui::MoveWindowToRect(addAccountButton_, layout.addAccount);
    ui::MoveWindowToRect(toggleAccountButton_, layout.toggleAccount);
    ui::MoveWindowToRect(resetPasswordButton_, layout.resetPassword);
    ui::MoveWindowToRect(roleAccountButton_, layout.roleAccount);
    ui::MoveWindowToRect(deleteAccountButton_, layout.deleteAccount);
    ui::MoveWindowToRect(accountsTable_, layout.accountsTable);
}

void SettingsPage::OnPaint(HDC hdc, const RECT& client) {
    const SettingsLayout layout = BuildLayout(client);
    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"System Preferences", L"Системные параметры"),
        UiText(L"Settings are aligned into a cleaner form workspace, with database diagnostics and account administration on the same grid.",
               L"Настройки собраны в более аккуратное рабочее пространство вместе с диагностикой базы и управлением аккаунтами."),
        ui::Width(layout.title));

    ui::DrawKpiCard(hdc, layout.kpis[0], UiText(L"Database Engine", L"Движок БД"), overview_.engineVersion.empty() ? UiText(L"Offline", L"Оффлайн") : overview_.engineVersion, overview_.connected ? UiText(L"Local SQLite runtime available", L"Локальный SQLite доступен") : UiText(L"Database not initialized", L"База не инициализирована"), theme::kBlue);
    ui::DrawKpiCard(hdc, layout.kpis[1], UiText(L"User Accounts", L"Учётные записи"), std::to_wstring(overview_.usersCount), UiText(L"Role-based access seeded into SQL", L"Ролевой доступ загружен в SQL"), theme::kGreen);
    ui::DrawKpiCard(hdc, layout.kpis[2], UiText(L"Orders Stored", L"Заказов в базе"), std::to_wstring(overview_.ordersCount), UiText(L"Current SQL order rows", L"Текущие строки заказов в SQL"), theme::kAmber);
    ui::DrawKpiCard(hdc, layout.kpis[3], UiText(L"Last Seed", L"Последняя инициализация"), overview_.lastSeededAt.empty() ? UiText(L"N/A", L"Н/Д") : overview_.lastSeededAt.substr(0, 10), UiText(L"Demo data refresh timestamp", L"Время обновления демо-данных"), theme::kAccent);

    ui::DrawRoundedPanel(hdc, layout.prefsPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawRoundedPanel(hdc, layout.dbPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawRoundedPanel(hdc, layout.accountsPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);

    ui::DrawSectionTitle(hdc, layout.prefsPanel.left + ui::Scale(16), layout.prefsPanel.top + ui::Scale(14), UiText(L"Application Preferences", L"Параметры приложения"),
        UiText(L"Theme, language, formatting, and startup behavior stored in the settings table.", L"Тема, язык, форматирование и стартовая страница хранятся в таблице настроек."), ui::Width(layout.prefsPanel) - ui::Scale(32));
    ui::DrawSectionTitle(hdc, layout.dbPanel.left + ui::Scale(16), layout.dbPanel.top + ui::Scale(14), UiText(L"SQLite Workspace", L"Рабочее пространство SQLite"),
        UiText(L"Current local database path, engine metadata, and quick diagnostics.", L"Текущий путь к базе, метаданные движка и быстрая диагностика."), ui::Width(layout.dbPanel) - ui::Scale(32));
    ui::DrawSectionTitle(hdc, layout.accountsPanel.left + ui::Scale(16), layout.accountsPanel.top + ui::Scale(14), UiText(L"Accounts & Access Control", L"Аккаунты и доступ"),
        UiText(L"Manage seeded user accounts and demonstrate role-based administration flows.", L"Управление учётными записями и демонстрация ролевого администрирования."), ui::Width(layout.accountsPanel) - ui::Scale(32));

    DrawLabel(hdc, layout.theme, UiText(L"Theme", L"Тема"));
    DrawLabel(hdc, layout.language, UiText(L"Language", L"Язык"));
    DrawLabel(hdc, layout.date, UiText(L"Date Format", L"Формат даты"));
    DrawLabel(hdc, layout.company, UiText(L"Application Name", L"Название приложения"));
    DrawLabel(hdc, layout.refresh, UiText(L"Auto Refresh", L"Автообновление"));
    DrawLabel(hdc, layout.startup, UiText(L"Startup Page", L"Стартовая страница"));

    DrawLabel(hdc, layout.dbPath, UiText(L"Database File", L"Файл базы данных"));
    DrawLabel(hdc, layout.dbEngine, UiText(L"Engine Version", L"Версия движка"));
    DrawLabel(hdc, layout.dbUsers, UiText(L"User Count", L"Количество пользователей"));
    DrawLabel(hdc, layout.dbOrders, UiText(L"Orders Count", L"Количество заказов"));
}

LRESULT SettingsPage::OnCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);

    if (id == IDC_TEST && code == BN_CLICKED) {
        std::wstring message = UiText(L"SQLite database is available at:\n", L"SQLite база доступна по пути:\n") + overview_.databasePath;
        MessageBoxW(hwnd_, message.c_str(), UiText(L"Database Test", L"Проверка базы").c_str(), MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    if (id == IDC_SAVE && code == BN_CLICKED) {
        wchar_t buffer[256]{};
        GetWindowTextW(companyEdit_, buffer, 256);
        settings_.appName = buffer;
        GetWindowTextW(refreshEdit_, buffer, 256);
        settings_.autoRefreshMinutes = buffer;
        GetWindowTextW(themeCombo_, buffer, 256);
        settings_.theme = buffer;
        GetWindowTextW(languageCombo_, buffer, 256);
        settings_.language = buffer;
        GetWindowTextW(dateCombo_, buffer, 256);
        settings_.dateFormat = buffer;
        GetWindowTextW(startupCombo_, buffer, 256);
        settings_.startupPage = buffer;

        if (!data::Repository::Instance().SaveSettings(settings_)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Save Settings", L"Сохранение настроек").c_str(), MB_OK | MB_ICONWARNING);
        } else {
            theme::ApplyTheme(settings_.theme);
            i18n::SetLanguage(settings_.language);
            PostMessageW(GetParent(hwnd_), WM_APP_SETTINGS_CHANGED, 0, 0);
            MessageBoxW(hwnd_, i18n::Text(L"settings.saved").c_str(), i18n::Text(L"settings.saveTitle").c_str(), MB_OK | MB_ICONINFORMATION);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_RESET && code == BN_CLICKED) {
        if (MessageBoxW(hwnd_, UiText(L"Clear all business tables? Accounts and application settings will be kept.", L"Очистить все рабочие таблицы? Аккаунты и настройки приложения сохранятся.").c_str(), UiText(L"Clear Tables", L"Очистить таблицы").c_str(), MB_YESNO | MB_ICONWARNING) == IDYES) {
            if (!data::Repository::Instance().ClearBusinessData()) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Clear Tables", L"Очистить таблицы").c_str(), MB_OK | MB_ICONWARNING);
            } else {
                MessageBoxW(hwnd_, UiText(L"Business tables were cleared. You can add fresh records now.", L"Рабочие таблицы очищены. Теперь можно добавлять новые записи.").c_str(), UiText(L"Clear Tables", L"Очистить таблицы").c_str(), MB_OK | MB_ICONINFORMATION);
            }
            ReloadData();
        }
        return 0;
    }

    if (id == IDC_ADD_ACCOUNT && code == BN_CLICKED) {
        if (!RequireAccountAdmin(hwnd_, UiText(L"Add Account", L"Добавить аккаунт"))) {
            return 0;
        }
        if (!data::Repository::Instance().AddDemoAccount()) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Add Account", L"Добавить аккаунт").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_TOGGLE_ACCOUNT && code == BN_CLICKED) {
        if (!RequireAccountAdmin(hwnd_, UiText(L"Lock / Unlock Account", L"Блокировка аккаунта"))) {
            return 0;
        }
        const int index = SelectedAccountIndex();
        if (index < 0 || index >= static_cast<int>(accounts_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select an account first.", L"Сначала выберите аккаунт.").c_str(), UiText(L"Lock / Unlock Account", L"Блокировка аккаунта").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (!data::Repository::Instance().ToggleAccountStatus(accounts_[index].id)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Lock / Unlock Account", L"Блокировка аккаунта").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_RESET_PASSWORD && code == BN_CLICKED) {
        if (!RequireAccountAdmin(hwnd_, UiText(L"Reset Password", L"Сброс пароля"))) {
            return 0;
        }
        const int index = SelectedAccountIndex();
        if (index < 0 || index >= static_cast<int>(accounts_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select an account first.", L"Сначала выберите аккаунт.").c_str(), UiText(L"Reset Password", L"Сброс пароля").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (!data::Repository::Instance().ResetAccountPassword(accounts_[index].id)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Reset Password", L"Сброс пароля").c_str(), MB_OK | MB_ICONWARNING);
        } else {
            MessageBoxW(hwnd_, UiText(L"Password was reset to demo123 for the selected demo account.", L"Пароль выбранного демо-аккаунта сброшен на demo123.").c_str(), UiText(L"Reset Password", L"Сброс пароля").c_str(), MB_OK | MB_ICONINFORMATION);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_ROLE_ACCOUNT && code == BN_CLICKED) {
        if (!RequireAccountAdmin(hwnd_, UiText(L"Change Role", L"Change Role"))) {
            return 0;
        }
        const int index = SelectedAccountIndex();
        if (index < 0 || index >= static_cast<int>(accounts_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select an account first.", L"Select an account first.").c_str(), UiText(L"Change Role", L"Change Role").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (!data::Repository::Instance().CycleAccountRole(accounts_[index].id)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Change Role", L"Change Role").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_DELETE_ACCOUNT && code == BN_CLICKED) {
        if (!RequireAccountAdmin(hwnd_, UiText(L"Delete Account", L"Delete Account"))) {
            return 0;
        }
        const int index = SelectedAccountIndex();
        if (index < 0 || index >= static_cast<int>(accounts_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select an account first.", L"Select an account first.").c_str(), UiText(L"Delete Account", L"Delete Account").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (MessageBoxW(hwnd_, UiText(L"Delete the selected account?", L"Delete the selected account?").c_str(), UiText(L"Delete Account", L"Delete Account").c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES) {
            if (!data::Repository::Instance().DeleteAccount(accounts_[index].id)) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Delete Account", L"Delete Account").c_str(), MB_OK | MB_ICONWARNING);
            }
            ReloadData();
        }
        return 0;
    }

    return 0;
}

LRESULT SettingsPage::OnDrawItem(WPARAM, LPARAM lParam) {
    ui::DrawUiButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam), ui::GetButtonKind(reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->hwndItem), false);
    return TRUE;
}

LRESULT SettingsPage::OnNotify(WPARAM wParam, LPARAM lParam) {
    return PageBase::OnNotify(wParam, lParam);
}
