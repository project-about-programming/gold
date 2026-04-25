#pragma once
#include "DataModels.h"
#include "PageBase.h"
#include <vector>
class ClientsPage : public PageBase {
public:
    ClientsPage();
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
    void FillHistory();
    data::ClientFilter BuildFilter() const;

    HWND searchEdit_{}; HWND segmentCombo_{}; HWND sortCombo_{}; HWND addButton_{}; HWND editButton_{}; HWND deleteButton_{}; HWND clientsTable_{}; HWND historyTable_{}; int selectedClient_{};
    std::vector<data::ClientRecord> clients_{};
    std::vector<data::ClientOrderRecord> history_{};
};
