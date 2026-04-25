#include "MainWindow.h"
#include "DataRepository.h"
#include "Localization.h"
#include "LoginWindow.h"
#include "Theme.h"

#include <commctrl.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    theme::SetDpi(GetDpiForSystem());
    theme::InitializeFonts();
    if (data::Repository::Instance().Initialize()) {
        const auto settings = data::Repository::Instance().LoadSettings();
        theme::ApplyTheme(settings.theme);
        i18n::SetLanguage(settings.language);
    }

    LoginWindow login(instance);
    if (!login.Show()) {
        theme::ReleaseFonts();
        return 0;
    }

    MainWindow window(instance);
    if (!window.Create()) {
        theme::ReleaseFonts();
        return 0;
    }
    int code = window.Run();
    theme::ReleaseFonts();
    return code;
}
