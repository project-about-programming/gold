# SalesFlow

SalesFlow is a native Windows desktop application written in pure C++ with WinAPI and SQLite.
The project is a frontend-first sales monitoring system with demo data, role-based access, analytics pages, CRUD actions, and local export features.

## What The Application Does

- shows a dashboard with revenue, orders, products, and client metrics
- provides pages for sales, orders, products, clients, employees, analytics, reports, and settings
- stores demo and working data in SQLite
- supports login and registration
- supports role-based access for administrator, director, sales manager, head of sales, and product manager
- exports reports and CSV files to the local `bin/Release/exports/` folder

## Default Login

Use one of the demo accounts below:

- username: `a.petrova`
- password: `demo123`

Other seeded users also use `demo123`.

## Build And Run

### Visual Studio

1. Open [SalesMonitoringSystemNative.sln](./SalesMonitoringSystemNative.sln)
2. Select `Release | x64`
3. Build the solution
4. Run the application

### Output File

The compiled executable is usually created as:

- `bin/Release/SalesFlow.exe`

## Project Structure

```text
SalesMonitoringSystemNative/
├── database/        SQLite schema
├── include/         Header files
├── src/             C++ implementation files
├── third_party/     SQLite source files
├── .gitignore       Git exclusions
├── README.md        Project documentation
├── SalesMonitoringSystemNative.sln
├── SalesMonitoringSystemNative.vcxproj
├── SalesMonitoringSystemNative.vcxproj.filters
└── SalesMonitoringSystemNative.vcxproj.user
```

## Root Files

| File | Purpose |
|------|---------|
| `.gitignore` | Ignores build output, local database files, Visual Studio cache, and temporary files. |
| `README.md` | Main project guide. |
| `SalesMonitoringSystemNative.sln` | Visual Studio solution file. Open this file to work with the full project. |
| `SalesMonitoringSystemNative.vcxproj` | Main C++ project configuration: sources, include folders, libraries, build settings. |
| `SalesMonitoringSystemNative.vcxproj.filters` | Visual Studio UI grouping for files in Solution Explorer. |
| `SalesMonitoringSystemNative.vcxproj.user` | Local Visual Studio user settings for this project. |

## Database

| File | Purpose |
|------|---------|
| `database/schema.sql` | SQL schema for users, roles, products, clients, orders, sales, settings, tasks, pipeline, and audit-related tables. |

## Header Files (`include/`)

| File | Purpose |
|------|---------|
| `AccessControl.h` | Role model, permissions, page access checks, startup page logic, and role-specific navigation labels. |
| `AnalyticsPage.h` | Declaration of the analytics page. |
| `AppMessages.h` | Application-level custom Windows messages such as settings refresh notifications. |
| `ClientsPage.h` | Declaration of the clients page and client history UI. |
| `DashboardPage.h` | Declaration of the dashboard page with charts and monitoring tables. |
| `DataEntryDialog.h` | Reusable modal dialog for entering new records from add buttons. |
| `DataModels.h` | Shared data structures used by pages and repository code. |
| `DataRepository.h` | Main data/business layer interface. Handles SQLite access, CRUD, authentication, exports, settings, and seeded data. |
| `EmployeesPage.h` | Declaration of the employees performance page. |
| `Localization.h` | Translation and localized text access helpers. |
| `LoginWindow.h` | Declaration of the login and registration window. |
| `MainWindow.h` | Declaration of the main shell window, sidebar, header, pages, and status bar. |
| `OrdersPage.h` | Declaration of the orders page. |
| `PageBase.h` | Base class for all pages. Unifies create, resize, paint, command, and notify handling. |
| `ProductsPage.h` | Declaration of the products page. |
| `ReportsPage.h` | Declaration of the reports page and export center. |
| `SalesPage.h` | Declaration of the sales page. |
| `SettingsPage.h` | Declaration of the settings page and account management page. |
| `SqliteDatabase.h` | Low-level SQLite wrapper: open database, execute SQL, prepare statements, scalar queries. |
| `Theme.h` | Theme colors, fonts, DPI scaling, and style tokens. |
| `UiHelpers.h` | Shared UI drawing and layout helpers: buttons, edits, combos, list views, cards, charts, and rectangles. |

