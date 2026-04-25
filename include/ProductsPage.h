#pragma once
#include "DataModels.h"
#include "PageBase.h"
#include <vector>
class ProductsPage : public PageBase {
public:
    ProductsPage();
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
    data::ProductFilter BuildFilter() const;
    int SelectedProductIndex() const;

    HWND searchEdit_{}; HWND categoryCombo_{}; HWND statusCombo_{}; HWND addButton_{}; HWND editButton_{}; HWND removeButton_{}; HWND importButton_{}; HWND productsTable_{};
    std::vector<data::ProductRecord> products_{};
};
