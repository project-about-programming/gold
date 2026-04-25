# SalesFlow

## English

SalesFlow is a native Windows desktop application written in pure C++ with WinAPI and SQLite.
The project is a sales monitoring system with demo data, role-based access, analytics pages, CRUD actions, and local export features.

### What The Application Does

- shows a dashboard with revenue, orders, products, and client metrics
- provides pages for sales, orders, products, clients, employees, analytics, reports, and settings
- stores demo and working data in SQLite
- supports login and registration
- supports role-based access for administrator, director, head of sales, sales manager, and product manager
- exports reports and CSV files to `bin/Release/exports/`

### Default Login

Use one of the demo accounts below:

- username: `a.petrova`
- password: `demo123`

Other seeded users also use `demo123`.

### Build And Run

#### Visual Studio

1. Open [SalesMonitoringSystemNative.sln](./SalesMonitoringSystemNative.sln)
2. Select `Release | x64`
3. Build the solution
4. Run the application

#### Output File

The compiled executable is usually created as:

- `bin/Release/SalesFlow.exe`

### Project Structure

```text
SalesMonitoringSystemNative/
  database/                          SQLite schema
  include/                           Header files
  src/                               C++ source files
  third_party/                       SQLite vendor files
  .gitignore                         Git exclusions
  README.md                          Project documentation
  SalesMonitoringSystemNative.sln    Visual Studio solution
  SalesMonitoringSystemNative.vcxproj
  SalesMonitoringSystemNative.vcxproj.filters
  SalesMonitoringSystemNative.vcxproj.user
```

### Root Files

| File | Purpose |
|------|---------|
| `.gitignore` | Ignores build output, local database files, Visual Studio cache, and temporary files. |
| `README.md` | Main project guide. |
| `SalesMonitoringSystemNative.sln` | Visual Studio solution file. |
| `SalesMonitoringSystemNative.vcxproj` | Main C++ project configuration: sources, include folders, libraries, and build settings. |
| `SalesMonitoringSystemNative.vcxproj.filters` | Visual Studio file grouping in Solution Explorer. |
| `SalesMonitoringSystemNative.vcxproj.user` | Local Visual Studio user settings. |

### Database

| File | Purpose |
|------|---------|
| `database/schema.sql` | SQL schema for users, roles, products, clients, orders, sales, settings, tasks, pipeline, and audit-related entities. |

### Header Files (`include/`)

| File | Purpose |
|------|---------|
| `AccessControl.h` | Roles, permissions, page access checks, and role-specific navigation labels. |
| `AnalyticsPage.h` | Declaration of the analytics page. |
| `AppMessages.h` | Custom Windows messages used inside the app. |
| `ClientsPage.h` | Declaration of the clients page and client history UI. |
| `DashboardPage.h` | Declaration of the dashboard page with charts and monitoring tables. |
| `DataEntryDialog.h` | Reusable modal dialog for entering new records. |
| `DataModels.h` | Shared data structures used by pages and repository code. |
| `DataRepository.h` | Main data and business layer interface. |
| `EmployeesPage.h` | Declaration of the employees page. |
| `Localization.h` | Translation and localized text helpers. |
| `LoginWindow.h` | Declaration of the login and registration window. |
| `MainWindow.h` | Declaration of the application shell, sidebar, header, and pages. |
| `OrdersPage.h` | Declaration of the orders page. |
| `PageBase.h` | Base class for all pages. |
| `ProductsPage.h` | Declaration of the products page. |
| `ReportsPage.h` | Declaration of the reports and export page. |
| `SalesPage.h` | Declaration of the sales page. |
| `SettingsPage.h` | Declaration of the settings and account management page. |
| `SqliteDatabase.h` | Low-level SQLite wrapper. |
| `Theme.h` | Theme colors, fonts, DPI scaling, and style tokens. |
| `UiHelpers.h` | Shared UI drawing and layout helpers. |

### Source Files (`src/`)

