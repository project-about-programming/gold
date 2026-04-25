#include "MainWindow.h"

#include "AccessControl.h"
#include "AnalyticsPage.h"
#include "AppMessages.h"
#include "ClientsPage.h"
#include "DataRepository.h"
#include "DashboardPage.h"
#include "EmployeesPage.h"
#include "Localization.h"
#include "OrdersPage.h"
#include "ProductsPage.h"
#include "ReportsPage.h"
#include "SalesPage.h"
#include "SettingsPage.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <commctrl.h>
#include <array>
#include <cwchar>
#include <ctime>

namespace {
constexpr int kNavBaseId = 1000;
constexpr UINT_PTR kClockTimerId = 42;
HBRUSH gHeaderBrush = nullptr;

int SidebarWidth() { return ui::Scale(248); }
int HeaderHeight() { return ui::Scale(96); }
int StatusHeight() { return ui::Scale(24); }
int ShellMargin() { return ui::GetMetrics().outerPadding; }
RECT HeaderRect(const RECT& client) {
    return ui::MakeRect(SidebarWidth() + ShellMargin(), ShellMargin(), client.right - ShellMargin(), HeaderHeight());
}
RECT HeaderClockRect(const RECT& client) {
    return ui::MakeRect(client.right - ui::Scale(252), ui::Scale(38), client.right - ui::Scale(36), ui::Scale(70));
}

constexpr std::array<const wchar_t*, 9> kNavKeys = {
    L"nav.dashboard", L"nav.sales", L"nav.orders", L"nav.products", L"nav.clients", L"nav.employees", L"nav.analytics", L"nav.reports", L"nav.settings"
};

constexpr std::array<const wchar_t*, 9> kPageTitleKeys = {
    L"page.dashboard.title", L"page.sales.title", L"page.orders.title", L"page.products.title", L"page.clients.title", L"page.employees.title", L"page.analytics.title", L"page.reports.title", L"page.settings.title"
};
}

MainWindow::MainWindow(HINSTANCE instance)
    : instance_(instance), hwnd_(nullptr), clockLabel_(nullptr), statusBar_(nullptr), currentPage_(0) {}

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
    statusBar_ = CreateWindowExW(0, STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    SendMessageW(statusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(i18n::Text(L"status.demo").c_str()));
    CreatePages();
    CreateSidebar();
    RefreshLocalizedText();
    SetTimer(hwnd_, kClockTimerId, 1000, nullptr);
    SwitchPage(access::kPageDashboard);
}

void MainWindow::CreateSidebar() {
    const auto account = data::Repository::Instance().CurrentAccount();
    for (int i = 0; i < 9; ++i) {
        if (!access::CanOpenPage(account, i)) {
            continue;
        }
        NavItem item{};
        item.id = kNavBaseId + static_cast<int>(navItems_.size());
        item.pageIndex = i;
        item.text = i18n::Text(kNavKeys[static_cast<size_t>(i)]);
        item.button = ui::CreateUiButton(hwnd_, item.id, item.text, ui::ButtonKind::Navigation);
        navItems_.push_back(item);
    }
}

void MainWindow::CreateHeader() {
    clockLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT, 0, 0, 10, 10, hwnd_, nullptr, instance_, nullptr);
    ui::AssignFontRole(clockLabel_, ui::FontRole::BodyBold);
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
    const int top = ui::Scale(104);
    int gap = ui::Scale(10);
    int height = ui::Scale(42);
    const int settingsY = client.bottom - StatusHeight() - ui::Scale(62);
    int regularCount = 0;
    for (const auto& item : navItems_) {
        if (item.pageIndex != access::kPageSettings) {
            ++regularCount;
        }
    }
    const int available = std::max(0, settingsY - top - ui::Scale(12));
    const int needed = regularCount > 0 ? regularCount * height + (regularCount - 1) * gap : 0;
    if (regularCount > 0 && needed > available) {
        gap = std::max(ui::Scale(4), std::min(gap, available / std::max(regularCount * 5, 1)));
        height = std::max(ui::Scale(30), (available - (regularCount - 1) * gap) / regularCount);
    }
    int visibleIndex = 0;
    for (size_t i = 0; i < navItems_.size(); ++i) {
        int y = top + visibleIndex * (height + gap);
        if (navItems_[i].pageIndex == access::kPageSettings) {
            y = settingsY;
        } else {
            ++visibleIndex;
        }
        MoveWindow(navItems_[i].button, ui::Scale(18), y, std::max(0, sidebarWidth - ui::Scale(36)), height, TRUE);
    }
}

