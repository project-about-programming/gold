#pragma once

#include <windows.h>

#include "PageBase.h"

#include <memory>
#include <string>
#include <vector>

class MainWindow {
public:
    explicit MainWindow(HINSTANCE instance);
    bool Create();
    int Run();

private:
    struct NavItem {
        int id;
        int pageIndex;
        std::wstring text;
        HWND button;
    };

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void OnCreate();
    void OnSize(int width, int height);
    void OnPaint();
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void OnDrawItem(const DRAWITEMSTRUCT* dis);
    void OnDpiChanged(WPARAM wParam, LPARAM lParam);
    void OnSettingsChanged();
    void ApplyPersistedSettings();
    void RefreshLocalizedText();
    void RefreshThemeResources();
    std::wstring LocalizedPageTitle(int index) const;
    void SwitchPage(int index);
    void UpdateHeader();
    void CreatePages();
    void CreateSidebar();
    void CreateHeader();
    void LayoutSidebar(const RECT& client);
    void LayoutHeader(const RECT& client);
    void LayoutPages(const RECT& client);
    void LayoutStatusBar(const RECT& client);

    HINSTANCE instance_;
    HWND hwnd_;
    HWND clockLabel_;
    HWND statusBar_;
    std::vector<NavItem> navItems_;
    std::vector<std::unique_ptr<PageBase>> pages_;
    int currentPage_;
};
