#include "Localization.h"

#include <algorithm>
#include <iterator>

namespace i18n {
namespace {
struct Entry {
    const wchar_t* key;
    const wchar_t* en;
    const wchar_t* ru;
    const wchar_t* de;
};

std::wstring g_language = L"English";

constexpr Entry kEntries[] = {
    {L"app.windowTitle", L"SalesFlow", L"SalesFlow", L"SalesFlow"},
    {L"app.brand", L"SalesFlow", L"SalesFlow", L"SalesFlow"},
    {L"app.subtitle", L"Sales analytics", L"Sales analytics", L"Sales analytics"},
    {L"env.title", L"Environment", L"Среда", L"Umgebung"},
    {L"env.value", L"Win32 + SQLite demo", L"Win32 + SQLite демо", L"Win32 + SQLite Demo"},
    {L"status.demo", L"SalesFlow | Native C++ desktop application", L"SalesFlow | Native C++ desktop application", L"SalesFlow | Native C++ desktop application"},
    {L"header.sqliteConnected", L"SQLite Connected", L"SQLite подключён", L"SQLite verbunden"},
    {L"header.dbOffline", L"Database Offline", L"База данных недоступна", L"Datenbank offline"},
    {L"header.sqlDemo", L"SQL-backed Demo", L"Демо на SQL", L"SQL Demo"},
    {L"header.orders", L"Orders", L"Заказы", L"Bestellungen"},
    {L"header.users", L"Users", L"Пользователи", L"Benutzer"},

    {L"nav.dashboard", L"Dashboard", L"Панель", L"Dashboard"},
    {L"nav.sales", L"Sales", L"Продажи", L"Vertrieb"},
    {L"nav.orders", L"Orders", L"Заказы", L"Bestellungen"},
    {L"nav.products", L"Products", L"Товары", L"Produkte"},
    {L"nav.clients", L"Clients", L"Клиенты", L"Kunden"},
    {L"nav.employees", L"Employees", L"Сотрудники", L"Mitarbeiter"},
    {L"nav.analytics", L"Analytics", L"Аналитика", L"Analyse"},
    {L"nav.reports", L"Reports", L"Отчёты", L"Berichte"},
    {L"nav.settings", L"Settings", L"Настройки", L"Einstellungen"},

    {L"page.dashboard.title", L"Executive Dashboard", L"Главная панель", L"Executive Dashboard"},
    {L"page.dashboard.subtitle", L"Revenue indicators, operational watchlists, and the latest commercial activity.", L"Показатели выручки, операционные риски и последняя коммерческая активность.", L"Revenue indicators, operational watchlists, and the latest commercial activity."},
    {L"page.sales.title", L"Sales Transactions", L"Продажи", L"Sales Transactions"},
    {L"page.sales.subtitle", L"Search, filter, and export SQL-backed sales transaction data.", L"Поиск, фильтрация и экспорт данных продаж из SQL.", L"Search, filter, and export SQL-backed sales transaction data."},
    {L"page.orders.title", L"Orders Management", L"Управление заказами", L"Orders Management"},
    {L"page.orders.subtitle", L"Track client orders, status movement, and fulfillment value.", L"Контроль заказов клиентов, статусов и выполнения.", L"Track client orders, status movement, and fulfillment value."},
    {L"page.products.title", L"Product Catalog", L"Каталог товаров", L"Product Catalog"},
    {L"page.products.subtitle", L"Monitor stock, pricing, categories, and product availability.", L"Контроль остатков, цен, категорий и доступности товаров.", L"Monitor stock, pricing, categories, and product availability."},
    {L"page.clients.title", L"Client Directory", L"Клиенты", L"Client Directory"},
    {L"page.clients.subtitle", L"Review client profiles, contact details, and purchase context.", L"Профили клиентов, контакты и история покупок.", L"Review client profiles, contact details, and purchase context."},
    {L"page.employees.title", L"Employee Performance", L"Эффективность сотрудников", L"Employee Performance"},
    {L"page.employees.subtitle", L"Manager performance, completed sales, and revenue contribution.", L"Результаты менеджеров, закрытые продажи и вклад в выручку.", L"Manager performance, completed sales, and revenue contribution."},
    {L"page.analytics.title", L"Analytics Workspace", L"Аналитика", L"Analytics Workspace"},
    {L"page.analytics.subtitle", L"Trend analysis for revenue, categories, employees, and periods.", L"Анализ трендов по выручке, категориям, сотрудникам и периодам.", L"Trend analysis for revenue, categories, employees, and periods."},
    {L"page.reports.title", L"Reporting Center", L"Центр отчётов", L"Reporting Center"},
    {L"page.reports.subtitle", L"Report templates, export actions, and product expiration monitoring.", L"Шаблоны отчётов, экспорт и контроль сроков годности товаров.", L"Report templates, export actions, and product expiration monitoring."},
    {L"page.settings.title", L"System Settings", L"Настройки системы", L"System Settings"},
    {L"page.settings.subtitle", L"Application preferences, SQLite status, and account access management.", L"Параметры приложения, статус SQLite и управление учётными записями.", L"Application preferences, SQLite status, and account access management."},

    {L"settings.saved", L"Settings saved and applied.", L"Настройки сохранены и применены.", L"Einstellungen wurden gespeichert."},
    {L"settings.saveTitle", L"Save Settings", L"Сохранение настроек", L"Einstellungen speichern"}
};

constexpr Entry kDataEntries[] = {
    {L"Active", L"Active", L"Активен", L"Aktiv"},
    {L"Suspended", L"Suspended", L"Заблокирован", L"Gesperrt"},
    {L"Completed", L"Completed", L"Завершено", L"Abgeschlossen"},
    {L"Pending", L"Pending", L"В ожидании", L"Ausstehend"},
    {L"Processing", L"Processing", L"В обработке", L"In Bearbeitung"},
    {L"Confirmed", L"Confirmed", L"Подтверждено", L"Bestätigt"},
    {L"Packed", L"Packed", L"Упаковано", L"Verpackt"},
    {L"Delivered", L"Delivered", L"Доставлено", L"Geliefert"},
    {L"High", L"High", L"Высокий", L"Hoch"},
    {L"Medium", L"Medium", L"Средний", L"Mittel"},
    {L"Normal", L"Normal", L"Обычный", L"Normal"},
    {L"Watch", L"Watch", L"Наблюдение", L"Beobachtung"},
    {L"Low Stock", L"Low Stock", L"Мало на складе", L"Niedriger Bestand"},
    {L"Discontinued", L"Discontinued", L"Снят с продажи", L"Eingestellt"},
    {L"Critical", L"Critical", L"Критично", L"Kritisch"},
    {L"Warning", L"Warning", L"Предупреждение", L"Warnung"},
    {L"Expired", L"Expired", L"Просрочено", L"Abgelaufen"},
    {L"OK", L"OK", L"Норма", L"OK"},
    {L"Never", L"Never", L"Никогда", L"Nie"},
    {L"Direct", L"Direct", L"Прямой", L"Direkt"},
    {L"Online", L"Online", L"Онлайн", L"Online"},
    {L"Partner", L"Partner", L"Партнёр", L"Partner"},
    {L"Key Account", L"Key Account", L"Ключевой клиент", L"Schlüsselkunde"},
    {L"Mid-Market", L"Mid-Market", L"Средний бизнес", L"Mittelstand"},
    {L"Regional Partner", L"Regional Partner", L"Региональный партнёр", L"Regionalpartner"},
    {L"North America", L"North America", L"Северная Америка", L"Nordamerika"},
    {L"Western Europe", L"Western Europe", L"Западная Европа", L"Westeuropa"},
    {L"Central Europe", L"Central Europe", L"Центральная Европа", L"Mitteleuropa"},
    {L"Eastern Europe", L"Eastern Europe", L"Восточная Европа", L" Osteuropa"},
    {L"Exceeding Target", L"Exceeding Target", L"Выше плана", L"Über Plan"},
    {L"On Track", L"On Track", L"По плану", L"Im Plan"},
    {L"Administrator", L"Administrator", L"Администратор", L"Administrator"},
    {L"Commercial Director", L"Commercial Director", L"Коммерческий директор", L"Vertriebsleiter"},
    {L"Sales Analyst", L"Sales Analyst", L"Аналитик продаж", L"Vertriebsanalyst"},
    {L"Regional Sales Manager", L"Regional Sales Manager", L"Региональный менеджер продаж", L"Regionalvertriebsleiter"},
    {L"Procurement Controller", L"Procurement Controller", L"Контролёр закупок", L"Einkaufscontroller"},
    {L"Senior Sales Manager", L"Senior Sales Manager", L"Старший менеджер продаж", L"Senior Sales Manager"},
    {L"Key Account Manager", L"Key Account Manager", L"Менеджер ключевых клиентов", L"Key Account Manager"},
    {L"Regional Manager", L"Regional Manager", L"Региональный менеджер", L"Regionalmanager"},
    {L"Sales Manager", L"Sales Manager", L"Менеджер продаж", L"Sales Manager"},
    {L"Account Manager", L"Account Manager", L"Аккаунт-менеджер", L"Account Manager"},
    {L"Supervisor", L"Supervisor", L"Супервайзер", L"Supervisor"},
    {L"Sales Operator", L"Sales Operator", L"Оператор продаж", L"Sales Operator"}
};

bool IsRussian() {
    return g_language == L"Russian" || g_language == L"Русский";
}

bool IsGerman() {
    return g_language == L"German" || g_language == L"Deutsch";
}
} // namespace

void SetLanguage(const std::wstring& language) {
    g_language = language.empty() ? L"English" : language;
}

const std::wstring& CurrentLanguage() {
    return g_language;
}

std::wstring Text(const std::wstring& key) {
    const auto it = std::find_if(std::begin(kEntries), std::end(kEntries), [&](const Entry& entry) {
        return key == entry.key;
    });
    if (it == std::end(kEntries)) {
        return key;
    }
    if (IsRussian()) {
        return it->ru;
    }
    if (IsGerman()) {
        return it->de;
    }
    return it->en;
}

std::wstring DataText(const std::wstring& value) {
    const auto it = std::find_if(std::begin(kDataEntries), std::end(kDataEntries), [&](const Entry& entry) {
        return value == entry.key;
    });
    if (it == std::end(kDataEntries)) {
        return value;
    }
    if (IsRussian()) {
        return it->ru;
    }
    if (IsGerman()) {
        return it->de;
    }
    return it->en;
}

std::wstring MonthShort(int month) {
    static const wchar_t* kMonthsEn[] = {L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};
    static const wchar_t* kMonthsRu[] = {L"Янв", L"Фев", L"Мар", L"Апр", L"Май", L"Июн", L"Июл", L"Авг", L"Сен", L"Окт", L"Ноя", L"Дек"};
    static const wchar_t* kMonthsDe[] = {L"Jan", L"Feb", L"Mär", L"Apr", L"Mai", L"Jun", L"Jul", L"Aug", L"Sep", L"Okt", L"Nov", L"Dez"};

    if (month < 1 || month > 12) {
        return L"";
    }
    if (IsRussian()) {
        return kMonthsRu[month - 1];
    }
    if (IsGerman()) {
        return kMonthsDe[month - 1];
    }
    return kMonthsEn[month - 1];
}

} // namespace i18n
