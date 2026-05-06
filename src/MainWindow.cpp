#include "MainWindow.h"

#include "AccessControl.h"
#include "AnalyticsPage.h"
#include "AppMessages.h"
#include "AuditLogPage.h"
#include "ClientsPage.h"
#include "DataRepository.h"
#include "DashboardPage.h"
#include "EmployeesPage.h"
#include "Localization.h"
#include "NotificationsPage.h"
#include "OrdersPage.h"
#include "ProductsPage.h"
#include "ProfilePage.h"
#include "ReportsPage.h"
#include "SalesPage.h"
#include "SettingsPage.h"
#include "Theme.h"
#include "UiHelpers.h"
#include "UsersRolesPage.h"

#include <commctrl.h>
#include <algorithm>
#include <array>
#include <cwchar>
#include <ctime>

namespace {
constexpr int kNavBaseId = 1000;
constexpr int kHeaderRefreshId = 9401;
constexpr int kHeaderExportId = 9402;
constexpr UINT_PTR kClockTimerId = 42;
HBRUSH gHeaderBrush = nullptr;

int SidebarWidth() { return ui::Scale(240); }
int HeaderHeight() { return ui::Scale(112); }
int StatusHeight() { return 0; }
int ShellMargin() { return ui::GetMetrics().outerPadding; }
RECT HeaderRect(const RECT& client) {
    return ui::MakeRect(SidebarWidth() + ShellMargin(), ShellMargin(), client.right - ShellMargin(), HeaderHeight());
}
RECT HeaderClockRect(const RECT& client) {
    return ui::MakeRect(client.right - ui::Scale(232), ui::Scale(44), client.right - ui::Scale(34), ui::Scale(76));
}

constexpr std::array<const wchar_t*, 9> kNavKeys = {
    L"nav.dashboard", L"nav.sales", L"nav.orders", L"nav.products", L"nav.clients", L"nav.employees", L"nav.analytics", L"nav.reports", L"nav.settings"
};

constexpr std::array<const wchar_t*, 9> kPageTitleKeys = {
    L"page.dashboard.title", L"page.sales.title", L"page.orders.title", L"page.products.title", L"page.clients.title", L"page.employees.title", L"page.analytics.title", L"page.reports.title", L"page.settings.title"
};

struct MenuSpec {
    const wchar_t* section;
    const wchar_t* title;
    int pageIndex;
    int iconIndex;
    access::Permission permission;
    bool hasPermission;
    bool isPlaceholder;
    bool bottomAligned;
};

const std::array<MenuSpec, 13> kMenuSpecs = {
    MenuSpec{ L"MAIN", L"Dashboard", access::kPageDashboard, 0, access::Permission::OpenDashboard, true, false, false },
    MenuSpec{ L"MAIN", L"Analytics", access::kPageAnalytics, 6, access::Permission::OpenAnalytics, true, false, false },
    MenuSpec{ L"MAIN", L"Notifications", access::kPageNotifications, 7, access::Permission::OpenNotifications, true, false, false },
    MenuSpec{ L"SALES", L"Sales", access::kPageSales, 1, access::Permission::OpenSales, true, false, false },
    MenuSpec{ L"SALES", L"Orders", access::kPageOrders, 2, access::Permission::OpenOrders, true, false, false },
    MenuSpec{ L"SALES", L"Customers", access::kPageClients, 4, access::Permission::OpenClients, true, false, false },
    MenuSpec{ L"STOCK", L"Inventory", access::kPageProducts, 3, access::Permission::OpenProducts, true, false, false },
    MenuSpec{ L"MANAGEMENT", L"Employees", access::kPageEmployees, 5, access::Permission::OpenEmployees, true, false, false },
    MenuSpec{ L"MANAGEMENT", L"Reports", access::kPageReports, 7, access::Permission::OpenReports, true, false, false },
    MenuSpec{ L"SYSTEM", L"Users & Roles", access::kPageUsersRoles, 5, access::Permission::ManageUsers, true, false, false },
    MenuSpec{ L"SYSTEM", L"Audit Log", access::kPageAuditLog, 7, access::Permission::OpenAuditLog, true, false, false },
    MenuSpec{ L"SYSTEM", L"Profile", access::kPageProfile, 5, access::Permission::OpenProfile, true, false, false },
    MenuSpec{ L"SYSTEM", L"Settings", access::kPageSettings, 8, access::Permission::OpenSettings, true, false, true }
};

bool CanShowMenuItem(const data::AccountRecord& account, const MenuSpec& spec) {
    if (spec.isPlaceholder) {
        return access::HasPermission(account, spec.permission);
    }
    if (spec.hasPermission) {
        return access::HasPermission(account, spec.permission) || access::CanOpenPage(account, spec.pageIndex);
    }
    return access::CanOpenPage(account, spec.pageIndex);
}
}