## Source Files (`src/`)

| File | Purpose |
|------|---------|
| `main.cpp` | Application entry point. Initializes common controls, theme, login window, and main window. |
| `AccessControl.cpp` | Implements roles, permission checks, page visibility, and role-specific page titles. |
| `AnalyticsPage.cpp` | Draws analytical charts and sales funnel visualizations. |
| `ClientsPage.cpp` | Implements the clients page, list/detail layout, filters, client CRUD, and purchase history. |
| `DashboardPage.cpp` | Implements the dashboard, KPI cards, charts, and recent activity tables. |
| `DataEntryDialog.cpp` | Implements the reusable modal data entry dialog. |
| `DataRepository.cpp` | Core repository implementation. Contains SQL queries, schema setup, demo seeding, authentication, CRUD logic, exports, and settings persistence. |
| `EmployeesPage.cpp` | Implements employee performance cards, charts, and the employees table. |
| `Localization.cpp` | Contains translation dictionaries and text lookup logic. |
| `LoginWindow.cpp` | Implements login, registration mode switching, credential validation, and login UI layout. |
| `MainWindow.cpp` | Implements the application shell: sidebar, page switching, header clock, layout management, and settings refresh flow. |
| `OrdersPage.cpp` | Implements orders filtering, order table, quick order creation, status changes, and delete actions. |
| `PageBase.cpp` | Implements the shared page message loop behavior used by all pages. |
| `ProductsPage.cpp` | Implements product filtering, product table, product CRUD, and CSV import. |
| `ReportsPage.cpp` | Implements the reporting center, expiration watchlist, and export actions. |
| `SalesPage.cpp` | Implements sales filters, sales table, export, status updates, and delete actions. |
| `SettingsPage.cpp` | Implements system settings, theme/language controls, database info, and user account management. |
| `SqliteDatabase.cpp` | Implements the low-level SQLite helper class declared in `SqliteDatabase.h`. |
| `Theme.cpp` | Implements theme color palettes, DPI scaling, and font creation. |
| `UiHelpers.cpp` | Implements owner-drawn buttons, charts, rounded panels, list view helpers, and rectangle/layout utilities. |

## Third-Party Files (`third_party/`)

| File | Purpose |
|------|---------|
| `third_party/sqlite-amalgamation-3510300.zip` | Original SQLite amalgamation archive kept with the project. |
| `third_party/sqlite/sqlite3.c` | SQLite engine source code compiled into the application. |
| `third_party/sqlite/sqlite3.h` | Main SQLite header file. |
| `third_party/sqlite/sqlite3ext.h` | SQLite extension API header. |
| `third_party/sqlite/shell.c` | SQLite shell source from the amalgamation package. Not required for the app runtime, but included with the vendor files. |

## Architecture Summary

### UI Layer

- `MainWindow` hosts the full application shell
- each page inherits from `PageBase`
- custom controls and drawing are centralized in `UiHelpers`
- colors, fonts, and DPI behavior are centralized in `Theme`

### Data Layer

- `DataRepository` is the main business/data access class
- `SqliteDatabase` is the low-level SQLite helper
- `DataModels` contains all DTO-style records used by pages and repository methods

### Security And Access

- `AccessControl` defines roles and permissions
- login is handled by `LoginWindow`
- permissions are checked both in UI visibility and in repository actions

## Notes

- The project is a native desktop app, not a web app.
- The app uses WinAPI, GDI drawing, custom owner-drawn controls, and SQLite.
- Build artifacts and local databases are excluded from Git by `.gitignore`.
- The repository currently keeps the SQLite vendor sources inside `third_party/` so the project can build without downloading dependencies.