| File | Purpose |
|------|---------|
| `main.cpp` | Application entry point. |
| `AccessControl.cpp` | Implements roles, permissions, and page visibility logic. |
| `AnalyticsPage.cpp` | Draws analytical charts and sales funnel visualizations. |
| `ClientsPage.cpp` | Implements client filters, client table, detail panel, and history table. |
| `DashboardPage.cpp` | Implements KPI cards, charts, and recent activity tables. |
| `DataEntryDialog.cpp` | Implements the reusable data entry dialog. |
| `DataRepository.cpp` | Core repository implementation: schema setup, demo seed, auth, CRUD, exports, and settings persistence. |
| `EmployeesPage.cpp` | Implements employee KPI, chart, and table logic. |
| `Localization.cpp` | Translation dictionaries and localized text lookup. |
| `LoginWindow.cpp` | Login and registration UI and authentication flow. |
| `MainWindow.cpp` | Main shell window, sidebar, page switching, layout, and header clock. |
| `OrdersPage.cpp` | Orders filtering, quick creation, status change, and delete actions. |
| `PageBase.cpp` | Shared page window behavior for all app pages. |
| `ProductsPage.cpp` | Product filters, product table, product CRUD, and CSV import. |
| `ReportsPage.cpp` | Reporting center, expiration watchlist, and export actions. |
| `SalesPage.cpp` | Sales filters, sales table, export, status updates, and delete actions. |
| `SettingsPage.cpp` | Theme, language, settings, database info, and account management. |
| `SqliteDatabase.cpp` | Low-level SQLite helper implementation. |
| `Theme.cpp` | Theme palette, font creation, and DPI scaling logic. |
| `UiHelpers.cpp` | Owner-drawn buttons, charts, rounded panels, list view helpers, and layout utilities. |

### Third-Party Files (`third_party/`)

| File | Purpose |
|------|---------|
| `third_party/sqlite-amalgamation-3510300.zip` | Original SQLite amalgamation archive. |
| `third_party/sqlite/sqlite3.c` | SQLite engine source code compiled into the app. |
| `third_party/sqlite/sqlite3.h` | Main SQLite header file. |
| `third_party/sqlite/sqlite3ext.h` | SQLite extension API header. |
| `third_party/sqlite/shell.c` | SQLite shell source from the vendor package. |

### Architecture Summary

#### UI Layer

- `MainWindow` hosts the full application shell
- each page inherits from `PageBase`
- custom controls and drawing are centralized in `UiHelpers`
- colors, fonts, and DPI behavior are centralized in `Theme`

#### Data Layer

- `DataRepository` is the main business and data access class
- `SqliteDatabase` is the low-level SQLite helper
- `DataModels` contains shared records used by pages and repository methods

#### Security And Access

- `AccessControl` defines roles and permissions
- `LoginWindow` handles login and registration
- permissions are checked in both UI visibility and repository actions

### Notes

- The project is a native desktop app, not a web app.
- The app uses WinAPI, GDI drawing, custom owner-drawn controls, and SQLite.
- Build artifacts and local databases are excluded from Git by `.gitignore`.
- SQLite vendor sources are stored inside `third_party/` so the project can build without downloading dependencies.

---

## Русская Версия

SalesFlow — это нативное Windows-приложение, написанное на чистом C++ с использованием WinAPI и SQLite.
Проект представляет собой систему мониторинга продаж с демо-данными, ролевым доступом, аналитическими страницами, CRUD-операциями и локальным экспортом.

### Что Делает Приложение

- показывает dashboard с метриками по выручке, заказам, товарам и клиентам
- содержит страницы продаж, заказов, товаров, клиентов, сотрудников, аналитики, отчётов и настроек
- хранит рабочие и демо-данные в SQLite
- поддерживает вход и регистрацию
- поддерживает роли: администратор, директор, руководитель отдела продаж, менеджер по продажам и менеджер по товарам
- экспортирует отчёты и CSV-файлы в `bin/Release/exports/`

