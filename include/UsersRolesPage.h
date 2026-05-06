#pragma once

#include "DataModels.h"
#include "PageBase.h"

#include <vector>

class UsersRolesPage : public PageBase {
public:
    UsersRolesPage();

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
    int SelectedAccountIndex() const;

    HWND addButton_{};
    HWND toggleButton_{};
    HWND resetPasswordButton_{};
    HWND changeRoleButton_{};
    HWND deleteButton_{};
    HWND accountsTable_{};
    HWND rolesTable_{};
    std::vector<data::AccountRecord> accounts_;
    std::vector<data::RoleSummaryRecord> roles_;
};