void MainWindow::LayoutHeader(const RECT& client) {
    RECT clockRect = HeaderClockRect(client);
    MoveWindow(clockLabel_, clockRect.left, clockRect.top, ui::Width(clockRect), ui::Height(clockRect), TRUE);
}

void MainWindow::LayoutPages(const RECT& client) {
    RECT pageRc{ SidebarWidth() + ShellMargin(), HeaderHeight() + ui::Scale(18), client.right - ShellMargin(), client.bottom - StatusHeight() - ui::Scale(8) };
    for (const auto& page : pages_) page->Resize(pageRc);
}

void MainWindow::LayoutStatusBar(const RECT& client) {
    MoveWindow(statusBar_, 0, client.bottom - StatusHeight(), client.right, StatusHeight(), TRUE);
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

    RECT header = HeaderRect(client);
    ui::DrawRoundedPanel(hdc, header, theme::kHeaderBackground, theme::kPanelBorder, ui::Scale(18), true);

    RECT logo{ ui::Scale(20), ui::Scale(24), ui::Scale(58), ui::Scale(62) };
    ui::DrawRoundedPanel(hdc, logo, theme::kAccent, RGB(89, 137, 208), ui::Scale(12), false);
    ui::DrawTextLine(hdc, L"SF", logo, theme::BodyBoldFont(), RGB(255, 255, 255), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    ui::DrawTextLine(hdc, i18n::Text(L"app.brand"), ui::MakeRect(ui::Scale(70), ui::Scale(28), sidebarWidth - ui::Scale(18), ui::Scale(58)), theme::BodyBoldFont(), RGB(255, 255, 255), DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    ui::DrawTextLine(hdc, L"Sales analytics", ui::MakeRect(ui::Scale(70), ui::Scale(54), sidebarWidth - ui::Scale(18), ui::Scale(78)), theme::SmallFont(), RGB(166, 178, 195), DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    if (!pages_.empty()) {
        const int titleLeft = sidebarWidth + ui::Scale(34);
        ui::DrawTextLine(hdc, LocalizedPageTitle(currentPage_), ui::MakeRect(titleLeft, ui::Scale(35), client.right - ui::Scale(330), ui::Scale(72)), theme::TitleFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    }

    EndPaint(hwnd_, &ps);
}

void MainWindow::OnCommand(WPARAM wParam, LPARAM) {
    int id = LOWORD(wParam);
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
    for (const auto& item : navItems_) {
        if (item.id == id) {
            active = item.pageIndex == currentPage_;
            break;
        }
    }
    ui::DrawUiButton(dis, ui::GetButtonKind(dis->hwndItem), active);
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
    const auto role = access::RoleForAccount(data::Repository::Instance().CurrentAccount());
    for (auto& item : navItems_) {
        std::wstring label = access::PageNavLabel(role, item.pageIndex);
        if (label.empty() && item.pageIndex >= 0 && item.pageIndex < static_cast<int>(kNavKeys.size())) {
            label = i18n::Text(kNavKeys[static_cast<size_t>(item.pageIndex)]);
        }
        item.text = label;
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
            ListView_SetBkColor(child, theme::kPanelBackground);
            ListView_SetTextBkColor(child, theme::kPanelBackground);
            ListView_SetTextColor(child, theme::kTextPrimary);
            InvalidateRect(child, nullptr, TRUE);
        } else {
            InvalidateRect(child, nullptr, TRUE);
        }
        return TRUE;
    }, 0);
}

std::wstring MainWindow::LocalizedPageTitle(int index) const {
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
        for (const auto& item : navItems_) {
            if (access::CanOpenPage(data::Repository::Instance().CurrentAccount(), item.pageIndex)) {
                index = item.pageIndex;
                break;
            }
        }
    }
    currentPage_ = index;
    for (size_t i = 0; i < pages_.size(); ++i) pages_[i]->Show(static_cast<int>(i) == currentPage_);
    pages_[currentPage_]->Activate();
    for (const auto& item : navItems_) InvalidateRect(item.button, nullptr, TRUE);
    UpdateHeader();
    RECT client{};
    GetClientRect(hwnd_, &client);
    RECT headerRect = HeaderRect(client);
    InvalidateRect(hwnd_, &headerRect, FALSE);
}

void MainWindow::UpdateHeader() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_s(&tm, &t);
    wchar_t buffer[128]{};
    wcsftime(buffer, 128, L"%d %b %Y  |  %H:%M:%S", &tm);
    SetWindowTextW(clockLabel_, buffer);
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
