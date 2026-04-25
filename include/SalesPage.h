#pragma once
#include "DataModels.h"
#include "PageBase.h"
#include <vector>
class SalesPage : public PageBase {
public:
    SalesPage();
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
    data::SalesFilter BuildFilter() const;

    HWND searchEdit_{}; HWND fromDate_{}; HWND toDate_{}; HWND statusCombo_{}; HWND channelCombo_{};
    HWND addButton_{}; HWND statusButton_{}; HWND deleteButton_{}; HWND refreshButton_{}; HWND exportButton_{}; HWND salesTable_{};
    std::vector<data::SalesRecord> sales_{};
};
