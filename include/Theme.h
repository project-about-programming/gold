#pragma once

#include <windows.h>
#include <string>

namespace theme {

constexpr UINT kBaseDpi = 96;

extern COLORREF kWindowBackground;
extern COLORREF kSidebarBackground;
extern COLORREF kSidebarSurface;
extern COLORREF kPanelBackground;
extern COLORREF kPanelMuted;
extern COLORREF kPanelBorder;
extern COLORREF kPanelBorderStrong;
extern COLORREF kHeaderBackground;
extern COLORREF kTextPrimary;
extern COLORREF kTextSecondary;
extern COLORREF kAccent;
extern COLORREF kAccentSoft;
extern COLORREF kSuccessSoft;
extern COLORREF kWarningSoft;
extern COLORREF kDangerSoft;
extern COLORREF kSuccess;
extern COLORREF kWarning;
extern COLORREF kDanger;
extern COLORREF kBlue;
extern COLORREF kGreen;
extern COLORREF kAmber;
extern COLORREF kSlate;
extern COLORREF kTableAlternate;
extern COLORREF kTableSelection;

void InitializeFonts();
void ReleaseFonts();
void SetDpi(UINT dpi);
UINT CurrentDpi();
int Scale(int value);
void ApplyTheme(const std::wstring& themeName);

HFONT TitleFont();
HFONT SubtitleFont();
HFONT BodyFont();
HFONT BodyBoldFont();
HFONT SmallFont();
HFONT SmallBoldFont();
HFONT KpiValueFont();
HFONT NavFont();

} // namespace theme
