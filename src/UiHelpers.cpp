#include "UiHelpers.h"
#include "Theme.h"

#include <uxtheme.h>

#include <algorithm>
#include <cwctype>
#include <iomanip>
#include <initializer_list>
#include <sstream>

namespace ui {
namespace {
const wchar_t* kButtonKindProp = L"UiButtonKind";
const wchar_t* kFontRoleProp = L"UiFontRole";
const wchar_t* kIconIndexProp = L"UiIconIndex";

HFONT ResolveFont(FontRole role) {
    switch (role) {
    case FontRole::BodyBold:
        return theme::BodyBoldFont();
    case FontRole::Small:
        return theme::SmallFont();
    case FontRole::SmallBold:
        return theme::SmallBoldFont();
    case FontRole::Navigation:
        return theme::NavFont();
    case FontRole::Body:
    default:
        return theme::BodyFont();
    }
}

RECT Shrink(const RECT& rc, int x, int y) {
    RECT r{ rc.left + x, rc.top + y, rc.right - x, rc.bottom - y };
    return r;
}

RECT NormalizeRect(const RECT& rc) {
    RECT normalized{ rc.left, rc.top, rc.right - 1, rc.bottom - 1 };
    return normalized;
}

BOOL CALLBACK RefreshFontProc(HWND hwnd, LPARAM) {
    const auto role = static_cast<FontRole>(reinterpret_cast<INT_PTR>(GetPropW(hwnd, kFontRoleProp)));
    if (GetPropW(hwnd, kFontRoleProp) != nullptr) {
        ApplyControlFont(hwnd, ResolveFont(role));
    }
    return TRUE;
}

void DrawNavIcon(HDC hdc, RECT rc, int index, COLORREF color) {
    HPEN pen = CreatePen(PS_SOLID, Scale(2), color);
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);

    const int cx = (rc.left + rc.right) / 2;
    const int cy = (rc.top + rc.bottom) / 2;
    const int s = Scale(5);

    switch (index) {
    case 0:
        Rectangle(hdc, rc.left, rc.top, cx - Scale(2), cy - Scale(2));
        Rectangle(hdc, cx + Scale(2), rc.top, rc.right, cy - Scale(2));
        Rectangle(hdc, rc.left, cy + Scale(2), cx - Scale(2), rc.bottom);
        Rectangle(hdc, cx + Scale(2), cy + Scale(2), rc.right, rc.bottom);
        break;
    case 1:
        MoveToEx(hdc, rc.left, rc.bottom - Scale(2), nullptr);
        LineTo(hdc, rc.left + Scale(7), cy);
        LineTo(hdc, cx + Scale(2), cy + Scale(3));
        LineTo(hdc, rc.right, rc.top + Scale(3));
        Ellipse(hdc, rc.right - Scale(4), rc.top + Scale(1), rc.right + Scale(2), rc.top + Scale(7));
        break;
    case 2:
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left + Scale(2), rc.top, rc.right - Scale(1), rc.bottom);
        MoveToEx(hdc, rc.left + Scale(5), rc.top + Scale(6), nullptr);
        LineTo(hdc, rc.right - Scale(4), rc.top + Scale(6));
        MoveToEx(hdc, rc.left + Scale(5), rc.top + Scale(12), nullptr);
        LineTo(hdc, rc.right - Scale(4), rc.top + Scale(12));
        break;
    case 3:
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left + Scale(1), rc.top + Scale(3), rc.right - Scale(1), rc.bottom - Scale(1));
        MoveToEx(hdc, rc.left + Scale(1), rc.top + Scale(3), nullptr);
        LineTo(hdc, cx, rc.top - Scale(2));
        LineTo(hdc, rc.right - Scale(1), rc.top + Scale(3));
        break;
    case 4:
        Ellipse(hdc, rc.left + Scale(1), rc.top, rc.left + Scale(9), rc.top + Scale(8));
        Ellipse(hdc, rc.left + Scale(10), rc.top + Scale(3), rc.left + Scale(18), rc.top + Scale(11));
        RoundRect(hdc, rc.left, rc.top + Scale(10), rc.left + Scale(11), rc.bottom, Scale(6), Scale(6));
        RoundRect(hdc, rc.left + Scale(9), rc.top + Scale(13), rc.right, rc.bottom, Scale(6), Scale(6));
        break;
    case 5:
        Ellipse(hdc, cx - s, rc.top, cx + s, rc.top + Scale(10));
        RoundRect(hdc, rc.left + Scale(3), rc.top + Scale(11), rc.right - Scale(3), rc.bottom, Scale(7), Scale(7));
        break;
    case 6:
        Rectangle(hdc, rc.left, cy, rc.left + Scale(4), rc.bottom);
        Rectangle(hdc, cx - Scale(2), rc.top + Scale(5), cx + Scale(2), rc.bottom);
        Rectangle(hdc, rc.right - Scale(4), rc.top, rc.right, rc.bottom);
        break;
    case 7:
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left + Scale(2), rc.top, rc.right - Scale(1), rc.bottom);
        MoveToEx(hdc, rc.left + Scale(5), rc.top + Scale(7), nullptr);
        LineTo(hdc, rc.right - Scale(4), rc.top + Scale(7));
        MoveToEx(hdc, rc.left + Scale(5), rc.top + Scale(12), nullptr);
        LineTo(hdc, rc.right - Scale(6), rc.top + Scale(12));
        MoveToEx(hdc, rc.left + Scale(5), rc.top + Scale(17), nullptr);
        LineTo(hdc, rc.right - Scale(8), rc.top + Scale(17));
        break;
    case 8:
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Ellipse(hdc, cx - Scale(6), cy - Scale(6), cx + Scale(6), cy + Scale(6));
        Ellipse(hdc, cx - Scale(2), cy - Scale(2), cx + Scale(2), cy + Scale(2));
        MoveToEx(hdc, cx, rc.top, nullptr); LineTo(hdc, cx, cy - Scale(7));
        MoveToEx(hdc, cx, cy + Scale(7), nullptr); LineTo(hdc, cx, rc.bottom);
        MoveToEx(hdc, rc.left, cy, nullptr); LineTo(hdc, cx - Scale(7), cy);
        MoveToEx(hdc, cx + Scale(7), cy, nullptr); LineTo(hdc, rc.right, cy);
        break;
    default:
        Ellipse(hdc, cx - s, cy - s, cx + s, cy + s);
        break;
    }

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