MainWindow::MainWindow(HINSTANCE instance)
    : instance_(instance), hwnd_(nullptr), clockLabel_(nullptr), refreshButton_(nullptr), exportButton_(nullptr), statusBar_(nullptr), currentPage_(0) {}

bool MainWindow::Create() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.hInstance = instance_;
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = L"SalesMonitoringMainWindow";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(0, wc.lpszClassName, L"SalesFlow", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                            CW_USEDEFAULT, CW_USEDEFAULT, 1560, 960, nullptr, nullptr, instance_, this);
    return hwnd_ != nullptr;
}

int MainWindow::Run() {
    ShowWindow(hwnd_, SW_SHOWMAXIMIZED);
    UpdateWindow(hwnd_);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    if (!self) return DefWindowProcW(hwnd, message, wParam, lParam);

    switch (message) {
    case WM_CREATE:
        self->OnCreate();
        return 0;
    case WM_SIZE:
        self->OnSize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_PAINT:
        self->OnPaint();
        return 0;
    case WM_DPICHANGED:
        self->OnDpiChanged(wParam, lParam);
        return 0;
    case WM_APP_SETTINGS_CHANGED:
        self->OnSettingsChanged();
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_COMMAND:
        self->OnCommand(wParam, lParam);
        return 0;
    case WM_DRAWITEM:
        self->OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        return TRUE;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && self->IsNavButton(reinterpret_cast<HWND>(wParam))) {
            SetCursor(LoadCursorW(nullptr, IDC_HAND));
            return TRUE;
        }
        break;
    case WM_CTLCOLORSTATIC: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        HWND ctl = reinterpret_cast<HWND>(lParam);
        SetBkMode(hdc, TRANSPARENT);
        if (ctl == self->clockLabel_) {
            SetTextColor(hdc, theme::kTextPrimary);
            return reinterpret_cast<INT_PTR>(gHeaderBrush);
        }
        break;
    }
    case WM_TIMER:
        self->UpdateHeader();
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, kClockTimerId);
        data::Repository::Instance().LogLogout();
        if (gHeaderBrush) {
            DeleteObject(gHeaderBrush);
            gHeaderBrush = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void MainWindow::OnCreate() {
    ui::EnableDoubleBuffering(hwnd_);
    if (!data::Repository::Instance().Initialize()) {
        MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Database Initialization", MB_OK | MB_ICONWARNING);
    }
    ApplyPersistedSettings();
    if (!gHeaderBrush) gHeaderBrush = CreateSolidBrush(theme::kHeaderBackground);
    CreateHeader();
    CreatePages();
    CreateSidebar();
    RefreshLocalizedText();
    SetTimer(hwnd_, kClockTimerId, 1000, nullptr);
    SwitchPage(access::StartupPage(data::Repository::Instance().CurrentAccount()));
}

void MainWindow::CreateSidebar() {
    const auto account = data::Repository::Instance().CurrentAccount();
    const auto role = access::RoleForAccount(account);
    std::wstring previousSection;
    for (const auto& spec : kMenuSpecs) {
        if (!CanShowMenuItem(account, spec)) {
            continue;
        }
        std::wstring navLabel = access::PageNavLabel(role, spec.pageIndex);
        if (navLabel.empty()) {
            navLabel = spec.title;
        }
        NavItem item{};
        item.id = kNavBaseId + static_cast<int>(navItems_.size());
        item.pageIndex = spec.pageIndex;
        item.text = navLabel;
        item.section = spec.section;
        item.iconIndex = spec.iconIndex;
        item.isPlaceholder = spec.isPlaceholder;
        item.bottomAligned = spec.bottomAligned;
        item.startsSection = item.section != previousSection;
        item.button = ui::CreateUiButton(hwnd_, item.id, item.text, ui::ButtonKind::Navigation);
        ui::SetButtonIconIndex(item.button, item.iconIndex);
        navItems_.push_back(item);
        previousSection = spec.section;
    }
    UpdateNavigationBadges();
}

void MainWindow::CreateHeader() {
    clockLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT, 0, 0, 10, 10, hwnd_, nullptr, instance_, nullptr);
    ui::AssignFontRole(clockLabel_, ui::FontRole::BodyBold);
    refreshButton_ = ui::CreateUiButton(hwnd_, kHeaderRefreshId, L"Refresh", ui::ButtonKind::Secondary);
    exportButton_ = ui::CreateUiButton(hwnd_, kHeaderExportId, L"Export Report", ui::ButtonKind::Primary);
}

