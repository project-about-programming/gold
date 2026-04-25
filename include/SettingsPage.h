#pragma once
#include "DataModels.h"
#include "PageBase.h"
#include <vector>
class SettingsPage : public PageBase {
public:
    SettingsPage();
protected:
    const wchar_t* ClassName() const override;
    void OnCreate() override;
    void OnActivate() override;
    void OnSize(int width,int height) override;
    void OnPaint(HDC hdc,const RECT& client) override;
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam) override;
    LRESULT OnDrawItem(WPARAM wParam, LPARAM lParam) override;
    LRESULT OnNotify(WPARAM wParam, LPARAM lParam) override;
private:
    void ApplyAccess();
    void ReloadData();
    void FillAccounts();
    int SelectedAccountIndex() const;

    HWND themeCombo_{}; HWND languageCombo_{}; HWND dateCombo_{}; HWND companyEdit_{}; HWND refreshEdit_{}; HWND startupCombo_{};
    HWND dbPathEdit_{}; HWND dbEngineEdit_{}; HWND dbUsersEdit_{}; HWND dbOrdersEdit_{};
    HWND testButton_{}; HWND saveButton_{}; HWND resetButton_{};
    HWND addAccountButton_{}; HWND toggleAccountButton_{}; HWND resetPasswordButton_{}; HWND roleAccountButton_{}; HWND deleteAccountButton_{};
    HWND accountsTable_{};
    data::DatabaseOverview overview_{};
    data::AppSettings settings_{};
    std::vector<data::AccountRecord> accounts_{};
};
