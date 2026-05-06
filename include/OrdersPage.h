#pragma once
#include "DataModels.h"
#include "PageBase.h"
#include <vector>
class OrdersPage : public PageBase {
public:
    OrdersPage();
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
    data::OrderFilter BuildFilter() const;

    HWND searchEdit_{}; HWND statusCombo_{}; HWND paymentCombo_{}; HWND customerCombo_{}; HWND fromDate_{}; HWND toDate_{};
    HWND addButton_{}; HWND editButton_{}; HWND advanceButton_{}; HWND cancelButton_{}; HWND deleteButton_{}; HWND refreshButton_{}; HWND exportButton_{}; HWND ordersTable_{};
    std::vector<data::OrderRecord> orders_{};
};