void MainWindow::CreatePages() {
    pages_.push_back(std::make_unique<DashboardPage>());
    pages_.push_back(std::make_unique<SalesPage>());
    pages_.push_back(std::make_unique<OrdersPage>());
    pages_.push_back(std::make_unique<ProductsPage>());
    pages_.push_back(std::make_unique<ClientsPage>());
    pages_.push_back(std::make_unique<EmployeesPage>());
    pages_.push_back(std::make_unique<AnalyticsPage>());
    pages_.push_back(std::make_unique<ReportsPage>());
    pages_.push_back(std::make_unique<SettingsPage>());
    pages_.push_back(std::make_unique<AuditLogPage>());
    pages_.push_back(std::make_unique<UsersRolesPage>());
    pages_.push_back(std::make_unique<NotificationsPage>());
    pages_.push_back(std::make_unique<ProfilePage>());
    for (auto& page : pages_) page->Create(hwnd_, instance_);
}

void MainWindow::OnSize(int, int) {
    RECT client{};
    GetClientRect(hwnd_, &client);
    LayoutSidebar(client);
    LayoutHeader(client);
    LayoutPages(client);
    LayoutStatusBar(client);
}

void MainWindow::LayoutSidebar(const RECT&) {
    RECT client{};
    GetClientRect(hwnd_, &client);
    const int sidebarWidth = SidebarWidth();
    const int brandHeight = ui::Scale(94);
    const int menuTop = brandHeight + ui::Scale(14);
    const int sidebarBottom = client.bottom - StatusHeight();
    const int profileHeight = ui::Scale(74);
    const int profileTop = sidebarBottom - profileHeight - ui::Scale(14);
    const int settingsHeight = ui::Scale(44);
    const int settingsY = profileTop - settingsHeight - ui::Scale(14);
    const int menuBottom = settingsY - ui::Scale(12);
    const int buttonX = ui::Scale(12);
    const int buttonWidth = std::max(0, sidebarWidth - ui::Scale(24));

    int itemHeight = ui::Scale(44);
    int sectionHeight = ui::Scale(22);
    int itemGap = ui::Scale(4);
    int normalItems = 0;
    int normalSections = 0;
    for (const auto& item : navItems_) {
        if (item.bottomAligned) {
            continue;
        }
        ++normalItems;
        if (item.startsSection) {
            ++normalSections;
        }
    }

    const int needed = normalItems * itemHeight + std::max(0, normalItems - 1) * itemGap + normalSections * sectionHeight;
    const int available = std::max(0, menuBottom - menuTop);
    if (needed > available) {
        sectionHeight = ui::Scale(14);
        itemGap = ui::Scale(1);
        const int fixedSpace = normalSections * sectionHeight + std::max(0, normalItems - 1) * itemGap;
        itemHeight = normalItems > 0
            ? std::max(ui::Scale(28), (available - fixedSpace) / normalItems)
            : ui::Scale(28);
    }

    int y = menuTop;
    for (auto& item : navItems_) {
        item.rect = RECT{};
        item.sectionRect = RECT{};
        if (item.bottomAligned) {
            if (item.startsSection) {
                item.sectionRect = RECT{ buttonX + ui::Scale(6), settingsY - sectionHeight - ui::Scale(6), sidebarWidth - buttonX, settingsY - ui::Scale(6) };
            }
            item.rect = RECT{ buttonX, settingsY, buttonX + buttonWidth, settingsY + settingsHeight };
        } else {
            if (item.startsSection) {
                item.sectionRect = RECT{ buttonX + ui::Scale(6), y, sidebarWidth - buttonX, y + sectionHeight };
                y += sectionHeight;
            }
            item.rect = RECT{ buttonX, y, buttonX + buttonWidth, y + itemHeight };
            y += itemHeight + itemGap;
        }
        MoveWindow(item.button, item.rect.left, item.rect.top, ui::Width(item.rect), ui::Height(item.rect), TRUE);
    }
}