### Данные Для Входа

Можно использовать один из демо-аккаунтов:

- логин: `a.petrova`
- пароль: `demo123`

Для остальных стандартных аккаунтов пароль тоже `demo123`.

### Сборка И Запуск

#### Через Visual Studio

1. Откройте [SalesMonitoringSystemNative.sln](./SalesMonitoringSystemNative.sln)
2. Выберите конфигурацию `Release | x64`
3. Соберите решение
4. Запустите приложение

#### Исполняемый Файл

После сборки приложение обычно находится здесь:

- `bin/Release/SalesFlow.exe`

### Структура Проекта

```text
SalesMonitoringSystemNative/
  database/                          SQL-схема базы данных
  include/                           Заголовочные файлы
  src/                               Исходные C++ файлы
  third_party/                       Сторонние файлы SQLite
  .gitignore                         Исключения для Git
  README.md                          Документация проекта
  SalesMonitoringSystemNative.sln    Решение Visual Studio
  SalesMonitoringSystemNative.vcxproj
  SalesMonitoringSystemNative.vcxproj.filters
  SalesMonitoringSystemNative.vcxproj.user
```

### Файлы В Корне Проекта

| Файл | Назначение |
|------|------------|
| `.gitignore` | Исключает из Git результаты сборки, локальные базы данных, кэш Visual Studio и временные файлы. |
| `README.md` | Основное описание проекта. |
| `SalesMonitoringSystemNative.sln` | Файл решения Visual Studio. |
| `SalesMonitoringSystemNative.vcxproj` | Основная конфигурация проекта C++: исходники, include-пути, библиотеки и параметры сборки. |
| `SalesMonitoringSystemNative.vcxproj.filters` | Группировка файлов в Solution Explorer Visual Studio. |
| `SalesMonitoringSystemNative.vcxproj.user` | Локальные пользовательские настройки Visual Studio. |

### База Данных

| Файл | Назначение |
|------|------------|
| `database/schema.sql` | SQL-схема для пользователей, ролей, товаров, клиентов, заказов, продаж, настроек, задач, воронки и связанных сущностей. |

### Заголовочные Файлы (`include/`)

| Файл | Назначение |
|------|------------|
| `AccessControl.h` | Роли, права доступа, проверки открытия страниц и названия пунктов меню по ролям. |
| `AnalyticsPage.h` | Объявление страницы аналитики. |
| `AppMessages.h` | Пользовательские Windows-сообщения внутри приложения. |
| `ClientsPage.h` | Объявление страницы клиентов и истории заказов клиента. |
| `DashboardPage.h` | Объявление dashboard со сводкой, графиками и таблицами мониторинга. |
| `DataEntryDialog.h` | Переиспользуемое модальное окно для ввода новых данных. |
| `DataModels.h` | Общие структуры данных, используемые страницами и репозиторием. |
| `DataRepository.h` | Главный интерфейс слоя данных и бизнес-логики. |
| `EmployeesPage.h` | Объявление страницы сотрудников. |
| `Localization.h` | Помощники для перевода и локализованного текста. |
| `LoginWindow.h` | Объявление окна входа и регистрации. |
| `MainWindow.h` | Объявление главного окна приложения, меню, заголовка и страниц. |
| `OrdersPage.h` | Объявление страницы заказов. |
| `PageBase.h` | Базовый класс для всех страниц приложения. |
| `ProductsPage.h` | Объявление страницы товаров. |
| `ReportsPage.h` | Объявление страницы отчётов и экспорта. |
| `SalesPage.h` | Объявление страницы продаж. |
| `SettingsPage.h` | Объявление страницы настроек и управления аккаунтами. |
| `SqliteDatabase.h` | Низкоуровневая обёртка над SQLite. |
| `Theme.h` | Цвета темы, шрифты, DPI-масштабирование и визуальные токены. |
| `UiHelpers.h` | Общие помощники для отрисовки интерфейса и расчёта layout. |

