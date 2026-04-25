#pragma once
#include "DataModels.h"
#include "PageBase.h"
class DashboardPage : public PageBase {
public:
    DashboardPage();
protected:
    const wchar_t* ClassName() const override;
    void OnCreate() override;
    void OnActivate() override;
    void OnSize(int width,int height) override;
    void OnPaint(HDC hdc,const RECT& client) override;
private:
    void LoadSnapshot();
    void FillTables();

    HWND topProducts_{};
    HWND recentOrders_{};
    HWND lowStock_{};
    data::DashboardSnapshot snapshot_{};
};