void MainWindow::LayoutHeader(const RECT& client) {
    RECT clockRect = HeaderClockRect(client);
    MoveWindow(clockLabel_, clockRect.left, clockRect.top, ui::Width(clockRect), ui::Height(clockRect), TRUE);
    const bool dashboard = currentPage_ == access::kPageDashboard;
    const auto account = data::Repository::Instance().CurrentAccount();
    const bool canExportDashboard = access::HasPermission(account, access::Permission::ExportReports)
        || access::HasPermission(account, access::Permission::ExportSales);
    ShowWindow(refreshButton_, dashboard ? SW_SHOW : SW_HIDE);
    ShowWindow(exportButton_, (dashboard && canExportDashboard) ? SW_SHOW : SW_HIDE);
    if (dashboard) {
        const int buttonHeight = ui::Scale(38);
        const int exportWidth = ui::Scale(138);
        const int refreshWidth = ui::Scale(104);
        const int gap = ui::Scale(10);
        const int top = ui::Scale(38);
        const int exportRight = clockRect.left - ui::Scale(18);
        RECT exportRc{ exportRight - exportWidth, top, exportRight, top + buttonHeight };
        RECT refreshRc = canExportDashboard
            ? RECT{ exportRc.left - gap - refreshWidth, top, exportRc.left - gap, top + buttonHeight }
            : RECT{ exportRight - refreshWidth, top, exportRight, top + buttonHeight };
        ui::MoveWindowToRect(refreshButton_, refreshRc);
        ui::MoveWindowToRect(exportButton_, exportRc);
    }
}

void MainWindow::LayoutPages(const RECT& client) {
    RECT pageRc{ SidebarWidth() + ShellMargin(), HeaderHeight() + ui::Scale(18), client.right - ShellMargin(), client.bottom - StatusHeight() - ui::Scale(8) };
    for (const auto& page : pages_) page->Resize(pageRc);
}

void MainWindow::LayoutStatusBar(const RECT& client) {
    if (statusBar_) {
        ShowWindow(statusBar_, SW_HIDE);
        MoveWindow(statusBar_, 0, client.bottom, client.right, 0, TRUE);
    }
}

