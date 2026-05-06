#include "ProfilePage.h"

#include "AccessControl.h"
#include "DataRepository.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <string>
#include <vector>

namespace {

struct ProfileLayout {
    RECT title{};
    RECT identity{};
    RECT role{};
    RECT pages{};
    RECT actions{};
};

ProfileLayout BuildLayout(const RECT& client) {
    ProfileLayout layout{};
    const auto m = ui::GetMetrics();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);

    RECT topRow = ui::TakeTop(&area, ui::Scale(178), m.sectionGap);
    const auto topColumns = ui::SplitColumns(topRow, 2, m.sectionGap);
    layout.identity = topColumns[0];
    layout.role = topColumns[1];

    const auto bottomColumns = ui::SplitColumns(area, 2, m.sectionGap);
    layout.pages = bottomColumns[0];
    layout.actions = bottomColumns[1];
    return layout;
}

std::wstring RoleMission(access::AppRole role) {
    switch (role) {
    case access::AppRole::SuperAdmin:
        return L"Full recovery and demonstration access. Use carefully.";
    case access::AppRole::SystemAdministrator:
        return L"Keep accounts, roles, audit log, database and system settings healthy.";
    case access::AppRole::Director:
        return L"Monitor company performance, review analytics and export executive reports.";
    case access::AppRole::SalesSupervisor:
        return L"Control team sales, manager workload, tasks and operational reports.";
    case access::AppRole::SalesManager:
        return L"Work with own clients, create sales and track personal tasks.";
    case access::AppRole::Cashier:
        return L"Register fast sales, create orders and work only with operational data.";
    case access::AppRole::ProductManager:
        return L"Maintain catalog, prices, product status, import/export and stock risks.";
    case access::AppRole::WarehouseWorker:
        return L"Control stock receiving, write-offs, inventory and expiry warnings.";
    case access::AppRole::Accountant:
        return L"Check sales totals, revenue reports, returns and export finance summaries.";
    case access::AppRole::Analyst:
        return L"Review trends, analytics, reports and read-only business data.";
    }
    return L"Use the allowed modules shown below.";
}

std::vector<std::wstring> RecommendedWork(access::AppRole role) {
    switch (role) {
    case access::AppRole::SystemAdministrator:
        return {L"Review Users & Roles before giving access", L"Check Audit Log for denied actions", L"Use Settings for database, backup and theme", L"Watch System Alerts"};
    case access::AppRole::Director:
        return {L"Start from Executive Dashboard", L"Use Analytics for trends", L"Generate Reports for presentation", L"Open Sales/Customers in read-only mode"};
    case access::AppRole::SalesSupervisor:
        return {L"Check Team Dashboard", L"Review Team Sales and Orders", L"Create tasks in Notifications", L"Export operational reports"};
    case access::AppRole::SalesManager:
        return {L"Create sales from My Sales", L"Maintain own customers", L"Use Inventory before selling", L"Close assigned tasks"};
    case access::AppRole::Cashier:
        return {L"Create quick sales", L"Track active orders", L"Lookup customer profiles", L"Close own tasks"};
    case access::AppRole::ProductManager:
        return {L"Add and edit inventory items", L"Import/export inventory", L"Monitor low stock", L"Review expiry reports"};
    case access::AppRole::WarehouseWorker:
        return {L"Use Stock In / Stock Out", L"Watch expiry and low stock", L"Update order status when shipped", L"Export stock reports"};
    case access::AppRole::Accountant:
        return {L"Open Reports first", L"Export sales and profit reports", L"Check returns/cancellations", L"Review revenue analytics"};
    case access::AppRole::Analyst:
        return {L"Open Analytics first", L"Compare reports and trends", L"Use read-only business pages", L"Export analysis reports"};
    case access::AppRole::SuperAdmin:
        return {L"Recover broken access only when needed", L"Validate every role", L"Keep destructive actions intentional", L"Use Audit Log after changes"};
    }
    return {};
}

