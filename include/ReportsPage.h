#pragma once
#include "DataModels.h"
#include "PageBase.h"
#include <vector>
class ReportsPage : public PageBase {
public:
    ReportsPage();
protected:
    const wchar_t* ClassName() const override;
    void OnCreate() override;
    void OnActivate() override;
    void OnSize(int width,int height) override;
    void OnPaint(HDC hdc,const RECT& client) override;
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam) override;
    LRESULT OnDrawItem(WPARAM wParam, LPARAM lParam) override;
private:
    void ApplyAccess();
    void ReloadData();
    HWND dailyButton_{}; HWND fulfillButton_{}; HWND inventoryButton_{}; HWND employeeButton_{};
    HWND clientButton_{}; HWND profitButton_{}; HWND cancelButton_{}; HWND auditButton_{};
    HWND csvButton_{}; HWND pdfButton_{}; HWND archiveButton_{};
    HWND expiryTable_{};
    std::vector<data::ProductExpiryRecord> expiryRows_{};
};