void MainWindow::OnPaint() {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd_, &ps);
    RECT client{};
    GetClientRect(hwnd_, &client);
    ui::FillRectColor(hdc, client, theme::kWindowBackground);

    const int sidebarWidth = SidebarWidth();
    RECT sidebar{ 0, 0, sidebarWidth, client.bottom };
    ui::FillRectColor(hdc, sidebar, theme::kSidebarBackground);
    RECT sidebarBorder{ sidebarWidth - 1, 0, sidebarWidth, client.bottom };
    ui::FillRectColor(hdc, sidebarBorder, RGB(30, 41, 59));

    RECT header = HeaderRect(client);
    ui::DrawRoundedPanel(hdc, header, theme::kHeaderBackground, theme::kPanelBorder, ui::Scale(18), true);

    RECT logo{ ui::Scale(18), ui::Scale(20), ui::Scale(58), ui::Scale(60) };
    ui::DrawRoundedPanel(hdc, logo, RGB(37, 99, 235), RGB(37, 99, 235), ui::Scale(8), false);
    ui::DrawTextLine(hdc, L"SF", logo, theme::BodyBoldFont(), RGB(255, 255, 255), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    ui::DrawTextLine(hdc, L"SalesFlow", ui::MakeRect(ui::Scale(70), ui::Scale(20), sidebarWidth - ui::Scale(18), ui::Scale(44)), theme::BodyBoldFont(), RGB(255, 255, 255), DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    ui::DrawTextLine(hdc, L"Sales analytics", ui::MakeRect(ui::Scale(70), ui::Scale(45), sidebarWidth - ui::Scale(18), ui::Scale(66)), theme::SmallFont(), RGB(148, 163, 184), DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    RECT brandSeparator{ ui::Scale(16), ui::Scale(86), sidebarWidth - ui::Scale(16), ui::Scale(87) };
    ui::FillRectColor(hdc, brandSeparator, RGB(30, 41, 59));

    for (const auto& item : navItems_) {
        if (item.startsSection && ui::Height(item.sectionRect) > 0) {
            ui::DrawTextLine(hdc, item.section, item.sectionRect, theme::SmallBoldFont(), RGB(100, 116, 139),
                DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        }
    }

    const int sidebarBottom = client.bottom - StatusHeight();
    RECT profile{ ui::Scale(12), sidebarBottom - ui::Scale(88), sidebarWidth - ui::Scale(12), sidebarBottom - ui::Scale(14) };
    ui::DrawRoundedPanel(hdc, profile, RGB(15, 23, 42), RGB(30, 41, 59), ui::Scale(8), false);
    const auto account = data::Repository::Instance().CurrentAccount();
    const std::wstring displayName = account.fullName.empty() ? account.username : account.fullName;
    const std::wstring displayRole = account.role.empty() ? access::CanonicalRoleName(access::RoleForAccount(account)) : account.role;
    RECT avatar{ profile.left + ui::Scale(10), profile.top + ui::Scale(17), profile.left + ui::Scale(46), profile.top + ui::Scale(53) };
    ui::DrawRoundedPanel(hdc, avatar, RGB(37, 99, 235), RGB(37, 99, 235), ui::Scale(8), false);
    ui::DrawTextLine(hdc, L"SF", avatar, theme::SmallBoldFont(), RGB(255, 255, 255), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    ui::DrawTextLine(hdc, displayName, ui::MakeRect(avatar.right + ui::Scale(10), profile.top + ui::Scale(14), profile.right - ui::Scale(10), profile.top + ui::Scale(38)),
        theme::SmallBoldFont(), RGB(255, 255, 255), DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    ui::DrawTextLine(hdc, displayRole, ui::MakeRect(avatar.right + ui::Scale(10), profile.top + ui::Scale(38), profile.right - ui::Scale(10), profile.top + ui::Scale(62)),
        theme::SmallFont(), RGB(148, 163, 184), DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    if (!pages_.empty()) {
        const int titleLeft = sidebarWidth + ui::Scale(34);
        const int titleRight = currentPage_ == access::kPageDashboard ? client.right - ui::Scale(520) : client.right - ui::Scale(330);
        const auto role = access::RoleForAccount(data::Repository::Instance().CurrentAccount());
        const bool systemDashboard = currentPage_ == access::kPageDashboard
            && role == access::AppRole::SystemAdministrator
            && !access::HasPermission(data::Repository::Instance().CurrentAccount(), access::Permission::SalesView)
            && !access::HasPermission(data::Repository::Instance().CurrentAccount(), access::Permission::OrdersView)
            && !access::HasPermission(data::Repository::Instance().CurrentAccount(), access::Permission::InventoryView)
            && !access::HasPermission(data::Repository::Instance().CurrentAccount(), access::Permission::CustomersView)
            && !access::HasPermission(data::Repository::Instance().CurrentAccount(), access::Permission::AnalyticsView);
        const std::wstring subtitle = currentPage_ == access::kPageDashboard
            ? (systemDashboard ? L"System health, access control and security overview"
                               : L"Business performance, sales activity and operational overview")
            : pages_[currentPage_]->Subtitle();
        ui::DrawTextLine(hdc, LocalizedPageTitle(currentPage_), ui::MakeRect(titleLeft, ui::Scale(28), titleRight, ui::Scale(62)),
            theme::TitleFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        ui::DrawTextLine(hdc, subtitle, ui::MakeRect(titleLeft, ui::Scale(62), titleRight, ui::Scale(86)),
            theme::SmallFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    }

    EndPaint(hwnd_, &ps);
}

void MainWindow::OnCommand(WPARAM wParam, LPARAM) {
    int id = LOWORD(wParam);
    if (id == kHeaderRefreshId) {
        RefreshCurrentPage();
        return;
    }
    if (id == kHeaderExportId) {
        ExportDashboardReport();
        return;
    }
    for (const auto& item : navItems_) {
        if (item.id == id) {
            SwitchPage(item.pageIndex);
            return;
        }
    }
}

void MainWindow::OnDrawItem(const DRAWITEMSTRUCT* dis) {
    int id = static_cast<int>(dis->CtlID);
    bool active = false;
    std::wstring badge;
    for (const auto& item : navItems_) {
        if (item.id == id) {
            active = item.pageIndex == currentPage_;
            badge = item.badge;
            break;
        }
    }
    ui::DrawUiButton(dis, ui::GetButtonKind(dis->hwndItem), active);
    if (!badge.empty()) {
        RECT rc = dis->rcItem;
        RECT badgeRc{ rc.right - ui::Scale(48), rc.top + ui::Scale(11), rc.right - ui::Scale(12), rc.bottom - ui::Scale(11) };
        ui::DrawRoundedPanel(dis->hDC, badgeRc, RGB(30, 41, 59), RGB(59, 130, 246), ui::Scale(8), false);
        ui::DrawTextLine(dis->hDC, badge, badgeRc, theme::SmallBoldFont(), RGB(191, 219, 254), DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }
}

void MainWindow::OnSettingsChanged() {
    ApplyPersistedSettings();
    RefreshThemeResources();
    RefreshLocalizedText();
    ui::RefreshWindowFonts(hwnd_);

    RECT client{};
    GetClientRect(hwnd_, &client);
    OnSize(client.right, client.bottom);
    for (const auto& page : pages_) {
        InvalidateRect(page->Handle(), nullptr, TRUE);
    }
    InvalidateRect(hwnd_, nullptr, TRUE);
}

void MainWindow::ApplyPersistedSettings() {
    const auto settings = data::Repository::Instance().LoadSettings();
    theme::ApplyTheme(settings.theme);
    i18n::SetLanguage(settings.language);
}

void MainWindow::RefreshLocalizedText() {
    SetWindowTextW(hwnd_, i18n::Text(L"app.windowTitle").c_str());
    for (auto& item : navItems_) {
        SetWindowTextW(item.button, item.text.c_str());
        InvalidateRect(item.button, nullptr, TRUE);
    }
    if (statusBar_) {
        const std::wstring status = i18n::Text(L"status.demo");
        SendMessageW(statusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(status.c_str()));
    }
}

void MainWindow::RefreshThemeResources() {
    if (gHeaderBrush) {
        DeleteObject(gHeaderBrush);
    }
    gHeaderBrush = CreateSolidBrush(theme::kHeaderBackground);

    EnumChildWindows(hwnd_, [](HWND child, LPARAM) -> BOOL {
        wchar_t className[64]{};
        GetClassNameW(child, className, 64);
        if (std::wcscmp(className, WC_LISTVIEWW) == 0) {
            ui::ApplyStandardTableStyle(child);
        } else {
            InvalidateRect(child, nullptr, TRUE);
        }
        return TRUE;
    }, 0);
}

std::wstring MainWindow::LocalizedPageTitle(int index) const {
    if (index == access::kPageProducts) {
        return L"Inventory";
    }
    if (index == access::kPageClients) {
        return L"Customers";
    }
    const auto role = access::RoleForAccount(data::Repository::Instance().CurrentAccount());
    const std::wstring roleTitle = access::PageTitle(role, index);
    if (!roleTitle.empty()) {
        return roleTitle;
    }
    if (index >= 0 && index < static_cast<int>(kPageTitleKeys.size())) {
        return i18n::Text(kPageTitleKeys[static_cast<size_t>(index)]);
    }
    return !pages_.empty() ? pages_[currentPage_]->Title() : L"";
}

void MainWindow::SwitchPage(int index) {
    if (index < 0 || index >= static_cast<int>(pages_.size())) return;
    if (!access::CanOpenPage(data::Repository::Instance().CurrentAccount(), index)) {
        const std::wstring page = LocalizedPageTitle(index);
        data::Repository::Instance().LogAccessDenied(L"navigation", L"Open page denied: " + page);
        MessageBoxW(hwnd_, L"Access denied. You do not have permission to view this section.", page.c_str(), MB_OK | MB_ICONWARNING);
        return;
    }
    currentPage_ = index;
    for (size_t i = 0; i < pages_.size(); ++i) pages_[i]->Show(static_cast<int>(i) == currentPage_);
    pages_[currentPage_]->Activate();
    UpdateNavigationBadges();
    for (const auto& item : navItems_) InvalidateRect(item.button, nullptr, TRUE);
    UpdateHeader();
    RECT client{};
    GetClientRect(hwnd_, &client);
    RECT headerRect = HeaderRect(client);
    InvalidateRect(hwnd_, &headerRect, FALSE);
    RECT sidebarRect{ 0, 0, SidebarWidth(), client.bottom };
    InvalidateRect(hwnd_, &sidebarRect, FALSE);
    LayoutHeader(client);
}

bool MainWindow::IsNavButton(HWND hwnd) const {
    return std::any_of(navItems_.begin(), navItems_.end(), [hwnd](const NavItem& item) {
        return item.button == hwnd;
    });
}

void MainWindow::RefreshCurrentPage() {
    if (currentPage_ >= 0 && currentPage_ < static_cast<int>(pages_.size())) {
        pages_[currentPage_]->Activate();
        UpdateNavigationBadges();
        InvalidateRect(pages_[currentPage_]->Handle(), nullptr, FALSE);
    }
}

void MainWindow::ExportDashboardReport() {
    const std::wstring path = data::Repository::Instance().ExportDailySalesSummaryReport();
    if (path.empty()) {
        MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Export Report", MB_OK | MB_ICONWARNING);
        return;
    }
    MessageBoxW(hwnd_, (L"Report exported:\n" + path).c_str(), L"Export Report", MB_OK | MB_ICONINFORMATION);
}

void MainWindow::UpdateHeader() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &t);
    wchar_t buffer[128]{};
    wcsftime(buffer, 128, L"%d %b %Y  |  %H:%M:%S", &tm);
    SetWindowTextW(clockLabel_, buffer);
}

void MainWindow::UpdateNavigationBadges() {
    int openTasks = 0;
    if (data::Repository::Instance().HasCurrentAccount()
        && access::CanOpenPage(data::Repository::Instance().CurrentAccount(), access::kPageNotifications)) {
        openTasks = static_cast<int>(data::Repository::Instance().LoadOpenTasks().size());
    }

    for (auto& item : navItems_) {
        if (item.pageIndex == access::kPageNotifications && openTasks > 0) {
            item.badge = openTasks > 99 ? L"99+" : std::to_wstring(openTasks);
        } else {
            item.badge.clear();
        }
        InvalidateRect(item.button, nullptr, TRUE);
    }
}

void MainWindow::OnDpiChanged(WPARAM wParam, LPARAM lParam) {
    const UINT dpi = HIWORD(wParam);
    theme::SetDpi(dpi);
    ui::RefreshWindowFonts(hwnd_);
    if (const RECT* suggested = reinterpret_cast<const RECT*>(lParam)) {
        SetWindowPos(hwnd_, nullptr, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
    RECT client{};
    GetClientRect(hwnd_, &client);
    OnSize(client.right, client.bottom);
    InvalidateRect(hwnd_, nullptr, TRUE);
}
