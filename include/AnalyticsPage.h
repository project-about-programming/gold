#pragma once
#include "PageBase.h"
class AnalyticsPage : public PageBase {
public:
    AnalyticsPage();
protected:
    const wchar_t* ClassName() const override;
    void OnCreate() override;
    void OnSize(int width,int height) override;
    void OnPaint(HDC hdc,const RECT& client) override;
};
