#pragma once

#include <windows.h>

#include <string>

class PageBase {
public:
    PageBase(std::wstring title, std::wstring subtitle);
    virtual ~PageBase() = default;

    bool Create(HWND parent, HINSTANCE instance);
    void Show(bool visible) const;
    void Resize(const RECT& bounds) const;
    void Activate();
    HWND Handle() const;

    const std::wstring& Title() const;
    const std::wstring& Subtitle() const;

protected:
    virtual const wchar_t* ClassName() const = 0;
    virtual void OnCreate() = 0;
    virtual void OnSize(int width, int height) = 0;
    virtual void OnPaint(HDC hdc, const RECT& client) = 0;
    virtual void OnActivate();
    virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnDrawItem(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnMessage(UINT message, WPARAM wParam, LPARAM lParam);

    HWND hwnd_;

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Register(HINSTANCE instance);

    std::wstring title_;
    std::wstring subtitle_;
};
