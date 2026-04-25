#include "LoginWindow.h"

#include "DataRepository.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <commctrl.h>
#include <string>

namespace {
enum : int {
    IDC_USERNAME = 12001,
    IDC_PASSWORD,
    IDC_SUBMIT
};

struct LoginLayout {
    RECT card{};
    RECT username{};
    RECT password{};
    RECT submit{};
};

LRESULT CALLBACK EnterToSubmitSubclass(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR refData) {
    if (message == WM_KEYDOWN && wParam == VK_RETURN) {
        const HWND parent = GetParent(hwnd);
        if (parent) {
            PostMessageW(parent, WM_COMMAND, MAKEWPARAM(static_cast<int>(refData), BN_CLICKED), reinterpret_cast<LPARAM>(GetDlgItem(parent, static_cast<int>(refData))));
        }
        return 0;
    }
    return DefSubclassProc(hwnd, message, wParam, lParam);
}

LoginLayout BuildLayout(const RECT& client) {
    LoginLayout layout{};
    layout.card = ui::MakeRect(client.left + ui::Scale(28), client.top + ui::Scale(28), client.right - ui::Scale(28), client.bottom - ui::Scale(28));

    const int fieldLeft = layout.card.left + ui::Scale(36);
    const int fieldRight = layout.card.right - ui::Scale(36);
    const int fieldHeight = ui::GetMetrics().controlHeight;
    int y = layout.card.top + ui::Scale(122);
    layout.username = ui::MakeRect(fieldLeft, y, fieldRight, y + fieldHeight);
    y += fieldHeight + ui::Scale(36);
    layout.password = ui::MakeRect(fieldLeft, y, fieldRight, y + fieldHeight);

    const int buttonTop = layout.card.bottom - ui::Scale(78);
    layout.submit = ui::MakeRect(fieldLeft, buttonTop, fieldRight, buttonTop + ui::GetMetrics().buttonHeight);
    return layout;
}

void DrawFieldLabel(HDC hdc, const RECT& field, const std::wstring& label) {
    ui::DrawTextLine(hdc, label, ui::MakeRect(field.left, field.top - ui::Scale(22), field.right, field.top - ui::Scale(4)),
        theme::SmallBoldFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
}
} // namespace

LoginWindow::LoginWindow(HINSTANCE instance) : instance_(instance) {}

bool LoginWindow::Show() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.hInstance = instance_;
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = L"SalesMonitoringLoginWindow";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExW(&wc);

    const int width = ui::Scale(520);
    const int height = ui::Scale(420);
    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    const int x = workArea.left + (ui::Width(workArea) - width) / 2;
    const int y = workArea.top + (ui::Height(workArea) - height) / 2;

    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"Вход - Мониторинг продаж",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        x, y, width, height, nullptr, nullptr, instance_, this);
    if (!hwnd_) {
        return false;
    }

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);

    MSG msg{};
    while (!done_ && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(hwnd_, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return accepted_;
}

LRESULT CALLBACK LoginWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LoginWindow* self = reinterpret_cast<LoginWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<LoginWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    if (!self) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

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
    case WM_COMMAND:
        self->OnCommand(wParam);
        return 0;
    case WM_DRAWITEM:
        self->OnDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        return TRUE;
    case WM_ERASEBKGND:
        return 1;
    case WM_CLOSE:
        self->done_ = true;
        self->accepted_ = false;
        DestroyWindow(hwnd);
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

void LoginWindow::OnCreate() {
    usernameEdit_ = ui::CreateUiEdit(hwnd_, IDC_USERNAME, L"");
    passwordEdit_ = ui::CreateUiEdit(hwnd_, IDC_PASSWORD, L"");
    submitButton_ = ui::CreateUiButton(hwnd_, IDC_SUBMIT, L"Войти", ui::ButtonKind::Primary);

    SendMessageW(passwordEdit_, EM_SETPASSWORDCHAR, L'*', 0);
    ui::SetEditCueBanner(usernameEdit_, L"Логин или email");
    ui::SetEditCueBanner(passwordEdit_, L"Введите пароль");
    SetWindowTextW(usernameEdit_, L"a.petrova");
    SetWindowTextW(passwordEdit_, L"");
    SetWindowSubclass(usernameEdit_, EnterToSubmitSubclass, 1, IDC_SUBMIT);
    SetWindowSubclass(passwordEdit_, EnterToSubmitSubclass, 2, IDC_SUBMIT);
    SetFocus(passwordEdit_);
}

void LoginWindow::OnSize(int width, int height) {
    const LoginLayout layout = BuildLayout(ui::MakeRect(0, 0, width, height));
    MoveWindow(usernameEdit_, layout.username.left, layout.username.top, ui::Width(layout.username), ui::Height(layout.username), TRUE);
    MoveWindow(passwordEdit_, layout.password.left, layout.password.top, ui::Width(layout.password), ui::Height(layout.password), TRUE);
    MoveWindow(submitButton_, layout.submit.left, layout.submit.top, ui::Width(layout.submit), ui::Height(layout.submit), TRUE);
}

void LoginWindow::OnPaint() {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd_, &ps);
    RECT client{};
    GetClientRect(hwnd_, &client);
    const LoginLayout layout = BuildLayout(client);

    ui::FillRectColor(hdc, client, theme::kPanelBackground);
    ui::FillRectColor(hdc, layout.card, theme::kPanelBackground);

    ui::DrawTextLine(hdc, L"Вход в систему",
        ui::MakeRect(layout.card.left + ui::Scale(36), layout.card.top + ui::Scale(28), layout.card.right - ui::Scale(36), layout.card.top + ui::Scale(64)),
        theme::TitleFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    ui::DrawTextLine(hdc,
        L"Введите логин и пароль, чтобы открыть рабочую панель продаж.",
        ui::MakeRect(layout.card.left + ui::Scale(36), layout.card.top + ui::Scale(72), layout.card.right - ui::Scale(36), layout.card.top + ui::Scale(108)),
        theme::SmallFont(), theme::kTextSecondary, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);

    DrawFieldLabel(hdc, layout.username, L"Логин или email");
    DrawFieldLabel(hdc, layout.password, L"Пароль");

    EndPaint(hwnd_, &ps);
}

void LoginWindow::OnCommand(WPARAM wParam) {
    const int id = LOWORD(wParam);
    if (id == IDC_SUBMIT || id == IDOK) {
        TrySubmit();
    }
}

void LoginWindow::OnDrawItem(const DRAWITEMSTRUCT* dis) {
    ui::DrawUiButton(dis, ui::GetButtonKind(dis->hwndItem), false);
}

bool LoginWindow::TrySubmit() {
    data::AccountRecord account{};
    const std::wstring username = ControlText(usernameEdit_);
    const std::wstring password = ControlText(passwordEdit_);

    if (!data::Repository::Instance().AuthenticateUser(username, password, &account)) {
        MessageBoxW(hwnd_, data::Repository::Instance().LastError().c_str(), L"Вход", MB_OK | MB_ICONWARNING);
        return false;
    }

    accepted_ = true;
    done_ = true;
    DestroyWindow(hwnd_);
    return true;
}

std::wstring LoginWindow::ControlText(HWND control) const {
    const int length = GetWindowTextLengthW(control);
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    if (length > 0) {
        GetWindowTextW(control, text.data(), length + 1);
    }
    text.resize(static_cast<size_t>(length));
    return text;
}