std::vector<std::wstring> AvailablePages(const data::AccountRecord& account) {
    struct PageSpec {
        int page;
        const wchar_t* label;
    };
    const PageSpec specs[] = {
        {access::kPageDashboard, L"Dashboard"},
        {access::kPageSales, L"Sales"},
        {access::kPageOrders, L"Orders"},
        {access::kPageClients, L"Customers"},
        {access::kPageProducts, L"Inventory"},
        {access::kPageEmployees, L"Employees"},
        {access::kPageAnalytics, L"Analytics"},
        {access::kPageReports, L"Reports"},
        {access::kPageUsersRoles, L"Users & Roles"},
        {access::kPageAuditLog, L"Audit Log"},
        {access::kPageNotifications, L"Notifications / Tasks"},
        {access::kPageSettings, L"Settings"},
        {access::kPageProfile, L"Profile"}
    };

    std::vector<std::wstring> pages;
    for (const auto& spec : specs) {
        if (access::CanOpenPage(account, spec.page)) {
            pages.push_back(spec.label);
        }
    }
    return pages;
}

std::vector<std::wstring> PermissionActions(const data::AccountRecord& account) {
    struct PermissionSpec {
        access::Permission permission;
        const wchar_t* label;
    };
    const PermissionSpec specs[] = {
        {access::Permission::CreateSale, L"Create sales"},
        {access::Permission::ChangeSaleStatus, L"Change sale status"},
        {access::Permission::ExportSales, L"Export sales"},
        {access::Permission::CreateOrder, L"Create orders"},
        {access::Permission::ChangeOrderStatus, L"Change order status"},
        {access::Permission::CreateClient, L"Create customers"},
        {access::Permission::EditClient, L"Edit customers"},
        {access::Permission::CreateProduct, L"Create inventory items"},
        {access::Permission::EditProduct, L"Edit inventory and stock"},
        {access::Permission::ImportExportCatalog, L"Import/export inventory"},
        {access::Permission::ExportReports, L"Export reports"},
        {access::Permission::ManageTasks, L"Create team tasks"},
        {access::Permission::ManageUsers, L"Manage users"},
        {access::Permission::ManageRoles, L"Manage roles"},
        {access::Permission::ViewAuditLog, L"View audit log"},
        {access::Permission::ManageSystemSettings, L"Change system settings"}
    };

    std::vector<std::wstring> actions;
    for (const auto& spec : specs) {
        if (access::HasPermission(account, spec.permission)) {
            actions.push_back(spec.label);
        }
    }
    return actions;
}

