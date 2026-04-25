#pragma once

#include <string>

namespace i18n {

void SetLanguage(const std::wstring& language);
const std::wstring& CurrentLanguage();
std::wstring Text(const std::wstring& key);
std::wstring DataText(const std::wstring& value);
std::wstring MonthShort(int month);

} // namespace i18n
