#pragma once

#include <windows.h>

#include <string>
#include <vector>

namespace ui {

enum class DataEntryFieldKind {
    Text,
    Combo,
    Number,
    Decimal,
    Date,
    ReadOnly
};

struct DataEntryOption {
    int id{};
    std::wstring text;
};

struct DataEntryField {
    std::wstring label;
    std::wstring placeholder;
    std::wstring value;
    bool required{true};
    DataEntryFieldKind kind{DataEntryFieldKind::Text};
    std::vector<DataEntryOption> options;
    bool storeOptionId{true};
    bool allowCustomValue{false};
    double minValue{};
    double maxValue{};
    bool hasMinValue{false};
    bool hasMaxValue{false};
};

bool ShowDataEntryDialog(HWND owner, const std::wstring& title, const std::wstring& subtitle, std::vector<DataEntryField>& fields);

} // namespace ui
