#pragma once

#include "DataModels.h"
#include "PageBase.h"

#include <vector>

class AuditLogPage : public PageBase {
public:
    AuditLogPage();

protected:
    const wchar_t* ClassName() const override;
    void OnCreate() override;
    void OnActivate() override;
    void OnSize(int width, int height) override;
    void OnPaint(HDC hdc, const RECT& client) override;
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam) override;
    LRESULT OnDrawItem(WPARAM wParam, LPARAM lParam) override;

private:
    void ReloadData();
    std::wstring SeverityFor(const data::AuditRecord& row) const;

    HWND refreshButton_{};
    HWND exportButton_{};
    HWND auditTable_{};
    std::vector<data::AuditRecord> rows_;
};