bool HasUsableSize(const RECT& rc, int minSize = 4) {
    return (rc.right - rc.left) >= Scale(minSize) && (rc.bottom - rc.top) >= Scale(minSize);
}

bool HasClassName(HWND hwnd, const wchar_t* className) {
    wchar_t buffer[64]{};
    GetClassNameW(hwnd, buffer, 64);
    return std::wstring(buffer) == className;
}

std::wstring LowerCopy(std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return text;
}

bool ContainsAny(const std::wstring& text, std::initializer_list<const wchar_t*> needles) {
    const std::wstring lower = LowerCopy(text);
    for (const wchar_t* needle : needles) {
        if (lower.find(LowerCopy(needle)) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

UINT ColumnFormatForTitle(const std::wstring& title) {
    if (ContainsAny(title, {
            L"qty", L"units", L"revenue", L"margin", L"price", L"total", L"amount", L"cost", L"stock",
            L"reorder", L"discount", L"ltv", L"spent", L"sales", L"value", L"conversion",
            L"кол", L"ед", L"выруч", L"марж", L"цен", L"сум", L"стоим", L"остат", L"порог", L"скид",
            L"продаж", L"конверс", L"доход"
        })) {
        return LVCFMT_RIGHT;
    }
    if (ContainsAny(title, { L"status", L"severity", L"priority", L"role", L"статус", L"важность", L"приоритет", L"роль" })) {
        return LVCFMT_CENTER;
    }
    return LVCFMT_LEFT;
}

bool MatchesAny(const std::wstring& text, std::initializer_list<const wchar_t*> values) {
    const std::wstring lower = LowerCopy(text);
    for (const wchar_t* value : values) {
        if (lower == LowerCopy(value)) {
            return true;
        }
    }
    return false;
}

bool IsPositiveStatus(const std::wstring& text) {
    return MatchesAny(text, {
        L"Active", L"OK", L"Delivered", L"Completed", L"Paid", L"In Stock", L"Stock In", L"Resolved",
        L"Активен", L"Доставлено", L"Завершено", L"Оплачено", L"В наличии",
        L"Aktiv", L"Geliefert", L"Abgeschlossen"
    });
}

bool IsWarningStatus(const std::wstring& text) {
    return MatchesAny(text, {
        L"Pending", L"Warning", L"Low Stock", L"Partial", L"High", L"Medium", L"Stock Out", L"Open",
        L"В ожидании", L"Предупреждение", L"Мало на складе", L"Частично", L"Высокий", L"Средний",
        L"Ausstehend", L"Warnung", L"Niedriger Bestand"
    });
}

bool IsInfoStatus(const std::wstring& text) {
    return MatchesAny(text, {
        L"Processing", L"Confirmed", L"Packed", L"Info",
        L"В обработке", L"Подтверждено", L"Упаковано", L"Инфо",
        L"In Bearbeitung", L"Bestätigt", L"Verpackt"
    });
}

bool IsNeutralStatus(const std::wstring& text) {
    return MatchesAny(text, {
        L"Refunded", L"Returned", L"Suspended", L"Disabled",
        L"Возвращено", L"Возврат", L"Приостановлен", L"Отключён", L"Отключен"
    });
}

bool IsDangerStatus(const std::wstring& text) {
    return MatchesAny(text, {
        L"Critical", L"Expired", L"Discontinued", L"Cancelled", L"Canceled", L"Out of Stock", L"Blocked", L"Failed",
        L"Критично", L"Истёк", L"Истек", L"Снят с продажи", L"Отменено", L"Нет на складе", L"Заблокирован", L"Ошибка",
        L"Kritisch"
    });
}

bool IsBadgeText(const std::wstring& text) {
    return IsPositiveStatus(text) || IsWarningStatus(text) || IsInfoStatus(text) || IsNeutralStatus(text) || IsDangerStatus(text);
}

LRESULT HandleHeaderCustomDraw(LPARAM lParam) {
    auto* draw = reinterpret_cast<NMCUSTOMDRAW*>(lParam);
    switch (draw->dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT: {
        RECT rc = draw->rc;
        FillRectColor(draw->hdc, rc, RGB(241, 245, 249));
        RECT line{ rc.left, rc.bottom - 1, rc.right, rc.bottom };
        FillRectColor(draw->hdc, line, RGB(226, 232, 240));

        wchar_t text[160]{};
        HDITEMW item{};
        item.mask = HDI_TEXT;
        item.pszText = text;
        item.cchTextMax = static_cast<int>(sizeof(text) / sizeof(text[0]));
        Header_GetItem(draw->hdr.hwndFrom, static_cast<int>(draw->dwItemSpec), &item);

        rc.left += Scale(10);
        rc.right -= Scale(8);
        DrawTextLine(draw->hdc, text, rc, theme::SmallBoldFont(), RGB(51, 65, 85),
            DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        return CDRF_SKIPDEFAULT;
    }
    default:
        return CDRF_DODEFAULT;
    }
}
}

void EnableDoubleBuffering(HWND hwnd) {
    (void)hwnd;
    // Descendant controls already use their own buffered painting.
    // For this dashboard, WS_EX_COMPOSITED serializes redraw of the whole
    // hierarchy and causes severe lag/flicker with list views and charts.
}

int Scale(int value) {
    return theme::Scale(value);
}

const Metrics& GetMetrics() {
    static Metrics metrics{};
    metrics.outerPadding = Scale(18);
    metrics.sectionGap = Scale(18);
    metrics.compactGap = Scale(12);
    metrics.controlHeight = Scale(36);
    metrics.buttonHeight = Scale(40);
    metrics.panelHeaderHeight = Scale(64);
    metrics.kpiHeight = Scale(132);
    metrics.titleBlockHeight = Scale(64);
    metrics.radius = Scale(16);
    return metrics;
}

int IconButtonWidth() {
    return Scale(124);
}

void FillRectColor(HDC hdc, const RECT& rc, COLORREF color) {
    if (!HasUsableSize(rc, 1)) {
        return;
    }
    HBRUSH b = CreateSolidBrush(color);
    FillRect(hdc, &rc, b);
    DeleteObject(b);
}

void DrawRoundedPanel(HDC hdc, const RECT& rc, COLORREF fill, COLORREF border, int radius, bool shadow) {
    if (!HasUsableSize(rc, 6)) {
        return;
    }
    RECT panelRc = NormalizeRect(rc);
    if (shadow) {
        RECT shadowRc{ panelRc.left + Scale(2), panelRc.top + Scale(5), panelRc.right + Scale(2), panelRc.bottom + Scale(5) };
        HBRUSH shadowBrush = CreateSolidBrush(RGB(232, 238, 247));
        HGDIOBJ oldBrush = SelectObject(hdc, shadowBrush);
        HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
        RoundRect(hdc, shadowRc.left, shadowRc.top, shadowRc.right, shadowRc.bottom, radius, radius);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(shadowBrush);
    }

    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, panelRc.left, panelRc.top, panelRc.right, panelRc.bottom, radius, radius);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(brush);
    DeleteObject(pen);
}

void DrawTextLine(HDC hdc, const std::wstring& text, RECT rc, HFONT font, COLORREF color, UINT format) {
    if (text.empty() || !HasUsableSize(rc, 1)) {
        return;
    }
    HGDIOBJ oldFont = SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    DrawTextW(hdc, text.c_str(), -1, &rc, format | DT_NOPREFIX);
    SelectObject(hdc, oldFont);
}

void DrawSectionTitle(HDC hdc, int x, int y, const std::wstring& title, const std::wstring& subtitle, int width) {
    if (width <= Scale(24)) {
        return;
    }
    RECT titleRc{ x, y, x + width, y + Scale(28) };
    DrawTextLine(hdc, title, titleRc, theme::BodyBoldFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    if (!subtitle.empty()) {
        RECT subtitleRc{ x, y + Scale(29), x + width, y + Scale(52) };
        DrawTextLine(hdc, subtitle, subtitleRc, theme::SmallFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    }
}

void DrawPanelHeader(HDC hdc, const RECT& panel, const std::wstring& title, const std::wstring& subtitle) {
    DrawSectionTitle(hdc, panel.left + Scale(18), panel.top + Scale(14), title, subtitle, Width(panel) - Scale(36));
}

void DrawEmptyState(HDC hdc, const RECT& rc, const std::wstring& title, const std::wstring& description, const std::wstring& icon) {
    if (!HasUsableSize(rc, 60)) {
        return;
    }
    const int boxWidth = std::min(Scale(420), std::max(Scale(220), Width(rc) - Scale(40)));
    const int boxHeight = Scale(118);
    RECT box{
        rc.left + (Width(rc) - boxWidth) / 2,
        rc.top + (Height(rc) - boxHeight) / 2,
        rc.left + (Width(rc) + boxWidth) / 2,
        rc.top + (Height(rc) + boxHeight) / 2
    };
    DrawTextLine(hdc, icon, MakeRect(box.left, box.top, box.right, box.top + Scale(32)),
        theme::BodyBoldFont(), RGB(148, 163, 184), DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    DrawTextLine(hdc, title, MakeRect(box.left, box.top + Scale(42), box.right, box.top + Scale(68)),
        theme::BodyBoldFont(), RGB(51, 65, 85), DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    DrawTextLine(hdc, description, MakeRect(box.left, box.top + Scale(74), box.right, box.bottom),
        theme::SmallFont(), RGB(100, 116, 139), DT_CENTER | DT_WORDBREAK | DT_END_ELLIPSIS);
}

void DrawKpiCard(HDC hdc, const RECT& rc, const std::wstring& title, const std::wstring& value, const std::wstring& caption, COLORREF accent) {
    if (!HasUsableSize(rc, 40)) {
        return;
    }
    DrawRoundedPanel(hdc, rc, theme::kPanelBackground, theme::kPanelBorder, 18, true);
    RECT topBar{ rc.left + Scale(18), rc.top + Scale(16), rc.left + Scale(84), rc.top + Scale(20) };
    FillRectColor(hdc, topBar, accent);
    RECT titleRc{ rc.left + Scale(18), rc.top + Scale(28), rc.right - Scale(18), rc.top + Scale(54) };
    DrawTextLine(hdc, title, titleRc, theme::SmallBoldFont(), theme::kTextSecondary, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    RECT valueRc{ rc.left + Scale(18), rc.top + Scale(54), rc.right - Scale(18), rc.top + Scale(84) };
    DrawTextLine(hdc, value, valueRc, theme::KpiValueFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    RECT captionRc{ rc.left + Scale(18), rc.top + Scale(92), rc.right - Scale(18), rc.bottom - Scale(12) };
    DrawTextLine(hdc, caption, captionRc, theme::SmallFont(), accent, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
}

void DrawLineChartPanel(HDC hdc, const RECT& rc, const std::wstring& title, const std::wstring& subtitle,
                        const std::vector<SeriesPoint>& points, COLORREF lineColor, double minValue, double maxValue,
                        const std::wstring& yPrefix, const std::wstring& ySuffix) {
    if (!HasUsableSize(rc, 90)) {
        DrawRoundedPanel(hdc, rc, theme::kPanelBackground, theme::kPanelBorder, Scale(18), true);
        return;
    }
    if (maxValue <= minValue) {
        maxValue = minValue + 1.0;
    }
    DrawRoundedPanel(hdc, rc, theme::kPanelBackground, theme::kPanelBorder, Scale(18), true);
    DrawSectionTitle(hdc, rc.left + Scale(20), rc.top + Scale(16), title, subtitle, rc.right - rc.left - Scale(40));

    RECT plot{ rc.left + Scale(34), rc.top + Scale(72), rc.right - Scale(24), rc.bottom - Scale(40) };
    if (!HasUsableSize(plot, 24)) {
        return;
    }
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(233, 237, 243));
    HGDIOBJ oldPen = SelectObject(hdc, gridPen);
    for (int i = 0; i < 5; ++i) {
        int y = plot.top + ((plot.bottom - plot.top) * i / 4);
        MoveToEx(hdc, plot.left, y, nullptr);
        LineTo(hdc, plot.right, y);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);

    if (points.size() >= 2) {
        HPEN linePen = CreatePen(PS_SOLID, Scale(3), lineColor);
        oldPen = SelectObject(hdc, linePen);
        for (size_t i = 0; i < points.size(); ++i) {
            double norm = (points[i].value - minValue) / (maxValue - minValue);
            norm = std::clamp(norm, 0.0, 1.0);
            int x = plot.left + static_cast<int>((plot.right - plot.left - Scale(20)) * i / static_cast<int>(points.size() - 1)) + Scale(10);
            int y = plot.bottom - static_cast<int>((plot.bottom - plot.top - Scale(20)) * norm) - Scale(10);
            if (i == 0) MoveToEx(hdc, x, y, nullptr); else LineTo(hdc, x, y);
            HBRUSH dotBrush = CreateSolidBrush(lineColor);
            HGDIOBJ oldBrush = SelectObject(hdc, dotBrush);
            HGDIOBJ oldPenObj = SelectObject(hdc, GetStockObject(NULL_PEN));
            Ellipse(hdc, x - Scale(4), y - Scale(4), x + Scale(4), y + Scale(4));
            SelectObject(hdc, oldPenObj);
            SelectObject(hdc, oldBrush);
            DeleteObject(dotBrush);
            RECT labelRc{ x - Scale(18), plot.bottom + Scale(6), x + Scale(22), plot.bottom + Scale(24) };
            DrawTextLine(hdc, points[i].label, labelRc, theme::SmallFont(), theme::kTextSecondary, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        }
        SelectObject(hdc, oldPen);
        DeleteObject(linePen);
    }

    for (int i = 0; i < 5; ++i) {
        double v = maxValue - ((maxValue - minValue) * i / 4.0);
        std::wstringstream ss;
        ss << yPrefix << static_cast<int>(v) << ySuffix;
        RECT yr{ rc.left + Scale(8), plot.top + ((plot.bottom - plot.top) * i / 4) - Scale(8), plot.left - Scale(6), plot.top + ((plot.bottom - plot.top) * i / 4) + Scale(10) };
        DrawTextLine(hdc, ss.str(), yr, theme::SmallFont(), theme::kTextSecondary, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
    }
}

void DrawBarChartPanel(HDC hdc, const RECT& rc, const std::wstring& title, const std::wstring& subtitle,
                       const std::vector<SeriesPoint>& values, COLORREF barColor, double maxValue,
                       const std::wstring& yPrefix, const std::wstring& ySuffix) {
    if (!HasUsableSize(rc, 90)) {
        DrawRoundedPanel(hdc, rc, theme::kPanelBackground, theme::kPanelBorder, Scale(18), true);
        return;
    }
    if (values.empty() || maxValue <= 0.0) {
        maxValue = 1.0;
    }
    DrawRoundedPanel(hdc, rc, theme::kPanelBackground, theme::kPanelBorder, Scale(18), true);
    DrawSectionTitle(hdc, rc.left + Scale(20), rc.top + Scale(16), title, subtitle, rc.right - rc.left - Scale(40));

    RECT plot{ rc.left + Scale(30), rc.top + Scale(72), rc.right - Scale(24), rc.bottom - Scale(40) };
    if (!HasUsableSize(plot, 24)) {
        return;
    }
    for (size_t i = 0; i < values.size(); ++i) {
        int slot = std::max(Scale(8), static_cast<int>(plot.right - plot.left) / static_cast<int>(values.size()));
        int x1 = plot.left + static_cast<int>(i) * slot + Scale(12);
        int x2 = std::max(x1 + Scale(2), x1 + slot - Scale(24));
        x2 = std::min(x2, static_cast<int>(plot.right));
        int barHeight = static_cast<int>((plot.bottom - plot.top - Scale(24)) * (values[i].value / maxValue));
        RECT bar{ x1, plot.bottom - barHeight - Scale(10), x2, plot.bottom - Scale(10) };
        FillRectColor(hdc, bar, barColor);
        RECT label{ x1 - Scale(4), plot.bottom + Scale(2), x2 + Scale(4), plot.bottom + Scale(22) };
        DrawTextLine(hdc, values[i].label, label, theme::SmallFont(), theme::kTextSecondary, DT_CENTER | DT_SINGLELINE);
    }

    for (int i = 0; i < 5; ++i) {
        double v = maxValue - (maxValue * i / 4.0);
        std::wstringstream ss;
        ss << yPrefix << static_cast<int>(v) << ySuffix;
        RECT yr{ rc.left + Scale(4), plot.top + ((plot.bottom - plot.top) * i / 4) - Scale(8), plot.left - Scale(6), plot.top + ((plot.bottom - plot.top) * i / 4) + Scale(8) };
        DrawTextLine(hdc, ss.str(), yr, theme::SmallFont(), theme::kTextSecondary, DT_RIGHT | DT_SINGLELINE);
    }
}

void DrawDonutChartPanel(HDC hdc, const RECT& rc, const std::wstring& title, const std::wstring& subtitle,
                         const std::vector<SeriesPoint>& values, const std::vector<COLORREF>& colors) {
    if (!HasUsableSize(rc, 120)) {
        DrawRoundedPanel(hdc, rc, theme::kPanelBackground, theme::kPanelBorder, Scale(18), true);
        return;
    }
    DrawRoundedPanel(hdc, rc, theme::kPanelBackground, theme::kPanelBorder, Scale(18), true);
    DrawSectionTitle(hdc, rc.left + Scale(20), rc.top + Scale(16), title, subtitle, rc.right - rc.left - Scale(40));
    RECT pie{ rc.left + Scale(24), rc.top + Scale(74), rc.left + Scale(220), rc.bottom - Scale(28) };
    HBRUSH inner = CreateSolidBrush(theme::kPanelBackground);
    Pie(hdc, pie.left, pie.top, pie.right, pie.bottom, pie.left, pie.top, pie.left, pie.top);
    HGDIOBJ oldB = SelectObject(hdc, inner);
    Ellipse(hdc, pie.left + Scale(42), pie.top + Scale(42), pie.right - Scale(42), pie.bottom - Scale(42));
    SelectObject(hdc, oldB);
    DeleteObject(inner);

    int legendY = rc.top + Scale(88);
    for (size_t i = 0; i < values.size(); ++i) {
        RECT sw{ rc.left + Scale(246), legendY + static_cast<int>(i) * Scale(28), rc.left + Scale(262), legendY + Scale(16) + static_cast<int>(i) * Scale(28) };
        FillRectColor(hdc, sw, colors[i % colors.size()]);
        RECT label{ rc.left + Scale(270), legendY - Scale(2) + static_cast<int>(i) * Scale(28), rc.right - Scale(20), legendY + Scale(18) + static_cast<int>(i) * Scale(28) };
        DrawTextLine(hdc, values[i].label, label, theme::SmallFont(), theme::kTextPrimary, DT_LEFT | DT_SINGLELINE);
    }
}

void DrawUiButton(const DRAWITEMSTRUCT* dis, ButtonKind kind, bool active) {
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    COLORREF fill = theme::kPanelBackground;
    COLORREF text = theme::kTextPrimary;
    COLORREF border = theme::kPanelBorder;
    const bool hovered = (dis->itemState & ODS_HOTLIGHT) != 0;
    const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
    const bool disabled = (dis->itemState & ODS_DISABLED) != 0;
    int radius = 12;

    if (kind == ButtonKind::Navigation) {
        fill = active ? RGB(30, 41, 59) : (hovered ? RGB(23, 32, 51) : theme::kSidebarBackground);
        border = fill;
        text = disabled ? RGB(100, 116, 139) : ((active || hovered) ? RGB(255, 255, 255) : RGB(203, 213, 225));
        radius = Scale(6);
    } else if (kind == ButtonKind::Secondary) {
        fill = pressed ? RGB(241, 245, 249) : (hovered ? RGB(248, 250, 252) : RGB(255, 255, 255));
        border = RGB(203, 213, 225);
        text = RGB(51, 65, 85);
    } else if (kind == ButtonKind::Primary) {
        fill = pressed ? RGB(24, 90, 186) : (hovered ? RGB(29, 101, 202) : theme::kAccent);
        border = fill;
        text = RGB(255, 255, 255);
    } else if (kind == ButtonKind::Success) {
        fill = pressed ? RGB(18, 126, 72) : (hovered ? RGB(20, 143, 82) : RGB(22, 163, 74));
        border = fill;
        text = RGB(255, 255, 255);
    } else if (kind == ButtonKind::Warning) {
        fill = pressed ? RGB(180, 83, 9) : (hovered ? RGB(202, 96, 12) : RGB(217, 119, 6));
        border = fill;
        text = RGB(255, 255, 255);
    } else if (kind == ButtonKind::Danger) {
        fill = pressed ? RGB(254, 226, 226) : (hovered ? RGB(254, 235, 235) : RGB(254, 242, 242));
        border = RGB(254, 202, 202);
        text = RGB(220, 38, 38);
    }

    if (disabled) {
        if (kind == ButtonKind::Navigation) {
            fill = theme::kSidebarBackground;
            border = fill;
            text = RGB(100, 116, 139);
        } else {
            fill = RGB(242, 244, 247);
            border = RGB(223, 228, 235);
            text = RGB(148, 156, 169);
        }
    }

    if (kind == ButtonKind::Navigation && !active && !hovered && !disabled) {
        FillRectColor(hdc, rc, theme::kSidebarBackground);
    } else {
        DrawRoundedPanel(hdc, rc, fill, border, radius, false);
    }

    if (kind == ButtonKind::Navigation && active) {
        RECT accent{ rc.left, rc.top + Scale(6), rc.left + Scale(4), rc.bottom - Scale(6) };
        FillRectColor(hdc, accent, RGB(59, 130, 246));
    }

    wchar_t textBuffer[128]{};
    GetWindowTextW(dis->hwndItem, textBuffer, 128);
    RECT textRc = rc;
    if (kind == ButtonKind::Navigation) {
        const int iconSize = Scale(20);
        const int iconTop = rc.top + std::max(0, (Height(rc) - iconSize) / 2);
        RECT iconRc{ rc.left + Scale(18), iconTop, rc.left + Scale(18) + iconSize, iconTop + iconSize };
        DrawNavIcon(hdc, iconRc, GetButtonIconIndex(dis->hwndItem), active ? RGB(96, 165, 250) : text);
        textRc.left += Scale(50);
        textRc.right -= Scale(10);
        DrawTextLine(hdc, textBuffer, textRc, theme::NavFont(), text, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else {
        DrawTextLine(hdc, textBuffer, textRc, theme::SmallBoldFont(), text, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Owner-drawn buttons use their visual state instead of the legacy dotted focus rectangle.
}

HWND CreateUiButton(HWND parent, int id, const std::wstring& text, ButtonKind kind) {
    HWND h = CreateWindowExW(0, L"BUTTON", text.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | BS_OWNERDRAW,
                             0, 0, 100, 34, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    SetButtonKind(h, kind);
    AssignFontRole(h, kind == ButtonKind::Navigation ? FontRole::Navigation : FontRole::SmallBold);
    return h;
}

HWND CreateUiEdit(HWND parent, int id, const std::wstring& text) {
    HWND h = CreateWindowExW(0, L"EDIT", text.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | WS_BORDER | ES_AUTOHSCROLL,
                             0, 0, 120, 32, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    SetWindowTheme(h, L"Explorer", nullptr);
    AssignFontRole(h, FontRole::Body);
    return h;
}

HWND CreateUiCombo(HWND parent, int id) {
    HWND h = CreateWindowExW(0, WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | WS_BORDER | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
                             0, 0, 140, 300, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    SetWindowTheme(h, L"Explorer", nullptr);
    SendMessageW(h, CB_SETMINVISIBLE, 8, 0);
    AssignFontRole(h, FontRole::Body);
    return h;
}

HWND CreateUiDatePicker(HWND parent, int id) {
    HWND h = CreateWindowExW(0, DATETIMEPICK_CLASSW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | WS_BORDER | DTS_SHORTDATEFORMAT | DTS_SHOWNONE,
                             0, 0, 120, 32, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    SetWindowTheme(h, L"Explorer", nullptr);
    DateTime_SetFormat(h, L"dd MMM yyyy");
    AssignFontRole(h, FontRole::Body);
    return h;
}

void ApplyStandardTableStyle(HWND list) {
    if (!list) {
        return;
    }
    ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP | LVS_EX_INFOTIP);
    ListView_SetBkColor(list, RGB(255, 255, 255));
    ListView_SetTextBkColor(list, RGB(255, 255, 255));
    ListView_SetTextColor(list, RGB(15, 23, 42));
    SetWindowTheme(list, L"ItemsView", nullptr);
    ListView_SetItemState(list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    InvalidateRect(list, nullptr, TRUE);
}

HWND CreateUiListView(HWND parent, int id) {
    HWND h = CreateWindowExW(0, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | LVS_REPORT | LVS_SINGLESEL,
                             0, 0, 100, 100, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    HIMAGELIST rowHeight = ImageList_Create(1, Scale(40), ILC_COLOR32, 1, 1);
    ListView_SetImageList(h, rowHeight, LVSIL_SMALL);
    ApplyStandardTableStyle(h);
    AssignFontRole(h, FontRole::Body);
    return h;
}

void ApplyControlFont(HWND hwnd, HFONT font) {
    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

void AssignFontRole(HWND hwnd, FontRole role) {
    SetPropW(hwnd, kFontRoleProp, reinterpret_cast<HANDLE>(static_cast<INT_PTR>(role)));
    ApplyControlFont(hwnd, ResolveFont(role));
}

void RefreshWindowFonts(HWND root) {
    EnumChildWindows(root, RefreshFontProc, 0);
}

void SetEditCueBanner(HWND hwnd, const std::wstring& text) {
    SendMessageW(hwnd, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(text.c_str()));
}

void AddComboItems(HWND combo, const std::vector<std::wstring>& items) {
    for (const auto& item : items) SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
    SendMessageW(combo, CB_SETCURSEL, 0, 0);
}

void AddListColumns(HWND list, const std::vector<std::wstring>& titles, const std::vector<int>& widths) {
    for (size_t i = 0; i < titles.size(); ++i) {
        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        col.fmt = ColumnFormatForTitle(titles[i]);
        col.pszText = const_cast<LPWSTR>(titles[i].c_str());
        col.cx = widths[i];
        ListView_InsertColumn(list, static_cast<int>(i), &col);
    }
}

void AddListRow(HWND list, int rowIndex, const std::vector<std::wstring>& values) {
    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = rowIndex;
    item.pszText = const_cast<LPWSTR>(values[0].c_str());
    ListView_InsertItem(list, &item);
    for (size_t i = 1; i < values.size(); ++i) {
        ListView_SetItemText(list, rowIndex, static_cast<int>(i), const_cast<LPWSTR>(values[i].c_str()));
    }
    ListView_SetItemState(list, rowIndex, 0, LVIS_SELECTED | LVIS_FOCUSED);
}

void ClearListRows(HWND list) {
    ListView_DeleteAllItems(list);
    ClearListSelection(list);
}

void ClearListSelection(HWND list) {
    if (!list) {
        return;
    }
    ListView_SetItemState(list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
}

void MoveWindowToRect(HWND hwnd, const RECT& rc, BOOL repaint) {
    if (!hwnd) {
        return;
    }
    const int width = Width(rc);
    const int height = Height(rc);
    MoveWindow(hwnd, rc.left, rc.top, width, height, repaint);
}

void SetButtonKind(HWND hwnd, ButtonKind kind) {
    SetPropW(hwnd, kButtonKindProp, reinterpret_cast<HANDLE>(static_cast<INT_PTR>(kind)));
}

ButtonKind GetButtonKind(HWND hwnd) {
    return static_cast<ButtonKind>(reinterpret_cast<INT_PTR>(GetPropW(hwnd, kButtonKindProp)));
}

void SetButtonIconIndex(HWND hwnd, int iconIndex) {
    SetPropW(hwnd, kIconIndexProp, reinterpret_cast<HANDLE>(static_cast<INT_PTR>(iconIndex)));
}

int GetButtonIconIndex(HWND hwnd) {
    HANDLE value = GetPropW(hwnd, kIconIndexProp);
    if (!value) {
        return GetDlgCtrlID(hwnd) - 1000;
    }
    return static_cast<int>(reinterpret_cast<INT_PTR>(value));
}

bool TryGetDatePickerIso(HWND picker, std::wstring* value) {
    SYSTEMTIME date{};
    const DWORD state = DateTime_GetSystemtime(picker, &date);
    if (state != GDT_VALID) {
        if (value) {
            value->clear();
        }
        return false;
    }

    if (value) {
        std::wostringstream stream;
        stream << std::setfill(L'0') << std::setw(4) << date.wYear << L"-"
               << std::setw(2) << date.wMonth << L"-"
               << std::setw(2) << date.wDay;
        *value = stream.str();
    }
    return true;
}

LRESULT HandleListViewCustomDraw(LPARAM lParam) {
    const auto* header = reinterpret_cast<const NMHDR*>(lParam);
    if (header && HasClassName(header->hwndFrom, WC_HEADERW)) {
        return HandleHeaderCustomDraw(lParam);
    }

    auto* draw = reinterpret_cast<NMLVCUSTOMDRAW*>(lParam);
    switch (draw->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
        return CDRF_NOTIFYSUBITEMDRAW;
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
        const bool selected = (ListView_GetItemState(draw->nmcd.hdr.hwndFrom,
            static_cast<int>(draw->nmcd.dwItemSpec), LVIS_SELECTED) & LVIS_SELECTED) != 0;
        const bool hot = (draw->nmcd.uItemState & CDIS_HOT) != 0;
        const COLORREF rowBg = selected ? RGB(219, 234, 254)
            : (hot ? RGB(239, 246, 255) : ((draw->nmcd.dwItemSpec % 2 == 0) ? RGB(255, 255, 255) : RGB(248, 250, 252)));
        draw->clrTextBk = rowBg;
        draw->clrText = selected ? RGB(15, 23, 42) : RGB(51, 65, 85);

        RECT cell{};
        if (draw->iSubItem == 0) {
            if (!ListView_GetItemRect(draw->nmcd.hdr.hwndFrom, static_cast<int>(draw->nmcd.dwItemSpec), &cell, LVIR_BOUNDS)) {
                return CDRF_DODEFAULT;
            }
        } else {
            if (!ListView_GetSubItemRect(draw->nmcd.hdr.hwndFrom, static_cast<int>(draw->nmcd.dwItemSpec), draw->iSubItem, LVIR_BOUNDS, &cell)) {
                return CDRF_DODEFAULT;
            }
        }
        const int columnWidth = ListView_GetColumnWidth(draw->nmcd.hdr.hwndFrom, draw->iSubItem);
        if (columnWidth > 0) {
            cell.right = std::min(cell.right, cell.left + columnWidth);
        }
        if (Width(cell) <= Scale(8)) {
            return CDRF_SKIPDEFAULT;
        }
        FillRectColor(draw->nmcd.hdc, cell, rowBg);
        RECT bottomLine{ cell.left, cell.bottom - 1, cell.right, cell.bottom };
        FillRectColor(draw->nmcd.hdc, bottomLine, RGB(226, 232, 240));

        wchar_t cellText[128]{};
        ListView_GetItemText(draw->nmcd.hdr.hwndFrom, static_cast<int>(draw->nmcd.dwItemSpec), draw->iSubItem, cellText, 128);
        const std::wstring status = cellText;
        if (IsBadgeText(status)) {
            RECT measure{ 0, 0, Width(cell), Scale(24) };
            HGDIOBJ oldFont = SelectObject(draw->nmcd.hdc, theme::SmallBoldFont());
            DrawTextW(draw->nmcd.hdc, status.c_str(), -1, &measure, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
            SelectObject(draw->nmcd.hdc, oldFont);
            const int maxBadgeWidth = std::max(Scale(24), Width(cell) - Scale(10));
            const int minBadgeWidth = std::min(Scale(44), maxBadgeWidth);
            const int desiredWidth = std::clamp(Width(measure) + Scale(22), minBadgeWidth, maxBadgeWidth);
            const int cellLeft = static_cast<int>(cell.left);
            const int cellRight = static_cast<int>(cell.right);
            int badgeLeft = cellLeft + (Width(cell) - desiredWidth) / 2;
            badgeLeft = std::max(cellLeft + Scale(5), std::min(badgeLeft, cellRight - desiredWidth - Scale(5)));
            const int badgeTop = cell.top + std::max(Scale(4), (Height(cell) - Scale(24)) / 2);
            RECT badge{ badgeLeft, badgeTop, std::min(cellRight - Scale(5), badgeLeft + desiredWidth), badgeTop + Scale(24) };
            COLORREF fill = RGB(220, 252, 231);
            COLORREF border = RGB(187, 247, 208);
            COLORREF text = RGB(22, 101, 52);
            if (IsWarningStatus(status)) {
                fill = RGB(254, 243, 199);
                border = RGB(253, 230, 138);
                text = RGB(146, 64, 14);
            } else if (IsInfoStatus(status)) {
                fill = RGB(219, 234, 254);
                border = RGB(191, 219, 254);
                text = RGB(29, 78, 216);
            } else if (IsNeutralStatus(status)) {
                fill = RGB(241, 245, 249);
                border = RGB(203, 213, 225);
                text = RGB(71, 85, 105);
            } else if (IsDangerStatus(status)) {
                fill = RGB(254, 242, 242);
                border = RGB(254, 202, 202);
                text = RGB(220, 38, 38);
            }
            DrawRoundedPanel(draw->nmcd.hdc, badge, fill, border, Scale(10), false);
            DrawTextLine(draw->nmcd.hdc, status, badge, theme::SmallBoldFont(), text, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            return CDRF_SKIPDEFAULT;
        }

        LVCOLUMNW column{};
        column.mask = LVCF_FMT;
        ListView_GetColumn(draw->nmcd.hdr.hwndFrom, draw->iSubItem, &column);
        RECT textRc{ cell.left + Scale(10), cell.top, cell.right - Scale(10), cell.bottom };
        UINT format = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS;
        if ((column.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_RIGHT) {
            format |= DT_RIGHT;
        } else if ((column.fmt & LVCFMT_JUSTIFYMASK) == LVCFMT_CENTER) {
            format |= DT_CENTER;
        } else {
            format |= DT_LEFT;
        }
        DrawTextLine(draw->nmcd.hdc, status, textRc, theme::BodyFont(), draw->clrText, format);
        return CDRF_SKIPDEFAULT;
    }
    default:
        return CDRF_DODEFAULT;
    }
}

RECT MakeRect(int left, int top, int right, int bottom) {
    RECT rc{ left, top, right, bottom };
    return rc;
}

RECT Inset(const RECT& rc, int dx, int dy) {
    RECT r = Shrink(rc, dx, dy);
    if (r.right < r.left) {
        const int mid = (r.left + r.right) / 2;
        r.left = mid;
        r.right = mid;
    }
    if (r.bottom < r.top) {
        const int mid = (r.top + r.bottom) / 2;
        r.top = mid;
        r.bottom = mid;
    }
    return r;
}

RECT ClampRect(const RECT& rc, const RECT& bounds) {
    RECT result{
        std::max(bounds.left, std::min(rc.left, bounds.right)),
        std::max(bounds.top, std::min(rc.top, bounds.bottom)),
        std::max(bounds.left, std::min(rc.right, bounds.right)),
        std::max(bounds.top, std::min(rc.bottom, bounds.bottom))
    };
    if (result.right < result.left) {
        result.right = result.left;
    }
    if (result.bottom < result.top) {
        result.bottom = result.top;
    }
    return result;
}

RECT TakeTop(RECT* rc, int height, int gapAfter) {
    RECT part{ rc->left, rc->top, rc->right, std::min(rc->bottom, rc->top + height) };
    rc->top = std::min(rc->bottom, part.bottom + gapAfter);
    return part;
}

RECT TakeBottom(RECT* rc, int height, int gapBefore) {
    RECT part{ rc->left, std::max(rc->top, rc->bottom - height), rc->right, rc->bottom };
    rc->bottom = std::max(rc->top, part.top - gapBefore);
    return part;
}

RECT TakeLeft(RECT* rc, int width, int gapAfter) {
    RECT part{ rc->left, rc->top, std::min(rc->right, rc->left + width), rc->bottom };
    rc->left = std::min(rc->right, part.right + gapAfter);
    return part;
}

RECT TakeRight(RECT* rc, int width, int gapBefore) {
    RECT part{ std::max(rc->left, rc->right - width), rc->top, rc->right, rc->bottom };
    rc->right = std::max(rc->left, part.left - gapBefore);
    return part;
}

std::vector<RECT> SplitColumns(const RECT& rc, int count, int gap) {
    std::vector<RECT> columns;
    if (count <= 0) {
        return columns;
    }
    columns.reserve(static_cast<size_t>(count));
    const int totalGap = gap * (count - 1);
    const int available = std::max(0, Width(rc) - totalGap);
    const int baseWidth = available / count;
    int remainder = available % count;
    int left = rc.left;
    for (int index = 0; index < count; ++index) {
        const int extra = remainder > 0 ? 1 : 0;
        remainder = std::max(0, remainder - 1);
        const int width = baseWidth + extra;
        columns.push_back(RECT{ left, rc.top, left + width, rc.bottom });
        left += width + gap;
    }
    if (!columns.empty()) {
        columns.back().right = rc.right;
    }
    return columns;
}

std::vector<RECT> SplitRows(const RECT& rc, int count, int gap) {
    std::vector<RECT> rows;
    if (count <= 0) {
        return rows;
    }
    rows.reserve(static_cast<size_t>(count));
    const int totalGap = gap * (count - 1);
    const int available = std::max(0, Height(rc) - totalGap);
    const int baseHeight = available / count;
    int remainder = available % count;
    int top = rc.top;
    for (int index = 0; index < count; ++index) {
        const int extra = remainder > 0 ? 1 : 0;
        remainder = std::max(0, remainder - 1);
        const int height = baseHeight + extra;
        rows.push_back(RECT{ rc.left, top, rc.right, top + height });
        top += height + gap;
    }
    if (!rows.empty()) {
        rows.back().bottom = rc.bottom;
    }
    return rows;
}

int Width(const RECT& rc) {
    return std::max(0, static_cast<int>(rc.right - rc.left));
}

int Height(const RECT& rc) {
    return std::max(0, static_cast<int>(rc.bottom - rc.top));
}

std::wstring FormatCurrency(int valueThousands) {
    std::wstringstream ss;
    ss << L"$" << valueThousands << L"k";
    return ss.str();
}

} // namespace ui