### Исходные Файлы (`src/`)

| Файл | Назначение |
|------|------------|
| `main.cpp` | Точка входа в приложение. |
| `AccessControl.cpp` | Реализация ролей, прав и логики видимости страниц. |
| `AnalyticsPage.cpp` | Отрисовка аналитических графиков и воронки продаж. |
| `ClientsPage.cpp` | Реализация фильтров клиентов, таблицы клиентов, карточки клиента и истории заказов. |
| `DashboardPage.cpp` | Реализация KPI-карточек, графиков и таблиц последних операций. |
| `DataEntryDialog.cpp` | Реализация универсального окна ввода данных. |
| `DataRepository.cpp` | Основной код репозитория: схема БД, начальные данные, аутентификация, CRUD, экспорт и настройки. |
| `EmployeesPage.cpp` | Реализация KPI, графика и таблицы по сотрудникам. |
| `Localization.cpp` | Словари переводов и поиск локализованных строк. |
| `LoginWindow.cpp` | Интерфейс входа, регистрации и логика аутентификации. |
| `MainWindow.cpp` | Главное окно приложения, боковое меню, переключение страниц, layout и часы в заголовке. |
| `OrdersPage.cpp` | Фильтрация заказов, создание быстрого заказа, смена статуса и удаление. |
| `PageBase.cpp` | Общая оконная логика для всех страниц приложения. |
| `ProductsPage.cpp` | Фильтры товаров, таблица товаров, CRUD по товарам и импорт CSV. |
| `ReportsPage.cpp` | Центр отчётов, контроль сроков годности и экспорт. |
| `SalesPage.cpp` | Фильтры продаж, таблица продаж, экспорт, смена статуса и удаление. |
| `SettingsPage.cpp` | Темы, язык, настройки, информация о базе данных и управление учётными записями. |
| `SqliteDatabase.cpp` | Реализация низкоуровневого помощника SQLite. |
| `Theme.cpp` | Палитры темы, создание шрифтов и DPI-масштабирование. |
| `UiHelpers.cpp` | Owner-drawn кнопки, графики, панели, list view helpers и layout-утилиты. |

### Сторонние Файлы (`third_party/`)

| Файл | Назначение |
|------|------------|
| `third_party/sqlite-amalgamation-3510300.zip` | Исходный архив SQLite amalgamation. |
| `third_party/sqlite/sqlite3.c` | Исходный код движка SQLite, который компилируется в приложение. |
| `third_party/sqlite/sqlite3.h` | Основной заголовочный файл SQLite. |
| `third_party/sqlite/sqlite3ext.h` | Заголовок API расширений SQLite. |
| `third_party/sqlite/shell.c` | Исходник SQLite shell из vendor-пакета. |

### Кратко Об Архитектуре

#### UI-Слой

- `MainWindow` содержит общий каркас приложения
- каждая страница наследуется от `PageBase`
- общая отрисовка и вспомогательные UI-функции находятся в `UiHelpers`
- цвета, шрифты и DPI-логика находятся в `Theme`

#### Слой Данных

- `DataRepository` — основной класс доступа к данным и бизнес-логике
- `SqliteDatabase` — низкоуровневая обёртка над SQLite
- `DataModels` содержит структуры данных, которыми пользуются страницы и репозиторий

#### Безопасность И Доступ

- `AccessControl` определяет роли и права
- `LoginWindow` отвечает за вход и регистрацию
- права проверяются и на уровне интерфейса, и на уровне вызовов репозитория

### Примечания

- Это нативное desktop-приложение, а не web-приложение.
- Приложение использует WinAPI, GDI-отрисовку, owner-drawn controls и SQLite.
- Результаты сборки и локальные базы данных исключены из Git через `.gitignore`.
- Исходники SQLite лежат внутри `third_party/`, чтобы проект собирался без отдельной установки зависимостей.
