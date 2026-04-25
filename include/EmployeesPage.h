#pragma once
#include "DataModels.h"
#include "PageBase.h"
#include <vector>
class EmployeesPage : public PageBase {
public:
    EmployeesPage();
protected:
    const wchar_t* ClassName() const override;
    void OnCreate() override;
    void OnActivate() override;
    void OnSize(int width,int height) override;
    void OnPaint(HDC hdc,const RECT& client) override;
private:
    void ReloadData();

    HWND employeesTable_{};
    std::vector<data::EmployeePerformanceRecord> employees_{};
};
