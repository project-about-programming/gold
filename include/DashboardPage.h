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
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam) override;
    LRESULT OnDrawItem(WPARAM wParam, LPARAM lParam) override;
private:
    void LoadSnapshot();
    void FillTables();
    void UpdatePeriodButtons();
    bool IsSystemDashboard() const;

    HWND periodButtons_[4]{};
    HWND topProducts_{};
    HWND recentOrders_{};
    HWND lowStock_{};
    int activePeriod_{2};
    bool systemDashboard_{false};
    data::DashboardSnapshot snapshot_{};
    data::DatabaseOverview database_{};
    std::vector<data::AccountRecord> accounts_{};
    std::vector<data::RoleSummaryRecord> roles_{};
    std::vector<data::AuditRecord> auditEvents_{};
    std::vector<data::TaskRecord> systemAlerts_{};
};
