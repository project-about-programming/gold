#pragma once

#include "DataModels.h"
#include "PageBase.h"

#include <string>
#include <vector>

class NotificationsPage : public PageBase {
public:
    NotificationsPage();

protected:
    const wchar_t* ClassName() const override;
    void OnCreate() override;
    void OnActivate() override;
    void OnSize(int width, int height) override;
    void OnPaint(HDC hdc, const RECT& client) override;
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam) override;
    LRESULT OnDrawItem(WPARAM wParam, LPARAM lParam) override;

private:
    struct NotificationRow {
        int taskId{};
        std::wstring type;
        std::wstring item;
        std::wstring owner;
        std::wstring due;
        std::wstring priority;
        std::wstring status;
    };

    void ApplyAccess();
    void ReloadData();
    int SelectedRowIndex() const;

    HWND addTaskButton_{};
    HWND completeButton_{};
    HWND refreshButton_{};
    HWND notificationsTable_{};
    std::vector<NotificationRow> rows_{};
};
