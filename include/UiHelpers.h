#pragma once

#include <windows.h>
#include <commctrl.h>

#include <string>
#include <vector>

namespace ui {

enum class ButtonKind {
    Navigation,
    Primary,
    Secondary,
    Success,
    Warning,
    Danger
};

enum class FontRole {
    Body,
    BodyBold,
    Small,
    SmallBold,
    Navigation
};

struct SeriesPoint {
    double value;
    std::wstring label;
};

struct Metrics {
    int outerPadding;
    int sectionGap;
    int compactGap;
    int controlHeight;
    int buttonHeight;
    int panelHeaderHeight;
    int kpiHeight;
    int titleBlockHeight;
    int radius;
};

void EnableDoubleBuffering(HWND hwnd);
int Scale(int value);
const Metrics& GetMetrics();
int IconButtonWidth();
void FillRectColor(HDC hdc, const RECT& rc, COLORREF color);
void DrawRoundedPanel(HDC hdc, const RECT& rc, COLORREF fill, COLORREF border, int radius = 18, bool shadow = true);
void DrawTextLine(HDC hdc, const std::wstring& text, RECT rc, HFONT font, COLORREF color, UINT format);
void DrawSectionTitle(HDC hdc, int x, int y, const std::wstring& title, const std::wstring& subtitle, int width);
void DrawPanelHeader(HDC hdc, const RECT& panel, const std::wstring& title, const std::wstring& subtitle);
void DrawEmptyState(HDC hdc, const RECT& rc, const std::wstring& title, const std::wstring& description, const std::wstring& icon = L"\u25A1");
void DrawKpiCard(HDC hdc, const RECT& rc, const std::wstring& title, const std::wstring& value, const std::wstring& caption, COLORREF accent);
void DrawLineChartPanel(HDC hdc, const RECT& rc, const std::wstring& title, const std::wstring& subtitle,
                        const std::vector<SeriesPoint>& points, COLORREF lineColor, double minValue, double maxValue,
                        const std::wstring& yPrefix, const std::wstring& ySuffix);
void DrawBarChartPanel(HDC hdc, const RECT& rc, const std::wstring& title, const std::wstring& subtitle,
                       const std::vector<SeriesPoint>& values, COLORREF barColor, double maxValue,
                       const std::wstring& yPrefix, const std::wstring& ySuffix);
void DrawDonutChartPanel(HDC hdc, const RECT& rc, const std::wstring& title, const std::wstring& subtitle,
                         const std::vector<SeriesPoint>& values, const std::vector<COLORREF>& colors);
void DrawUiButton(const DRAWITEMSTRUCT* dis, ButtonKind kind, bool active);

HWND CreateUiButton(HWND parent, int id, const std::wstring& text, ButtonKind kind);
HWND CreateUiEdit(HWND parent, int id, const std::wstring& text = L"");
HWND CreateUiCombo(HWND parent, int id);
HWND CreateUiDatePicker(HWND parent, int id);
HWND CreateUiListView(HWND parent, int id);
void ApplyStandardTableStyle(HWND list);
void ApplyControlFont(HWND hwnd, HFONT font);
void AssignFontRole(HWND hwnd, FontRole role);
void RefreshWindowFonts(HWND root);
void SetEditCueBanner(HWND hwnd, const std::wstring& text);
void AddComboItems(HWND combo, const std::vector<std::wstring>& items);
void AddListColumns(HWND list, const std::vector<std::wstring>& titles, const std::vector<int>& widths);
void AddListRow(HWND list, int rowIndex, const std::vector<std::wstring>& values);
void ClearListRows(HWND list);
void ClearListSelection(HWND list);
void MoveWindowToRect(HWND hwnd, const RECT& rc, BOOL repaint = TRUE);
void SetButtonKind(HWND hwnd, ButtonKind kind);
ButtonKind GetButtonKind(HWND hwnd);
void SetButtonIconIndex(HWND hwnd, int iconIndex);
int GetButtonIconIndex(HWND hwnd);
bool TryGetDatePickerIso(HWND picker, std::wstring* value);
LRESULT HandleListViewCustomDraw(LPARAM lParam);

RECT MakeRect(int left, int top, int right, int bottom);
RECT Inset(const RECT& rc, int dx, int dy);
RECT ClampRect(const RECT& rc, const RECT& bounds);
RECT TakeTop(RECT* rc, int height, int gapAfter = 0);
RECT TakeBottom(RECT* rc, int height, int gapBefore = 0);
RECT TakeLeft(RECT* rc, int width, int gapAfter = 0);
RECT TakeRight(RECT* rc, int width, int gapBefore = 0);
std::vector<RECT> SplitColumns(const RECT& rc, int count, int gap);
std::vector<RECT> SplitRows(const RECT& rc, int count, int gap);
int Width(const RECT& rc);
int Height(const RECT& rc);
std::wstring FormatCurrency(int valueThousands);

} // namespace ui
