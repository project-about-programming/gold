#include "DataEntryDialog.h"

#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>

namespace ui {
namespace {

constexpr int kFirstEditId = 17001;
constexpr int kOkId = 17100;
constexpr int kCancelId = 17101;
constexpr const wchar_t* kClassName = L"SalesFlowDataEntryDialog";

struct DialogState {
    HWND hwnd{};
    HWND owner{};
    std::wstring title;
    std::wstring subtitle;
    std::vector<DataEntryField>* fields{};
    std::vector<HWND> edits;
    HWND okButton{};
    HWND cancelButton{};
    bool accepted{};
};

std::wstring LocalText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

RECT FieldEditRect(const RECT& client, int index) {
    const int margin = Scale(24);
    const int top = Scale(108) + index * Scale(66);
    return MakeRect(margin, top + Scale(22), client.right - margin, top + Scale(22) + GetMetrics().controlHeight);
}

RECT FieldLabelRect(const RECT& client, int index) {
    const RECT edit = FieldEditRect(client, index);
    return MakeRect(edit.left, edit.top - Scale(22), edit.right, edit.top - Scale(4));
}

void LayoutDialog(DialogState* state) {
    RECT client{};
    GetClientRect(state->hwnd, &client);
    for (size_t index = 0; index < state->edits.size(); ++index) {
        const RECT edit = FieldEditRect(client, static_cast<int>(index));
        MoveWindow(state->edits[index], edit.left, edit.top, Width(edit), Height(edit), TRUE);
    }

    const int buttonWidth = Scale(116);
    const int buttonHeight = GetMetrics().buttonHeight;
    const int gap = Scale(10);
    const int bottom = client.bottom - Scale(24);
    RECT cancelRect = MakeRect(client.right - Scale(24) - buttonWidth, bottom - buttonHeight, client.right - Scale(24), bottom);
    RECT okRect = MakeRect(cancelRect.left - gap - buttonWidth, cancelRect.top, cancelRect.left - gap, cancelRect.bottom);
    MoveWindow(state->okButton, okRect.left, okRect.top, Width(okRect), Height(okRect), TRUE);
    MoveWindow(state->cancelButton, cancelRect.left, cancelRect.top, Width(cancelRect), Height(cancelRect), TRUE);
}

std::wstring ReadText(HWND hwnd) {
    const int length = GetWindowTextLengthW(hwnd);
    if (length <= 0) {
        return L"";
    }
    std::wstring value(static_cast<size_t>(length) + 1, L'\0');
    GetWindowTextW(hwnd, value.data(), length + 1);
    value.resize(static_cast<size_t>(length));
    return value;
}

bool AcceptDialog(DialogState* state) {
    for (size_t index = 0; index < state->edits.size(); ++index) {
        (*state->fields)[index].value = ReadText(state->edits[index]);
        if ((*state->fields)[index].required && (*state->fields)[index].value.empty()) {
            const std::wstring message = LocalText(L"Please fill required field: ", L"Заполните обязательное поле: ") + (*state->fields)[index].label;
            MessageBoxW(state->hwnd, message.c_str(), state->title.c_str(), MB_OK | MB_ICONWARNING);
            SetFocus(state->edits[index]);
            return false;
        }
    }
    state->accepted = true;
    DestroyWindow(state->hwnd);
    return true;
}

LRESULT CALLBACK DialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<DialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (message) {
    case WM_NCCREATE: {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = reinterpret_cast<DialogState*>(create->lpCreateParams);
        state->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        return TRUE;
    }
    case WM_CREATE: {
        for (size_t index = 0; index < state->fields->size(); ++index) {
            HWND edit = CreateUiEdit(hwnd, kFirstEditId + static_cast<int>(index), (*state->fields)[index].value);
            SetEditCueBanner(edit, (*state->fields)[index].placeholder);
            state->edits.push_back(edit);
        }
        state->okButton = CreateUiButton(hwnd, kOkId, LocalText(L"Save", L"Сохранить"), ButtonKind::Primary);
        state->cancelButton = CreateUiButton(hwnd, kCancelId, LocalText(L"Cancel", L"Отмена"), ButtonKind::Secondary);
        LayoutDialog(state);
        if (!state->edits.empty()) {
            SetFocus(state->edits.front());
        }
        return 0;
    }
    case WM_SIZE:
        LayoutDialog(state);
        return 0;
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        const int code = HIWORD(wParam);
        if (id == kOkId && code == BN_CLICKED) {
            AcceptDialog(state);
            return 0;
        }
        if (id == kCancelId && code == BN_CLICKED) {
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        if (wParam == VK_RETURN) {
            AcceptDialog(state);
            return 0;
        }
        break;
    case WM_DRAWITEM:
        DrawUiButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam), GetButtonKind(reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->hwndItem), false);
        return TRUE;
    case WM_CTLCOLORSTATIC: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, theme::kTextPrimary);
        static HBRUSH brush = CreateSolidBrush(theme::kPanelBackground);
        return reinterpret_cast<INT_PTR>(brush);
    }
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT client{};
        GetClientRect(hwnd, &client);
        FillRectColor(hdc, client, theme::kWindowBackground);
        RECT panel = Inset(client, Scale(10), Scale(10));
        DrawRoundedPanel(hdc, panel, theme::kPanelBackground, theme::kPanelBorder, Scale(18), true);
        DrawTextLine(hdc, state->title, MakeRect(Scale(24), Scale(20), client.right - Scale(24), Scale(54)), theme::TitleFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        DrawTextLine(hdc, state->subtitle, MakeRect(Scale(24), Scale(58), client.right - Scale(24), Scale(90)), theme::SmallFont(), theme::kTextSecondary, DT_LEFT | DT_WORDBREAK);
        for (size_t index = 0; index < state->fields->size(); ++index) {
            const std::wstring label = (*state->fields)[index].label + ((*state->fields)[index].required ? L" *" : L"");
            DrawTextLine(hdc, label, FieldLabelRect(client, static_cast<int>(index)), theme::SmallBoldFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (state && state->owner) {
            EnableWindow(state->owner, TRUE);
            SetActiveWindow(state->owner);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void RegisterDialogClass(HINSTANCE instance) {
    static bool registered = false;
    if (registered) {
        return;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DialogProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassName;
    RegisterClassExW(&wc);
    registered = true;
}

} // namespace

bool ShowDataEntryDialog(HWND owner, const std::wstring& title, const std::wstring& subtitle, std::vector<DataEntryField>& fields) {
    if (fields.empty()) {
        return false;
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    RegisterDialogClass(instance);

    DialogState state{};
    state.owner = owner;
    state.title = title;
    state.subtitle = subtitle;
    state.fields = &fields;

    const int width = Scale(540);
    const int height = std::max(Scale(360), Scale(184) + static_cast<int>(fields.size()) * Scale(66));
    RECT ownerRect{};
    if (owner) {
        GetWindowRect(owner, &ownerRect);
    } else {
        ownerRect = MakeRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }

    const int x = ownerRect.left + std::max(0, (Width(ownerRect) - width) / 2);
    const int y = ownerRect.top + std::max(0, (Height(ownerRect) - height) / 2);
    HWND hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, kClassName, title.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, width, height, owner, nullptr, instance, &state);
    if (!hwnd) {
        return false;
    }

    EnableWindow(owner, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg{};
    while (IsWindow(hwnd) && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return state.accepted;
}

} // namespace ui
