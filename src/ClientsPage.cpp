#include "ClientsPage.h"

#include "AccessControl.h"
#include "DataRepository.h"
#include "DataEntryDialog.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>
#include <sstream>

namespace {
enum : int { IDC_SEARCH = 6001, IDC_SEGMENT, IDC_SORT, IDC_ADD, IDC_EDIT, IDC_DELETE, IDC_CLIENTS, IDC_HISTORY };

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

std::wstring CanonicalSegment(const std::wstring& value) {
    if (value == L"Все сегменты") return L"All Segments";
    if (value == L"Ключевой клиент") return L"Key Account";
    if (value == L"Средний бизнес") return L"Mid-Market";
    if (value == L"Региональный партнёр") return L"Regional Partner";
    return value;
}

std::wstring CanonicalSort(const std::wstring& value) {
    if (value == L"Сортировка: компания") return L"Sort: Company";
    if (value == L"Сортировка: сегмент") return L"Sort: Segment";
    if (value == L"Сортировка: регион") return L"Sort: Region";
    return value;
}

std::wstring FormatMoney(double value) {
    std::wostringstream stream;
    stream << L"$" << static_cast<long long>(value + 0.5);
    return stream.str();
}

struct ClientsLayout {
    RECT title{};
    RECT filterPanel{};
    RECT search{};
    RECT segment{};
    RECT sort{};
    RECT add{};
    RECT edit{};
    RECT remove{};
    RECT clientsPanel{};
    RECT clientsTable{};
    RECT profilePanel{};
    RECT historyPanel{};
    RECT historyTable{};
};

ClientsLayout BuildLayout(const RECT& client) {
    ClientsLayout layout{};
    const auto m = ui::GetMetrics();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);
    layout.filterPanel = ui::TakeTop(&area, ui::Scale(88), m.sectionGap);

    RECT mainColumnsArea = area;
    const int leftWidth = std::min(
        std::max(ui::Scale(420), ui::Width(mainColumnsArea) * 58 / 100),
        std::max(0, ui::Width(mainColumnsArea) - ui::Scale(300) - m.sectionGap));
    layout.clientsPanel = ui::TakeLeft(&mainColumnsArea, leftWidth, m.sectionGap);
    RECT rightColumn = mainColumnsArea;
    const int minHistoryHeight = ui::Scale(132);
    int profileHeight = std::max(ui::Scale(280), ui::Height(rightColumn) * 54 / 100);
    if (ui::Height(rightColumn) > minHistoryHeight + m.sectionGap) {
        profileHeight = std::min(profileHeight, ui::Height(rightColumn) - minHistoryHeight - m.sectionGap);
    }
    layout.profilePanel = ui::TakeTop(&rightColumn, profileHeight, m.sectionGap);
    layout.historyPanel = rightColumn;

    RECT filterInner = ui::Inset(layout.filterPanel, ui::Scale(14), ui::Scale(12));
    const int actionWidth = ui::IconButtonWidth();
    layout.remove = ui::TakeRight(&filterInner, actionWidth, m.compactGap);
    layout.edit = ui::TakeRight(&filterInner, actionWidth, m.compactGap);
    layout.add = ui::TakeRight(&filterInner, actionWidth, m.compactGap);
    layout.sort = ui::TakeRight(&filterInner, ui::Scale(180), m.compactGap);
    layout.segment = ui::TakeRight(&filterInner, ui::Scale(180), m.compactGap);
    layout.search = filterInner;
    layout.search.top = layout.segment.top = layout.sort.top = layout.add.top = layout.edit.top = layout.remove.top = layout.filterPanel.top + ui::Scale(40);
    layout.search.bottom = layout.segment.bottom = layout.sort.bottom = layout.search.top + m.controlHeight;
    layout.add.bottom = layout.edit.bottom = layout.remove.bottom = layout.search.top + m.buttonHeight;

    layout.clientsTable = ui::Inset(ui::MakeRect(layout.clientsPanel.left, layout.clientsPanel.top + m.panelHeaderHeight, layout.clientsPanel.right, layout.clientsPanel.bottom), ui::Scale(12), ui::Scale(12));
    layout.historyTable = ui::Inset(ui::MakeRect(layout.historyPanel.left, layout.historyPanel.top + m.panelHeaderHeight, layout.historyPanel.right, layout.historyPanel.bottom), ui::Scale(12), ui::Scale(12));
    return layout;
}

void DrawFilterLabel(HDC hdc, const RECT& control, const std::wstring& text) {
    ui::DrawTextLine(hdc, text, ui::MakeRect(control.left, control.top - ui::Scale(22), control.right, control.top - ui::Scale(4)),
        theme::SmallBoldFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE);
}

