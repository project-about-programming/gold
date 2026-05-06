#include "NotificationsPage.h"

#include "AccessControl.h"
#include "DataEntryDialog.h"
#include "DataRepository.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>

namespace {
enum : int { IDC_ADD_TASK = 11001, IDC_COMPLETE_TASK, IDC_REFRESH, IDC_TABLE };

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Р СѓСЃСЃРєРёР№") ? std::wstring(ru) : std::wstring(en);
}

struct NotificationsLayout {
    RECT title{};
    RECT kpis[4]{};
    RECT toolbar{};
    RECT addTask{};
    RECT complete{};
    RECT refresh{};
    RECT tablePanel{};
    RECT table{};
};

NotificationsLayout BuildLayout(const RECT& client) {
    NotificationsLayout layout{};
    const auto m = ui::GetMetrics();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);

    RECT kpiRow = ui::TakeTop(&area, m.kpiHeight, m.sectionGap);
    const auto kpis = ui::SplitColumns(kpiRow, 4, m.sectionGap);
    for (int index = 0; index < 4; ++index) {
        layout.kpis[index] = kpis[index];
    }

    layout.toolbar = ui::TakeTop(&area, ui::Scale(76), m.sectionGap);
    layout.tablePanel = area;

    RECT actions = ui::Inset(layout.toolbar, ui::Scale(16), ui::Scale(18));
    const int actionWidth = ui::IconButtonWidth();
    layout.refresh = ui::TakeRight(&actions, actionWidth, m.compactGap);
    layout.complete = ui::TakeRight(&actions, actionWidth, m.compactGap);
    layout.addTask = ui::TakeRight(&actions, actionWidth, m.compactGap);

    layout.table = ui::Inset(ui::MakeRect(layout.tablePanel.left, layout.tablePanel.top + m.panelHeaderHeight,
        layout.tablePanel.right, layout.tablePanel.bottom), ui::Scale(12), ui::Scale(12));
    return layout;
}

bool Can(access::Permission permission) {
    return access::HasPermission(data::Repository::Instance().CurrentAccount(), permission);
}

bool CanCompleteTasks() {
    return data::Repository::Instance().HasCurrentAccount()
        && access::TaskScope(access::RoleForAccount(data::Repository::Instance().CurrentAccount())) != access::DataScope::None;
}

void SetActionVisible(HWND hwnd, bool visible) {
    ShowWindow(hwnd, SW_SHOW);
    EnableWindow(hwnd, visible ? TRUE : FALSE);
}

} // namespace

NotificationsPage::NotificationsPage()
    : PageBase(L"Notifications", L"Tasks, stock risks, stalled deals and operational messages.") {}

const wchar_t* NotificationsPage::ClassName() const { return L"NativeNotificationsPage"; }

void NotificationsPage::OnCreate() {
    addTaskButton_ = ui::CreateUiButton(hwnd_, IDC_ADD_TASK, L"New Task", ui::ButtonKind::Primary);
    completeButton_ = ui::CreateUiButton(hwnd_, IDC_COMPLETE_TASK, L"Mark Done", ui::ButtonKind::Secondary);
    refreshButton_ = ui::CreateUiButton(hwnd_, IDC_REFRESH, L"Refresh", ui::ButtonKind::Secondary);
    notificationsTable_ = ui::CreateUiListView(hwnd_, IDC_TABLE);

    ui::AddListColumns(notificationsTable_,
        {UiText(L"Type", L"Тип"), UiText(L"Item", L"Событие"), UiText(L"Owner", L"Ответственный"),
         UiText(L"Due / Value", L"Срок / значение"), UiText(L"Priority", L"Приоритет"), UiText(L"Status", L"Статус")},
        {100, 300, 180, 150, 110, 120});

    ApplyAccess();
    ReloadData();
}

void NotificationsPage::OnActivate() {
    ApplyAccess();
    ReloadData();
}

void NotificationsPage::ApplyAccess() {
    SetActionVisible(addTaskButton_, Can(access::Permission::ManageTasks));
    SetActionVisible(completeButton_, CanCompleteTasks());
}