void DrawPanelTitle(HDC hdc, const RECT& panel, const std::wstring& title, const std::wstring& subtitle = L"") {
    ui::DrawRoundedPanel(hdc, panel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawTextLine(hdc, title, ui::MakeRect(panel.left + ui::Scale(18), panel.top + ui::Scale(16), panel.right - ui::Scale(18), panel.top + ui::Scale(42)),
        theme::BodyBoldFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    if (!subtitle.empty()) {
        ui::DrawTextLine(hdc, subtitle, ui::MakeRect(panel.left + ui::Scale(18), panel.top + ui::Scale(42), panel.right - ui::Scale(18), panel.top + ui::Scale(66)),
            theme::SmallFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    }
}

void DrawInfoLine(HDC hdc, int x, int y, int width, const std::wstring& label, const std::wstring& value) {
    ui::DrawTextLine(hdc, label, ui::MakeRect(x, y, x + ui::Scale(128), y + ui::Scale(24)),
        theme::SmallBoldFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    ui::DrawTextLine(hdc, value, ui::MakeRect(x + ui::Scale(136), y, x + width, y + ui::Scale(24)),
        theme::BodyFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
}

void DrawList(HDC hdc, const RECT& panel, const std::vector<std::wstring>& values) {
    int y = panel.top + ui::Scale(76);
    const int left = panel.left + ui::Scale(22);
    const int right = panel.right - ui::Scale(22);
    const int lineHeight = ui::Scale(25);
    for (const auto& value : values) {
        if (y + lineHeight > panel.bottom - ui::Scale(16)) {
            ui::DrawTextLine(hdc, L"...", ui::MakeRect(left, y, right, y + lineHeight),
                theme::BodyBoldFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
            return;
        }
        ui::DrawTextLine(hdc, L"-", ui::MakeRect(left, y, left + ui::Scale(18), y + lineHeight),
            theme::BodyBoldFont(), theme::kAccent, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        ui::DrawTextLine(hdc, value, ui::MakeRect(left + ui::Scale(22), y, right, y + lineHeight),
            theme::BodyFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        y += lineHeight;
    }
}

} // namespace

ProfilePage::ProfilePage()
    : PageBase(L"Profile", L"Current user, role workspace and available permissions.") {}

const wchar_t* ProfilePage::ClassName() const {
    return L"NativeProfilePage";
}

void ProfilePage::OnCreate() {}

void ProfilePage::OnActivate() {
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void ProfilePage::OnSize(int, int) {}

void ProfilePage::OnPaint(HDC hdc, const RECT& client) {
    const ProfileLayout layout = BuildLayout(client);
    const auto account = data::Repository::Instance().CurrentAccount();
    const auto role = access::RoleForAccount(account);

    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, L"Profile",
        L"Your account, role workspace and available functions.", ui::Width(layout.title));

    DrawPanelTitle(hdc, layout.identity, L"Account", L"Signed-in user identity");
    RECT avatar{ layout.identity.left + ui::Scale(18), layout.identity.top + ui::Scale(76), layout.identity.left + ui::Scale(78), layout.identity.top + ui::Scale(136) };
    ui::DrawRoundedPanel(hdc, avatar, theme::kAccent, theme::kAccent, ui::Scale(14), false);
    ui::DrawTextLine(hdc, L"SF", avatar, theme::TitleFont(), RGB(255, 255, 255), DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    const int infoLeft = avatar.right + ui::Scale(18);
    const int infoWidth = layout.identity.right - infoLeft - ui::Scale(18);
    DrawInfoLine(hdc, infoLeft, layout.identity.top + ui::Scale(74), infoWidth, L"Name", account.fullName.empty() ? account.username : account.fullName);
    DrawInfoLine(hdc, infoLeft, layout.identity.top + ui::Scale(102), infoWidth, L"Username", account.username);
    DrawInfoLine(hdc, infoLeft, layout.identity.top + ui::Scale(130), infoWidth, L"Email", account.email);

    DrawPanelTitle(hdc, layout.role, L"Role Workspace", access::RoleInterfaceName(role));
    ui::DrawTextLine(hdc, access::CanonicalRoleName(role), ui::MakeRect(layout.role.left + ui::Scale(18), layout.role.top + ui::Scale(78), layout.role.right - ui::Scale(18), layout.role.top + ui::Scale(108)),
        theme::TitleFont(), theme::kAccent, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    ui::DrawTextLine(hdc, RoleMission(role), ui::MakeRect(layout.role.left + ui::Scale(18), layout.role.top + ui::Scale(114), layout.role.right - ui::Scale(18), layout.role.bottom - ui::Scale(18)),
        theme::BodyFont(), theme::kTextSecondary, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);

    DrawPanelTitle(hdc, layout.pages, L"Available Sections", L"Navigation items visible for this role");
    DrawList(hdc, layout.pages, AvailablePages(account));

    DrawPanelTitle(hdc, layout.actions, L"Recommended Work", L"What this role should focus on");
    std::vector<std::wstring> work = RecommendedWork(role);
    const auto permissions = PermissionActions(account);
    if (!permissions.empty()) {
        work.push_back(L"Allowed actions: " + std::to_wstring(permissions.size()) + L" permission groups");
    }
    DrawList(hdc, layout.actions, work);
}
