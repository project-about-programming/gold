#pragma once

#include "PageBase.h"

class ProfilePage : public PageBase {
public:
    ProfilePage();

protected:
    const wchar_t* ClassName() const override;
    void OnCreate() override;
    void OnActivate() override;
    void OnSize(int width, int height) override;
    void OnPaint(HDC hdc, const RECT& client) override;
};
