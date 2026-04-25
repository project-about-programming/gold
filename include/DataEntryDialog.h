#pragma once

#include <windows.h>

#include <string>
#include <vector>

namespace ui {

struct DataEntryField {
    std::wstring label;
    std::wstring placeholder;
    std::wstring value;
    bool required{true};
};

bool ShowDataEntryDialog(HWND owner, const std::wstring& title, const std::wstring& subtitle, std::vector<DataEntryField>& fields);

} // namespace ui
