#pragma once

#include <windows.h>

#include <string>

class LoginWindow {
public:
    explicit LoginWindow(HINSTANCE instance);
    bool Show();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void OnCreate();
    void OnSize(int width, int height);
    void OnPaint();
    void OnCommand(WPARAM wParam);
    void OnDrawItem(const DRAWITEMSTRUCT* dis);
    bool TrySubmit();
    std::wstring ControlText(HWND control) const;

    HINSTANCE instance_;
    HWND hwnd_{};
    HWND usernameEdit_{};
    HWND passwordEdit_{};
    HWND submitButton_{};
    bool done_{};
    bool accepted_{};
};
