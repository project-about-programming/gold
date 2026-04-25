#include "PageBase.h"

#include "UiHelpers.h"

#include <utility>

PageBase::PageBase(std::wstring title, std::wstring subtitle)
    : title_(std::move(title)), subtitle_(std::move(subtitle)), hwnd_(nullptr) {}

bool PageBase::Create(HWND parent, HINSTANCE instance) {
    Register(instance);
    hwnd_ = CreateWindowExW(0, ClassName(), L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                            0, 0, 100, 100, parent, nullptr, instance, this);
    return hwnd_ != nullptr;
}

void PageBase::Show(bool visible) const { ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE); }
void PageBase::Resize(const RECT& bounds) const { MoveWindow(hwnd_, bounds.left, bounds.top, ui::Width(bounds), ui::Height(bounds), TRUE); }
void PageBase::Activate() { OnActivate(); InvalidateRect(hwnd_, nullptr, FALSE); }
HWND PageBase::Handle() const { return hwnd_; }
const std::wstring& PageBase::Title() const { return title_; }
const std::wstring& PageBase::Subtitle() const { return subtitle_; }

void PageBase::OnActivate() {}
LRESULT PageBase::OnCommand(WPARAM, LPARAM) { return 0; }
LRESULT PageBase::OnDrawItem(WPARAM, LPARAM) { return 0; }
LRESULT PageBase::OnNotify(WPARAM, LPARAM lParam) {
    const auto* header = reinterpret_cast<const NMHDR*>(lParam);
    if (header && header->code == NM_CUSTOMDRAW) {
        return ui::HandleListViewCustomDraw(lParam);
    }
    return 0;
}

LRESULT PageBase::OnMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        return OnCommand(wParam, lParam);
    case WM_DRAWITEM:
        return OnDrawItem(wParam, lParam);
    case WM_NOTIFY:
        return OnNotify(wParam, lParam);
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        OnSize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd_, &ps);
        RECT rc{};
        GetClientRect(hwnd_, &rc);
        OnPaint(hdc, rc);
        EndPaint(hwnd_, &ps);
        return 0;
    }
    default:
        return DefWindowProcW(hwnd_, message, wParam, lParam);
    }
}

LRESULT CALLBACK PageBase::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PageBase* self = reinterpret_cast<PageBase*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<PageBase*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    if (!self) return DefWindowProcW(hwnd, message, wParam, lParam);
    if (message == WM_CREATE) {
        self->OnCreate();
        return 0;
    }
    return self->OnMessage(message, wParam, lParam);
}

void PageBase::Register(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.hInstance = instance;
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = ClassName();
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExW(&wc);
}
