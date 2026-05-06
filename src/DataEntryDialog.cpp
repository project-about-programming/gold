#include "DataEntryDialog.h"

#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>
#include <commctrl.h>
#include <cwctype>
#include <iomanip>
#include <sstream>
#include <uxtheme.h>

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
    std::vector<HWND> inputs;
    HWND okButton{};
    HWND cancelButton{};
    bool accepted{};
};

std::wstring LocalText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Р СѓСЃСЃРєРёР№") ? std::wstring(ru) : std::wstring(en);
}

std::wstring Trim(std::wstring value) {
    auto isSpace = [](wchar_t ch) { return std::iswspace(ch) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](wchar_t ch) { return !isSpace(ch); }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](wchar_t ch) { return !isSpace(ch); }).base(), value.end());
    return value;
}

bool IsDigits(const std::wstring& value) {
    return !value.empty() && std::all_of(value.begin(), value.end(), [](wchar_t ch) { return std::iswdigit(ch) != 0; });
}

bool TryParseDouble(const std::wstring& value, double* result) {
    try {
        size_t parsed{};
        const double number = std::stod(value, &parsed);
        if (parsed != value.size()) {
            return false;
        }
        if (result) {
            *result = number;
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool TryParseIsoDate(const std::wstring& value, SYSTEMTIME* out) {
    if (value.size() < 10) {
        return false;
    }
    try {
        SYSTEMTIME st{};
        st.wYear = static_cast<WORD>(std::stoi(value.substr(0, 4)));
        st.wMonth = static_cast<WORD>(std::stoi(value.substr(5, 2)));
        st.wDay = static_cast<WORD>(std::stoi(value.substr(8, 2)));
        if (st.wYear < 1970 || st.wMonth < 1 || st.wMonth > 12 || st.wDay < 1 || st.wDay > 31) {
            return false;
        }
        if (out) {
            *out = st;
        }
        return true;
    } catch (...) {
        return false;
    }
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

std::wstring DatePickerIso(HWND picker) {
    SYSTEMTIME date{};
    if (DateTime_GetSystemtime(picker, &date) != GDT_VALID) {
        return L"";
    }
    std::wostringstream stream;
    stream << std::setfill(L'0') << std::setw(4) << date.wYear << L"-"
           << std::setw(2) << date.wMonth << L"-"
           << std::setw(2) << date.wDay;
    return stream.str();
}

bool UseTwoColumns(size_t fieldCount) {
    return fieldCount > 6;
}

int FieldRows(size_t fieldCount) {
    return UseTwoColumns(fieldCount) ? static_cast<int>((fieldCount + 1) / 2) : static_cast<int>(fieldCount);
}

RECT FieldEditRect(const RECT& client, int index, size_t fieldCount) {
    const int margin = Scale(24);
    const int rowCount = FieldRows(fieldCount);
    const bool twoColumns = UseTwoColumns(fieldCount);
    const int column = twoColumns ? (index / rowCount) : 0;
    const int row = twoColumns ? (index % rowCount) : index;
    const int gap = Scale(18);
    const int availableWidth = client.right - margin * 2;
    const int columnWidth = twoColumns ? (availableWidth - gap) / 2 : availableWidth;
    const int left = margin + column * (columnWidth + gap);
    const int top = Scale(116) + row * Scale(82);
    return MakeRect(left, top + Scale(22), left + columnWidth, top + Scale(22) + GetMetrics().controlHeight);
}

RECT FieldLabelRect(const RECT& client, int index, size_t fieldCount) {
    const RECT edit = FieldEditRect(client, index, fieldCount);
    return MakeRect(edit.left, edit.top - Scale(22), edit.right, edit.top - Scale(4));
}

HWND CreateDialogCombo(HWND parent, int id, const DataEntryField& field) {
    const DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | WS_BORDER | CBS_HASSTRINGS
        | (field.allowCustomValue ? (CBS_DROPDOWN | CBS_AUTOHSCROLL) : CBS_DROPDOWNLIST);
    HWND combo = CreateWindowExW(0, WC_COMBOBOXW, L"", style,
        0, 0, 140, Scale(260), parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    SetWindowTheme(combo, L"Explorer", nullptr);
    SendMessageW(combo, CB_SETMINVISIBLE, 8, 0);
    AssignFontRole(combo, FontRole::Body);

    int selectedIndex = -1;
    for (const auto& option : field.options) {
        const int index = static_cast<int>(SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(option.text.c_str())));
        SendMessageW(combo, CB_SETITEMDATA, index, option.id);
        if (field.value == option.text || (!field.value.empty() && option.id > 0 && field.value == std::to_wstring(option.id))) {
            selectedIndex = index;
        }
    }
    if (selectedIndex >= 0) {
        SendMessageW(combo, CB_SETCURSEL, selectedIndex, 0);
    } else if (field.allowCustomValue && !field.value.empty()) {
        SetWindowTextW(combo, field.value.c_str());
    }
    return combo;
}

HWND CreateDialogDatePicker(HWND parent, int id, const std::wstring& value) {
    HWND picker = CreateWindowExW(0, DATETIMEPICK_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | WS_BORDER | DTS_SHORTDATEFORMAT,
        0, 0, 140, 32, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    SetWindowTheme(picker, L"Explorer", nullptr);
    DateTime_SetFormat(picker, L"dd MMM yyyy");
    SYSTEMTIME date{};
    if (TryParseIsoDate(value, &date)) {
        DateTime_SetSystemtime(picker, GDT_VALID, &date);
    }
    AssignFontRole(picker, FontRole::Body);
    return picker;
}

HWND CreateDialogInput(HWND parent, int id, const DataEntryField& field) {
    if (field.kind == DataEntryFieldKind::Combo) {
        return CreateDialogCombo(parent, id, field);
    }
    if (field.kind == DataEntryFieldKind::Date) {
        return CreateDialogDatePicker(parent, id, field.value);
    }

    HWND edit = CreateUiEdit(parent, id, field.value);
    SetEditCueBanner(edit, field.placeholder);
    if (field.kind == DataEntryFieldKind::Number) {
        SetWindowLongPtrW(edit, GWL_STYLE, GetWindowLongPtrW(edit, GWL_STYLE) | ES_NUMBER);
    }
    if (field.kind == DataEntryFieldKind::ReadOnly) {
        SendMessageW(edit, EM_SETREADONLY, TRUE, 0);
    }
    return edit;
}

void LayoutDialog(DialogState* state) {
    RECT client{};
    GetClientRect(state->hwnd, &client);
    for (size_t index = 0; index < state->inputs.size(); ++index) {
        const RECT input = FieldEditRect(client, static_cast<int>(index), state->inputs.size());
        MoveWindow(state->inputs[index], input.left, input.top, Width(input), Height(input), TRUE);
    }

    const int buttonWidth = Scale(116);
    const int buttonHeight = GetMetrics().buttonHeight;
    const int gap = Scale(10);
    const int bottom = client.bottom - Scale(26);
    RECT cancelRect = MakeRect(client.right - Scale(24) - buttonWidth, bottom - buttonHeight, client.right - Scale(24), bottom);
    RECT okRect = MakeRect(cancelRect.left - gap - buttonWidth, cancelRect.top, cancelRect.left - gap, cancelRect.bottom);
    MoveWindow(state->okButton, okRect.left, okRect.top, Width(okRect), Height(okRect), TRUE);
    MoveWindow(state->cancelButton, cancelRect.left, cancelRect.top, Width(cancelRect), Height(cancelRect), TRUE);
}

std::wstring ReadFieldValue(const DataEntryField& field, HWND input) {
    if (field.kind == DataEntryFieldKind::Combo) {
        const int selected = static_cast<int>(SendMessageW(input, CB_GETCURSEL, 0, 0));
        if (selected >= 0) {
            const int length = static_cast<int>(SendMessageW(input, CB_GETLBTEXTLEN, selected, 0));
            std::wstring text(static_cast<size_t>(std::max(0, length)) + 1, L'\0');
            SendMessageW(input, CB_GETLBTEXT, selected, reinterpret_cast<LPARAM>(text.data()));
            text.resize(static_cast<size_t>(std::max(0, length)));
            const int optionId = static_cast<int>(SendMessageW(input, CB_GETITEMDATA, selected, 0));
            return field.storeOptionId && optionId != 0 ? std::to_wstring(optionId) : text;
        }
    }
    if (field.kind == DataEntryFieldKind::Date) {
        return DatePickerIso(input);
    }
    return Trim(ReadText(input));
}

bool ValidateField(DialogState* state, size_t index) {
    auto& field = (*state->fields)[index];
    HWND input = state->inputs[index];
    field.value = ReadFieldValue(field, input);

    if (field.required && field.value.empty()) {
        const std::wstring message = LocalText(L"Please fill required field: ", L"Please fill required field: ") + field.label;
        MessageBoxW(state->hwnd, message.c_str(), state->title.c_str(), MB_OK | MB_ICONWARNING);
        SetFocus(input);
        return false;
    }
    if (field.kind == DataEntryFieldKind::Number && !field.value.empty() && !IsDigits(field.value)) {
        const std::wstring message = field.label + L": " + LocalText(L"enter a whole number.", L"enter a whole number.");
        MessageBoxW(state->hwnd, message.c_str(), state->title.c_str(), MB_OK | MB_ICONWARNING);
        SetFocus(input);
        return false;
    }
    if ((field.kind == DataEntryFieldKind::Number || field.kind == DataEntryFieldKind::Decimal) && !field.value.empty()) {
        double number{};
        if (field.kind == DataEntryFieldKind::Number) {
            number = std::stod(field.value);
        } else if (!TryParseDouble(field.value, &number)) {
            const std::wstring message = field.label + L": " + LocalText(L"enter a valid number.", L"enter a valid number.");
            MessageBoxW(state->hwnd, message.c_str(), state->title.c_str(), MB_OK | MB_ICONWARNING);
            SetFocus(input);
            return false;
        }
        if ((field.hasMinValue && number < field.minValue) || (field.hasMaxValue && number > field.maxValue)) {
            const std::wstring message = field.label + L": " + LocalText(L"value is outside the allowed range.", L"value is outside the allowed range.");
            MessageBoxW(state->hwnd, message.c_str(), state->title.c_str(), MB_OK | MB_ICONWARNING);
            SetFocus(input);
            return false;
        }
    }
    return true;
}

bool AcceptDialog(DialogState* state) {
    for (size_t index = 0; index < state->inputs.size(); ++index) {
        if (!ValidateField(state, index)) {
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
            state->inputs.push_back(CreateDialogInput(hwnd, kFirstEditId + static_cast<int>(index), (*state->fields)[index]));
        }
        state->okButton = CreateUiButton(hwnd, kOkId, LocalText(L"Save", L"Save"), ButtonKind::Primary);
        state->cancelButton = CreateUiButton(hwnd, kCancelId, LocalText(L"Cancel", L"Cancel"), ButtonKind::Secondary);
        LayoutDialog(state);
        if (!state->inputs.empty()) {
            SetFocus(state->inputs.front());
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
        DrawTextLine(hdc, state->subtitle, MakeRect(Scale(24), Scale(58), client.right - Scale(24), Scale(92)), theme::SmallFont(), theme::kTextSecondary, DT_LEFT | DT_WORDBREAK);
        for (size_t index = 0; index < state->fields->size(); ++index) {
            const auto& field = (*state->fields)[index];
            const std::wstring label = field.label + (field.required ? L" *" : L"");
            DrawTextLine(hdc, label, FieldLabelRect(client, static_cast<int>(index), state->fields->size()), theme::SmallBoldFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
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

    const bool twoColumns = UseTwoColumns(fields.size());
    const int width = twoColumns ? Scale(800) : Scale(560);
    const int height = std::max(Scale(460), Scale(280) + FieldRows(fields.size()) * Scale(82));
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
