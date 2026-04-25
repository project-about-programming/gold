#include "Theme.h"

namespace theme {
COLORREF kWindowBackground = RGB(243, 247, 252);
COLORREF kSidebarBackground = RGB(17, 26, 42);
COLORREF kSidebarSurface = RGB(28, 42, 64);
COLORREF kPanelBackground = RGB(255, 255, 255);
COLORREF kPanelMuted = RGB(248, 251, 255);
COLORREF kPanelBorder = RGB(219, 228, 240);
COLORREF kPanelBorderStrong = RGB(197, 211, 231);
COLORREF kHeaderBackground = RGB(253, 255, 255);
COLORREF kTextPrimary = RGB(24, 36, 56);
COLORREF kTextSecondary = RGB(96, 111, 133);
COLORREF kAccent = RGB(31, 107, 214);
COLORREF kAccentSoft = RGB(226, 239, 255);
COLORREF kSuccessSoft = RGB(229, 247, 240);
COLORREF kWarningSoft = RGB(255, 245, 226);
COLORREF kDangerSoft = RGB(254, 237, 235);
COLORREF kSuccess = RGB(24, 141, 94);
COLORREF kWarning = RGB(179, 118, 28);
COLORREF kDanger = RGB(198, 67, 55);
COLORREF kBlue = RGB(45, 112, 214);
COLORREF kGreen = RGB(32, 137, 96);
COLORREF kAmber = RGB(185, 126, 37);
COLORREF kSlate = RGB(108, 123, 145);
COLORREF kTableAlternate = RGB(250, 252, 255);
COLORREF kTableSelection = RGB(230, 241, 255);

namespace {
HFONT g_title = nullptr;
HFONT g_subtitle = nullptr;
HFONT g_body = nullptr;
HFONT g_bodyBold = nullptr;
HFONT g_small = nullptr;
HFONT g_smallBold = nullptr;
HFONT g_kpiValue = nullptr;
HFONT g_nav = nullptr;
UINT g_dpi = kBaseDpi;

HFONT CreateUiFont(int size, int weight) {
    const int scaledSize = MulDiv(size, static_cast<int>(g_dpi), static_cast<int>(kBaseDpi));
    return CreateFontW(-scaledSize, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                       VARIABLE_PITCH, L"Segoe UI");
}

void ApplyCorporateLight() {
    kWindowBackground = RGB(243, 247, 252);
    kSidebarBackground = RGB(17, 26, 42);
    kSidebarSurface = RGB(28, 42, 64);
    kPanelBackground = RGB(255, 255, 255);
    kPanelMuted = RGB(248, 251, 255);
    kPanelBorder = RGB(219, 228, 240);
    kPanelBorderStrong = RGB(197, 211, 231);
    kHeaderBackground = RGB(253, 255, 255);
    kTextPrimary = RGB(24, 36, 56);
    kTextSecondary = RGB(96, 111, 133);
    kAccent = RGB(31, 107, 214);
    kAccentSoft = RGB(226, 239, 255);
    kSuccessSoft = RGB(229, 247, 240);
    kWarningSoft = RGB(255, 245, 226);
    kDangerSoft = RGB(254, 237, 235);
    kSuccess = RGB(24, 141, 94);
    kWarning = RGB(179, 118, 28);
    kDanger = RGB(198, 67, 55);
    kBlue = RGB(45, 112, 214);
    kGreen = RGB(32, 137, 96);
    kAmber = RGB(185, 126, 37);
    kSlate = RGB(108, 123, 145);
    kTableAlternate = RGB(250, 252, 255);
    kTableSelection = RGB(230, 241, 255);
}

void ApplySlateContrast() {
    kWindowBackground = RGB(230, 235, 242);
    kSidebarBackground = RGB(17, 24, 39);
    kSidebarSurface = RGB(30, 41, 59);
    kPanelBackground = RGB(250, 252, 255);
    kPanelMuted = RGB(238, 243, 249);
    kPanelBorder = RGB(199, 210, 224);
    kPanelBorderStrong = RGB(169, 184, 204);
    kHeaderBackground = RGB(247, 250, 253);
    kTextPrimary = RGB(15, 23, 42);
    kTextSecondary = RGB(71, 85, 105);
    kAccent = RGB(29, 78, 216);
    kAccentSoft = RGB(219, 234, 254);
    kSuccessSoft = RGB(220, 252, 231);
    kWarningSoft = RGB(254, 243, 199);
    kDangerSoft = RGB(254, 226, 226);
    kSuccess = RGB(22, 101, 52);
    kWarning = RGB(146, 64, 14);
    kDanger = RGB(185, 28, 28);
    kBlue = RGB(30, 64, 175);
    kGreen = RGB(21, 128, 61);
    kAmber = RGB(180, 83, 9);
    kSlate = RGB(71, 85, 105);
    kTableAlternate = RGB(244, 247, 251);
    kTableSelection = RGB(219, 234, 254);
}

void ApplyPresentationMode() {
    kWindowBackground = RGB(246, 247, 250);
    kSidebarBackground = RGB(29, 37, 53);
    kSidebarSurface = RGB(40, 52, 73);
    kPanelBackground = RGB(255, 255, 255);
    kPanelMuted = RGB(249, 250, 252);
    kPanelBorder = RGB(212, 219, 229);
    kPanelBorderStrong = RGB(190, 201, 216);
    kHeaderBackground = RGB(255, 255, 255);
    kTextPrimary = RGB(22, 29, 43);
    kTextSecondary = RGB(82, 93, 112);
    kAccent = RGB(0, 103, 184);
    kAccentSoft = RGB(225, 240, 255);
    kSuccessSoft = RGB(228, 246, 238);
    kWarningSoft = RGB(255, 244, 222);
    kDangerSoft = RGB(253, 235, 234);
    kSuccess = RGB(16, 124, 65);
    kWarning = RGB(156, 101, 0);
    kDanger = RGB(190, 62, 50);
    kBlue = RGB(0, 103, 184);
    kGreen = RGB(16, 124, 65);
    kAmber = RGB(156, 101, 0);
    kSlate = RGB(91, 103, 125);
    kTableAlternate = RGB(248, 250, 252);
    kTableSelection = RGB(222, 238, 255);
}
}

void InitializeFonts() {
    if (g_body) return;
    g_title = CreateUiFont(27, FW_BOLD);
    g_subtitle = CreateUiFont(12, FW_NORMAL);
    g_body = CreateUiFont(15, FW_NORMAL);
    g_bodyBold = CreateUiFont(15, 600);
    g_small = CreateUiFont(12, FW_NORMAL);
    g_smallBold = CreateUiFont(12, 600);
    g_kpiValue = CreateUiFont(32, FW_BOLD);
    g_nav = CreateUiFont(16, 600);
}

void ReleaseFonts() {
    DeleteObject(g_title); DeleteObject(g_subtitle); DeleteObject(g_body); DeleteObject(g_bodyBold);
    DeleteObject(g_small); DeleteObject(g_smallBold); DeleteObject(g_kpiValue); DeleteObject(g_nav);
    g_title = g_subtitle = g_body = g_bodyBold = g_small = g_smallBold = g_kpiValue = g_nav = nullptr;
}

void SetDpi(UINT dpi) {
    const UINT next = dpi == 0 ? kBaseDpi : dpi;
    if (g_dpi == next && g_body) {
        return;
    }
    g_dpi = next;
    ReleaseFonts();
    InitializeFonts();
}

UINT CurrentDpi() {
    return g_dpi;
}

int Scale(int value) {
    return MulDiv(value, static_cast<int>(g_dpi), static_cast<int>(kBaseDpi));
}

void ApplyTheme(const std::wstring& themeName) {
    if (themeName == L"Slate Contrast") {
        ApplySlateContrast();
    } else if (themeName == L"Presentation Mode") {
        ApplyPresentationMode();
    } else {
        ApplyCorporateLight();
    }
}

HFONT TitleFont() { return g_title; }
HFONT SubtitleFont() { return g_subtitle; }
HFONT BodyFont() { return g_body; }
HFONT BodyBoldFont() { return g_bodyBold; }
HFONT SmallFont() { return g_small; }
HFONT SmallBoldFont() { return g_smallBold; }
HFONT KpiValueFont() { return g_kpiValue; }
HFONT NavFont() { return g_nav; }

} // namespace theme

