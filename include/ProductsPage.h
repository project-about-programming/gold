#pragma once
#include "DataModels.h"
#include "DataEntryDialog.h"
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
    LRESULT OnNotify(WPARAM wParam, LPARAM lParam) override;
private:
    void ApplyAccess();
    void ReloadData();
    void ReloadLookups();
    void ReloadMovementAndAlerts();
    data::ProductFilter BuildFilter() const;
    int SelectedProductIndex() const;
    data::ProductRecord BuildProductFromDialog(const std::vector<ui::DataEntryField>& fields, const data::ProductRecord* existing) const;

    HWND searchEdit_{}; HWND categoryCombo_{}; HWND supplierCombo_{}; HWND statusCombo_{}; HWND expiryCombo_{}; HWND addButton_{}; HWND editButton_{}; HWND stockInButton_{}; HWND stockOutButton_{}; HWND removeButton_{}; HWND importButton_{}; HWND exportButton_{}; HWND refreshButton_{}; HWND productsTable_{}; HWND movementsTable_{}; HWND alertsTable_{};
    std::vector<data::ProductRecord> products_{};
    std::vector<data::CategoryRecord> categories_{};
    std::vector<data::SupplierRecord> suppliers_{};
    std::vector<data::StockMovementRecord> movements_{};
    std::vector<data::InventoryAlertRecord> alerts_{};
    data::InventorySummary summary_{};
    int selectedProductId_{};
};