void DrawField(HDC hdc, int x, int y, int width, const std::wstring& label, const std::wstring& value) {
    ui::DrawTextLine(hdc, label, ui::MakeRect(x, y, x + width, y + ui::Scale(18)), theme::SmallBoldFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE);
    ui::DrawTextLine(hdc, value.empty() ? L"-" : value, ui::MakeRect(x, y + ui::Scale(18), x + width, y + ui::Scale(44)), theme::BodyFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
}

bool Can(access::Permission permission) {
    return access::HasPermission(data::Repository::Instance().CurrentAccount(), permission);
}

void SetActionVisible(HWND hwnd, bool visible) {
    ShowWindow(hwnd, SW_SHOW);
    EnableWindow(hwnd, visible ? TRUE : FALSE);
}

} // namespace

ClientsPage::ClientsPage() : PageBase(L"Client Registry", L"Customer portfolio, contact intelligence, and purchase history previews."), selectedClient_(-1) {}

const wchar_t* ClientsPage::ClassName() const { return L"NativeClientsPage"; }

void ClientsPage::OnCreate() {
    searchEdit_ = ui::CreateUiEdit(hwnd_, IDC_SEARCH);
    segmentCombo_ = ui::CreateUiCombo(hwnd_, IDC_SEGMENT);
    sortCombo_ = ui::CreateUiCombo(hwnd_, IDC_SORT);
    addButton_ = ui::CreateUiButton(hwnd_, IDC_ADD, L"+", ui::ButtonKind::Primary);
    editButton_ = ui::CreateUiButton(hwnd_, IDC_EDIT, L"\u270E", ui::ButtonKind::Secondary);
    deleteButton_ = ui::CreateUiButton(hwnd_, IDC_DELETE, L"\u2715", ui::ButtonKind::Danger);
    clientsTable_ = ui::CreateUiListView(hwnd_, IDC_CLIENTS);
    historyTable_ = ui::CreateUiListView(hwnd_, IDC_HISTORY);

    ui::SetEditCueBanner(searchEdit_, UiText(L"Search clients by company or contact", L"Поиск клиентов по компании или контакту"));
    ui::AddComboItems(segmentCombo_, {UiText(L"All Segments", L"Все сегменты"), UiText(L"Key Account", L"Ключевой клиент"), UiText(L"Mid-Market", L"Средний бизнес"), UiText(L"Regional Partner", L"Региональный партнёр")});
    ui::AddComboItems(sortCombo_, {UiText(L"Sort: Company", L"Сортировка: компания"), UiText(L"Sort: Segment", L"Сортировка: сегмент"), UiText(L"Sort: Region", L"Сортировка: регион")});
    ui::AddListColumns(clientsTable_, {UiText(L"Company", L"Компания"), UiText(L"Contact", L"Контакт"), UiText(L"Segment", L"Сегмент"), UiText(L"Region", L"Регион"), UiText(L"Last Order", L"Последний заказ"), L"LTV"}, {180, 140, 120, 130, 110, 100});
    ui::AddListColumns(historyTable_, {UiText(L"Order ID", L"Заказ"), UiText(L"Product", L"Товар"), UiText(L"Date", L"Дата"), UiText(L"Amount", L"Сумма")}, {110, 210, 110, 100});
    ApplyAccess();
    ReloadData();
}

void ClientsPage::OnActivate() {
    ApplyAccess();
    ReloadData();
}

void ClientsPage::ApplyAccess() {
    SetActionVisible(addButton_, Can(access::Permission::CreateClient));
    SetActionVisible(editButton_, Can(access::Permission::EditClient));
    SetActionVisible(deleteButton_, Can(access::Permission::DeleteClient));
}

data::ClientFilter ClientsPage::BuildFilter() const {
    data::ClientFilter filter{};
    wchar_t buffer[256]{};
    GetWindowTextW(searchEdit_, buffer, 256);
    filter.search = buffer;
    GetWindowTextW(segmentCombo_, buffer, 256);
    filter.segment = CanonicalSegment(buffer);
    GetWindowTextW(sortCombo_, buffer, 256);
    filter.sort = CanonicalSort(buffer);
    return filter;
}

