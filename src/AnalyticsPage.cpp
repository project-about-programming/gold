#include "AnalyticsPage.h"

#include "DataRepository.h"
#include "Localization.h"
#include "Theme.h"
#include "UiHelpers.h"

#include <algorithm>

namespace {

std::wstring UiText(const wchar_t* en, const wchar_t* ru) {
    const std::wstring language = i18n::CurrentLanguage();
    return (language == L"Russian" || language == L"Русский") ? std::wstring(ru) : std::wstring(en);
}

struct AnalyticsLayout {
    RECT title{};
    RECT topLeft{};
    RECT topRight{};
    RECT bottomLeft{};
    RECT bottomRight{};
};

AnalyticsLayout BuildLayout(const RECT& client) {
    AnalyticsLayout layout{};
    const auto m = ui::GetMetrics();
    RECT area = ui::Inset(client, m.outerPadding, m.outerPadding);
    layout.title = ui::TakeTop(&area, m.titleBlockHeight, m.sectionGap);
    const auto rows = ui::SplitRows(area, 2, m.sectionGap);
    const auto top = ui::SplitColumns(rows[0], 2, m.sectionGap);
    const auto bottom = ui::SplitColumns(rows[1], 2, m.sectionGap);
    layout.topLeft = top[0];
    layout.topRight = top[1];
    layout.bottomLeft = bottom[0];
    layout.bottomRight = bottom[1];
    return layout;
}

} // namespace

AnalyticsPage::AnalyticsPage() : PageBase(L"Advanced Analytics", L"Trend analysis, best-performing categories, and comparative sales dynamics.") {}

const wchar_t* AnalyticsPage::ClassName() const { return L"NativeAnalyticsPage"; }

void AnalyticsPage::OnCreate() {}

void AnalyticsPage::OnSize(int, int) {}

void AnalyticsPage::OnPaint(HDC hdc, const RECT& client) {
    const AnalyticsLayout layout = BuildLayout(client);
    ui::FillRectColor(hdc, client, theme::kWindowBackground);
    ui::DrawSectionTitle(hdc, layout.title.left, layout.title.top, UiText(L"Analytics Workspace", L"Аналитическое пространство"),
        UiText(L"A more polished 2x2 analytics grid with tighter alignment, less dead space, and clearer chart framing.", L"Более аккуратная аналитическая сетка 2x2 с лучшим выравниванием и читаемостью."), ui::Width(layout.title));

    ui::DrawLineChartPanel(hdc, layout.topLeft, UiText(L"Revenue Momentum", L"Динамика выручки"), UiText(L"Monthly revenue progression shown as an analytical line series.", L"Помесячная динамика выручки в виде аналитического линейного графика."),
        {{78,L"Jan"},{82,L"Feb"},{86,L"Mar"},{93,L"Apr"},{101,L"May"},{108,L"Jun"},{112,L"Jul"},{118,L"Aug"},{126,L"Sep"},{133,L"Oct"},{140,L"Nov"},{148,L"Dec"}},
        theme::kBlue, 70, 160, L"$", L"k");

    ui::DrawBarChartPanel(hdc, layout.topRight, UiText(L"Best-Selling Categories", L"Лучшие категории"), UiText(L"Category contribution to monitored sales volume.", L"Вклад категорий в отслеживаемый объём продаж."),
        {{34,L"Elect."},{24,L"Hardw."},{18,L"Auto."},{14,L"Log."},{10,L"Conn."}}, theme::kGreen, 40, L"", L"%");

    ui::DrawBarChartPanel(hdc, layout.bottomLeft, UiText(L"Sales by Employee", L"Продажи по сотрудникам"), UiText(L"Closed sales volume comparison across key sales managers.", L"Сравнение объёма закрытых продаж по ключевым менеджерам."),
        {{168,L"A.P."},{149,L"L.S."},{121,L"M.I."},{110,L"D.B."},{96,L"E.K."}}, theme::kAmber, 200, L"", L"");

    std::vector<ui::SeriesPoint> pipeline;
    double maxPipeline = 1.0;
    for (const auto& stage : data::Repository::Instance().LoadPipelineSummary()) {
        pipeline.push_back({static_cast<double>(stage.count), i18n::DataText(stage.stage)});
        maxPipeline = std::max(maxPipeline, static_cast<double>(stage.count + 1));
    }
    if (pipeline.empty()) {
        pipeline.push_back({0.0, L"-"});
    }
    ui::DrawBarChartPanel(hdc, layout.bottomRight, UiText(L"Sales Funnel", L"Sales Funnel"), UiText(L"Lead stages: new, in work, proposal, negotiation, won, and lost.", L"Lead stages: new, in work, proposal, negotiation, won, and lost."),
        pipeline, theme::kAccent, maxPipeline, L"", L"");
}