void NotificationsPage::ReloadData() {
    rows_.clear();

    for (const auto& task : data::Repository::Instance().LoadOpenTasks()) {
        rows_.push_back(NotificationRow{
            task.id,
            UiText(L"Task", L"Задача"),
            task.title,
            task.assignedTo,
            task.dueDateDisplay,
            i18n::DataText(task.priority),
            i18n::DataText(task.status)
        });
    }

    const auto account = data::Repository::Instance().CurrentAccount();
    if (access::HasPermission(account, access::Permission::OpenReports)
        || access::HasPermission(account, access::Permission::OpenProducts)) {
        const auto snapshot = data::Repository::Instance().LoadDashboardSnapshot();
        for (const auto& alert : snapshot.lowStock) {
            if (alert.size() < 5 || alert[0] == L"Task") {
                continue;
            }
            rows_.push_back(NotificationRow{
                0,
                i18n::DataText(alert[0]),
                alert[1],
                alert[2],
                alert[3],
                alert[4],
                alert[4]
            });
        }
    }

    ui::ClearListRows(notificationsTable_);
    for (size_t index = 0; index < rows_.size(); ++index) {
        const auto& row = rows_[index];
        ui::AddListRow(notificationsTable_, static_cast<int>(index), {
            row.type,
            row.item,
            row.owner,
            row.due,
            row.priority,
            row.status
        });
    }
    ShowWindow(notificationsTable_, rows_.empty() ? SW_HIDE : SW_SHOW);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

int NotificationsPage::SelectedRowIndex() const {
    return ListView_GetNextItem(notificationsTable_, -1, LVNI_SELECTED);
}

void NotificationsPage::OnSize(int width, int height) {
    const NotificationsLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    ui::MoveWindowToRect(addTaskButton_, layout.addTask);
    ui::MoveWindowToRect(completeButton_, layout.complete);
    ui::MoveWindowToRect(refreshButton_, layout.refresh);
    ui::MoveWindowToRect(notificationsTable_, layout.table);

    const int tableWidth = std::max(ui::Scale(760), ui::Width(layout.table) - ui::Scale(4));
    ListView_SetColumnWidth(notificationsTable_, 0, tableWidth * 10 / 100);
    ListView_SetColumnWidth(notificationsTable_, 1, tableWidth * 32 / 100);
    ListView_SetColumnWidth(notificationsTable_, 2, tableWidth * 18 / 100);
    ListView_SetColumnWidth(notificationsTable_, 3, tableWidth * 15 / 100);
    ListView_SetColumnWidth(notificationsTable_, 4, tableWidth * 12 / 100);
    ListView_SetColumnWidth(notificationsTable_, 5, tableWidth * 13 / 100);
}

void NotificationsPage::OnPaint(HDC hdc, const RECT& client) {
    const NotificationsLayout layout = BuildLayout(client);
    int tasks = 0;
    int highPriority = 0;
    int stockAlerts = 0;
    int blocked = 0;
    for (const auto& row : rows_) {
        tasks += row.taskId > 0 ? 1 : 0;
        highPriority += (row.priority == L"High" || row.priority == L"Высокий") ? 1 : 0;
        stockAlerts += (row.type == L"Stock" || row.type == L"Склад") ? 1 : 0;
        blocked += (row.status == L"Critical" || row.status == L"High" || row.status == L"Overdue") ? 1 : 0;
    }

    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top,
        UiText(L"Notifications", L"Уведомления"),
        UiText(L"Tasks, stock risks and operational alerts routed by role.", L"Задачи, складские риски и операционные уведомления по ролям."),
        ui::Width(layout.title));

    ui::DrawKpiCard(hdc, layout.kpis[0], UiText(L"Open Items", L"Открыто"), std::to_wstring(rows_.size()),
        UiText(L"Visible for current role", L"Доступно текущей роли"), theme::kBlue);
    ui::DrawKpiCard(hdc, layout.kpis[1], UiText(L"Tasks", L"Задачи"), std::to_wstring(tasks),
        UiText(L"Assigned or team tasks", L"Личные или командные задачи"), theme::kGreen);
    ui::DrawKpiCard(hdc, layout.kpis[2], UiText(L"Stock Alerts", L"Складские риски"), std::to_wstring(stockAlerts),
        UiText(L"Low stock and expiry watch", L"Остатки и сроки годности"), theme::kAmber);
    ui::DrawKpiCard(hdc, layout.kpis[3], UiText(L"Urgent", L"Срочно"), std::to_wstring(std::max(highPriority, blocked)),
        UiText(L"Needs attention", L"Требует внимания"), theme::kAccent);

    ui::DrawRoundedPanel(hdc, layout.toolbar, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawTextLine(hdc, UiText(L"Role-focused work queue", L"Рабочая очередь по роли"),
        ui::MakeRect(layout.toolbar.left + ui::Scale(18), layout.toolbar.top + ui::Scale(14), layout.toolbar.right - ui::Scale(420), layout.toolbar.top + ui::Scale(38)),
        theme::BodyBoldFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    ui::DrawTextLine(hdc, UiText(L"Managers can assign tasks; staff can close their own tasks; stock alerts are read-only.", L"Руководители назначают задачи, сотрудники закрывают свои, складские риски доступны для контроля."),
        ui::MakeRect(layout.toolbar.left + ui::Scale(18), layout.toolbar.top + ui::Scale(38), layout.toolbar.right - ui::Scale(420), layout.toolbar.top + ui::Scale(62)),
        theme::SmallFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    ui::DrawRoundedPanel(hdc, layout.tablePanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.tablePanel.left + ui::Scale(16), layout.tablePanel.top + ui::Scale(14),
        UiText(L"Notification Center", L"Центр уведомлений"),
        UiText(L"Operational messages are loaded from tasks, stock and sales monitoring data.", L"Операционные сообщения загружаются из задач, склада и мониторинга продаж."),
        ui::Width(layout.tablePanel) - ui::Scale(32));
    if (rows_.empty()) {
        ui::DrawEmptyState(hdc, layout.table,
            UiText(L"No active notifications", L"Активных уведомлений нет"),
            UiText(L"Tasks and operational alerts will appear here when action is needed.", L"Задачи и предупреждения появятся здесь, когда потребуется действие."),
            L"!");
    }
}

LRESULT NotificationsPage::OnCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);
    if (code != BN_CLICKED) {
        return 0;
    }

    if (id == IDC_REFRESH) {
        ReloadData();
        return 0;
    }

    if (id == IDC_ADD_TASK) {
        std::vector<ui::DataEntryField> fields{
            {UiText(L"Task title", L"Название задачи"), UiText(L"Example: Call client about renewal", L"Например: связаться с клиентом по продлению"), L"", true},
            {UiText(L"Assigned to", L"Исполнитель"), UiText(L"Exact employee name", L"Точное имя сотрудника"), L"", false},
            {UiText(L"Due date", L"Срок"), UiText(L"YYYY-MM-DD", L"ГГГГ-ММ-ДД"), L"", false},
            {UiText(L"Priority", L"Приоритет"), UiText(L"High / Medium / Low", L"High / Medium / Low"), L"Medium", false}
        };
        if (!ui::ShowDataEntryDialog(hwnd_, UiText(L"New Task", L"Новая задача"),
                UiText(L"Create an operational task for a manager or employee.", L"Создать операционную задачу для менеджера или сотрудника."),
                fields)) {
            return 0;
        }
        if (!data::Repository::Instance().CreateQuickEntry(L"Task", fields[0].value, fields[1].value, fields[2].value, fields[3].value)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"New Task", L"Новая задача").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    if (id == IDC_COMPLETE_TASK) {
        const int index = SelectedRowIndex();
        if (index < 0 || index >= static_cast<int>(rows_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select a task first.", L"Сначала выберите задачу.").c_str(),
                UiText(L"Mark Done", L"Отметить выполненной").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (rows_[index].taskId <= 0) {
            MessageBoxW(hwnd_, UiText(L"Only task rows can be marked as done. Stock and deal alerts are read-only.", L"Закрывать можно только задачи. Складские и сделочные предупреждения доступны только для просмотра.").c_str(),
                UiText(L"Mark Done", L"Отметить выполненной").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (!data::Repository::Instance().CompleteTask(rows_[index].taskId)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Mark Done", L"Отметить выполненной").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }

    return 0;
}

LRESULT NotificationsPage::OnDrawItem(WPARAM, LPARAM lParam) {
    ui::DrawUiButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam), ui::GetButtonKind(reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->hwndItem), false);
    return TRUE;
}