void ClientsPage::ReloadData() {
    clients_ = data::Repository::Instance().LoadClients(BuildFilter());
    ui::ClearListRows(clientsTable_);
    for (size_t index = 0; index < clients_.size(); ++index) {
        const auto& client = clients_[index];
        ui::AddListRow(clientsTable_, static_cast<int>(index), {
            client.company,
            client.contact,
            i18n::DataText(client.segment),
            i18n::DataText(client.region),
            client.lastOrderDateDisplay,
            FormatMoney(client.lifetimeValue)
        });
    }
    selectedClient_ = clients_.empty() ? -1 : 0;
    if (selectedClient_ >= 0) {
        ListView_SetItemState(clientsTable_, selectedClient_, LVIS_SELECTED, LVIS_SELECTED);
    }
    FillHistory();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void ClientsPage::FillHistory() {
    history_ = selectedClient_ >= 0 && selectedClient_ < static_cast<int>(clients_.size())
        ? data::Repository::Instance().LoadClientOrderHistory(clients_[selectedClient_].id)
        : std::vector<data::ClientOrderRecord>{};

    ui::ClearListRows(historyTable_);
    for (size_t index = 0; index < history_.size(); ++index) {
        const auto& row = history_[index];
        ui::AddListRow(historyTable_, static_cast<int>(index), {row.orderCode, row.product, row.dateDisplay, FormatMoney(row.amount)});
    }
}

void ClientsPage::OnSize(int width, int height) {
    const ClientsLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    ui::MoveWindowToRect(searchEdit_, layout.search);
    ui::MoveWindowToRect(segmentCombo_, layout.segment);
    ui::MoveWindowToRect(sortCombo_, layout.sort);
    ui::MoveWindowToRect(addButton_, layout.add);
    ui::MoveWindowToRect(editButton_, layout.edit);
    ui::MoveWindowToRect(deleteButton_, layout.remove);
    ui::MoveWindowToRect(clientsTable_, layout.clientsTable);
    ui::MoveWindowToRect(historyTable_, layout.historyTable);
}

void ClientsPage::OnPaint(HDC hdc, const RECT& client) {
    const ClientsLayout layout = BuildLayout(client);

    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"Client Registry", L"Реестр клиентов"), L"", ui::Width(layout.title));

    ui::DrawRoundedPanel(hdc, layout.filterPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    DrawFilterLabel(hdc, layout.search, UiText(L"Search", L"Поиск"));
    DrawFilterLabel(hdc, layout.segment, UiText(L"Segment", L"Сегмент"));
    DrawFilterLabel(hdc, layout.sort, UiText(L"Sort", L"Сортировка"));

    ui::DrawRoundedPanel(hdc, layout.clientsPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.clientsPanel.left + ui::Scale(16), layout.clientsPanel.top + ui::Scale(14),
        UiText(L"Clients Table", L"Таблица клиентов"), L"", ui::Width(layout.clientsPanel) - ui::Scale(32));

    ui::DrawRoundedPanel(hdc, layout.profilePanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.profilePanel.left + ui::Scale(16), layout.profilePanel.top + ui::Scale(14),
        UiText(L"Client Profile", L"Профиль клиента"), L"", ui::Width(layout.profilePanel) - ui::Scale(32));

    const int left = layout.profilePanel.left + ui::Scale(20);
    const int top = layout.profilePanel.top + ui::Scale(64);
    const int columnGap = ui::Scale(20);
    const int columnWidth = (ui::Width(layout.profilePanel) - ui::Scale(40) - columnGap) / 2;

    const int profileClip = SaveDC(hdc);
    IntersectClipRect(hdc, layout.profilePanel.left + ui::Scale(12), layout.profilePanel.top + ui::Scale(48),
        layout.profilePanel.right - ui::Scale(12), layout.profilePanel.bottom - ui::Scale(12));
    if (selectedClient_ >= 0 && selectedClient_ < static_cast<int>(clients_.size())) {
        const auto& selected = clients_[selectedClient_];
        const int rowGap = ui::Scale(48);
        DrawField(hdc, left, top, columnWidth, UiText(L"Company", L"Компания"), selected.company);
        DrawField(hdc, left + columnWidth + columnGap, top, columnWidth, UiText(L"Contact", L"Контакт"), selected.contact);
        DrawField(hdc, left, top + rowGap, columnWidth, UiText(L"Email", L"Email"), selected.email);
        DrawField(hdc, left + columnWidth + columnGap, top + rowGap, columnWidth, UiText(L"Phone", L"Телефон"), selected.phone);
        DrawField(hdc, left, top + rowGap * 2, columnWidth, UiText(L"Segment", L"Сегмент"), i18n::DataText(selected.segment));
        DrawField(hdc, left + columnWidth + columnGap, top + rowGap * 2, columnWidth, UiText(L"Region", L"Регион"), i18n::DataText(selected.region));
        DrawField(hdc, left, top + rowGap * 3, columnWidth, UiText(L"Last Order", L"Последний заказ"), selected.lastOrderDateDisplay);
        DrawField(hdc, left + columnWidth + columnGap, top + rowGap * 3, columnWidth, UiText(L"Lifetime Value", L"Пожизненная ценность"), FormatMoney(selected.lifetimeValue));
        DrawField(hdc, left, top + rowGap * 4, ui::Width(layout.profilePanel) - ui::Scale(40), UiText(L"Account Manager", L"Менеджер клиента"), selected.accountManager);
    } else {
        ui::DrawTextLine(hdc, UiText(L"No client selected", L"Клиент не выбран"),
            ui::MakeRect(left, top, layout.profilePanel.right - ui::Scale(20), top + ui::Scale(40)),
            theme::BodyFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    }
    RestoreDC(hdc, profileClip);

    ui::DrawRoundedPanel(hdc, layout.historyPanel, theme::kPanelBackground, theme::kPanelBorder, ui::Scale(18), true);
    ui::DrawSectionTitle(hdc, layout.historyPanel.left + ui::Scale(16), layout.historyPanel.top + ui::Scale(14),
        UiText(L"Purchase History", L"История покупок"), L"", ui::Width(layout.historyPanel) - ui::Scale(32));
}

LRESULT ClientsPage::OnCommand(WPARAM wParam, LPARAM) {
    const int id = LOWORD(wParam);
    const int code = HIWORD(wParam);
    if (id == IDC_ADD && code == BN_CLICKED) {
        std::vector<ui::DataEntryField> fields{
            {UiText(L"Company", L"Компания"), UiText(L"Example: Blue Peak Stores", L"Например: Blue Peak Stores"), L"", true},
            {UiText(L"Contact person", L"Контактное лицо"), UiText(L"Example: Alex Morgan", L"Например: Алексей Морозов"), L"", true},
            {UiText(L"Email", L"Email"), UiText(L"client@example.com", L"client@example.com"), L"", false},
            {UiText(L"Phone", L"Телефон"), UiText(L"+998 90 000 00 00", L"+998 90 000 00 00"), L"", false}
        };
        if (!ui::ShowDataEntryDialog(hwnd_, UiText(L"Add Client", L"Добавить клиента"),
                UiText(L"Enter client data. Required fields are marked with an asterisk.", L"Введите данные клиента. Обязательные поля отмечены звёздочкой."),
                fields)) {
            return 0;
        }
        if (!data::Repository::Instance().CreateQuickEntry(L"Client", fields[0].value, fields[1].value, fields[2].value, fields[3].value)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Add Client", L"Добавить клиента").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }
    if (id == IDC_EDIT && code == BN_CLICKED) {
        if (selectedClient_ < 0 || selectedClient_ >= static_cast<int>(clients_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select a client first.", L"Select a client first.").c_str(), UiText(L"Edit Client", L"Edit Client").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (!data::Repository::Instance().UpdateDemoClient(clients_[selectedClient_].id)) {
            MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Edit Client", L"Edit Client").c_str(), MB_OK | MB_ICONWARNING);
        }
        ReloadData();
        return 0;
    }
    if (id == IDC_DELETE && code == BN_CLICKED) {
        if (selectedClient_ < 0 || selectedClient_ >= static_cast<int>(clients_.size())) {
            MessageBoxW(hwnd_, UiText(L"Select a client first.", L"Select a client first.").c_str(), UiText(L"Delete Client", L"Delete Client").c_str(), MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        if (MessageBoxW(hwnd_, UiText(L"Delete the selected client and linked orders/sales?", L"Delete the selected client and linked orders/sales?").c_str(), UiText(L"Delete Client", L"Delete Client").c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES) {
            if (!data::Repository::Instance().DeleteClient(clients_[selectedClient_].id)) {
                MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), UiText(L"Delete Client", L"Delete Client").c_str(), MB_OK | MB_ICONWARNING);
            }
            ReloadData();
        }
        return 0;
    }
    if ((id == IDC_SEARCH && code == EN_CHANGE) ||
        ((id == IDC_SEGMENT || id == IDC_SORT) && code == CBN_SELCHANGE)) {
        ReloadData();
        return 0;
    }
    return 0;
}

LRESULT ClientsPage::OnDrawItem(WPARAM, LPARAM lParam) {
    ui::DrawUiButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam), ui::GetButtonKind(reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->hwndItem), false);
    return TRUE;
}

LRESULT ClientsPage::OnNotify(WPARAM wParam, LPARAM lParam) {
    auto* hdr = reinterpret_cast<LPNMHDR>(lParam);
    if (hdr && hdr->idFrom == IDC_CLIENTS && hdr->code == LVN_ITEMCHANGED) {
        auto* info = reinterpret_cast<NMLISTVIEW*>(lParam);
        if ((info->uChanged & LVIF_STATE) && (info->uNewState & LVIS_SELECTED) && info->iItem >= 0) {
            selectedClient_ = info->iItem;
            FillHistory();
            RECT client{};
            GetClientRect(hwnd_, &client);
            const ClientsLayout layout = BuildLayout(client);
            RECT detailsRect = ui::MakeRect(layout.profilePanel.left, layout.profilePanel.top, layout.historyPanel.right, layout.historyPanel.bottom);
            InvalidateRect(hwnd_, &detailsRect, FALSE);
        }
    }
    return PageBase::OnNotify(wParam, lParam);
}
