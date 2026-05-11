#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <sqlite3.h>

#include <string>
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vcclr.h>

#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>

using namespace System;
using namespace System::Drawing;
using namespace System::Text;
using namespace System::Windows::Forms;
using namespace System::Runtime::InteropServices;

namespace SalesFlowCLR {

static std::string ToUtf8(String^ value) {
    if (String::IsNullOrEmpty(value)) {
        return std::string();
    }
    cli::array<Byte>^ bytes = Encoding::UTF8->GetBytes(value);
    if (bytes->Length == 0) {
        return std::string();
    }
    pin_ptr<Byte> pinned = &bytes[0];
    return std::string(reinterpret_cast<char*>(pinned), bytes->Length);
}

static String^ FromUtf8(const unsigned char* text) {
    if (text == nullptr) {
        return String::Empty;
    }
    const char* raw = reinterpret_cast<const char*>(text);
    const int length = static_cast<int>(std::strlen(raw));
    if (length == 0) {
        return String::Empty;
    }
    cli::array<Byte>^ bytes = gcnew cli::array<Byte>(length);
    Marshal::Copy(IntPtr(const_cast<char*>(raw)), bytes, 0, length);
    return Encoding::UTF8->GetString(bytes);
}

static String^ TodayText() {
    return DateTime::Now.ToString("yyyy-MM-dd HH:mm");
}

public ref class ComboItem {
public:
    int Id;
    String^ Text;

    ComboItem(int id, String^ text) {
        Id = id;
        Text = text;
    }

    virtual String^ ToString() override {
        return Text;
    }
};

public ref class MainForm : public Form {
public:
    MainForm() {
        InitializeStyle();
        OpenDatabase();
        BuildShell();
        ShowDashboard();
    }

protected:
    ~MainForm() {
        if (db_ != nullptr) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

private:
    sqlite3* db_ = nullptr;

    TableLayoutPanel^ root_;
    TableLayoutPanel^ mainLayout_;
    Panel^ sidebar_;
    Panel^ header_;
    Panel^ content_;
    Label^ titleLabel_;
    Label^ subtitleLabel_;
    Label^ appLabel_;
    Label^ appSubtitle_;
    Panel^ logoPanel_;

    Button^ navDashboard_;
    Button^ navProducts_;
    Button^ navClients_;
    Button^ navSales_;
    Button^ navSettings_;

    DataGridView^ dashboardGrid_;
    DataGridView^ productsGrid_;
    DataGridView^ clientsGrid_;
    DataGridView^ salesGrid_;

    Label^ revenueCard_;
    Label^ productsCard_;
    Label^ clientsCard_;
    Label^ salesCard_;
    Label^ lowStockLabel_;

    TextBox^ productSearch_;
    ComboBox^ productCategoryFilter_;
    ComboBox^ productStockFilter_;
    TextBox^ productSku_;
    TextBox^ productName_;
    TextBox^ productCategory_;
    TextBox^ productStock_;
    TextBox^ productPrice_;

    TextBox^ clientSearch_;
    TextBox^ clientCompany_;
    TextBox^ clientContact_;
    TextBox^ clientPhone_;
    TextBox^ clientEmail_;

    TextBox^ saleSearch_;
    ComboBox^ saleStatusFilter_;
    ComboBox^ saleClientFilter_;
    ComboBox^ saleClient_;
    ComboBox^ saleProduct_;
    TextBox^ saleQuantity_;
    TextBox^ salePrice_;
    ComboBox^ saleStatus_;

    int selectedProductId_ = 0;
    int selectedClientId_ = 0;
    int selectedSaleId_ = 0;

    Color dark_ = Color::FromArgb(31, 41, 55);
    Color dark2_ = Color::FromArgb(17, 24, 39);
    Color background_ = Color::FromArgb(245, 247, 251);
    Color card_ = Color::White;
    Color accent_ = Color::FromArgb(37, 99, 235);
    Color text_ = Color::FromArgb(31, 41, 55);
    Color muted_ = Color::FromArgb(100, 116, 139);

    void InitializeStyle() {
        Text = "SalesFlow CLR - мониторинг продаж";
        Width = 1360;
        Height = 820;
        MinimumSize = Drawing::Size(1180, 720);
        StartPosition = FormStartPosition::CenterScreen;
        Font = gcnew Drawing::Font("Segoe UI", 10.0f, FontStyle::Regular);
        BackColor = background_;
    }

    void OpenDatabase() {
        String^ path = Application::StartupPath + "\\salesflow_clr.db";
        std::string dbPath = ToUtf8(path);
        sqlite3* rawDb = nullptr;
        if (sqlite3_open(dbPath.c_str(), &rawDb) != SQLITE_OK) {
            if (rawDb != nullptr) {
                sqlite3_close(rawDb);
            }
            MessageBox::Show("Не удалось открыть базу данных.", "Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Error);
            return;
        }
        db_ = rawDb;
        Exec("PRAGMA foreign_keys = ON;");
        EnsureSchema();
        SeedIfEmpty();
    }

    bool Exec(const char* sql) {
        char* error = nullptr;
        const int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &error);
        if (rc != SQLITE_OK) {
            String^ msg = error != nullptr ? gcnew String(error) : "Ошибка SQLite";
            if (error != nullptr) {
                sqlite3_free(error);
            }
            MessageBox::Show(msg, "Ошибка базы данных", MessageBoxButtons::OK, MessageBoxIcon::Error);
            return false;
        }
        return true;
    }

    void EnsureSchema() {
        Exec(
            "CREATE TABLE IF NOT EXISTS products ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "sku TEXT NOT NULL UNIQUE,"
            "name TEXT NOT NULL,"
            "category TEXT NOT NULL,"
            "stock INTEGER NOT NULL DEFAULT 0,"
            "price REAL NOT NULL DEFAULT 0);"
        );
        Exec(
            "CREATE TABLE IF NOT EXISTS clients ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "company TEXT NOT NULL,"
            "contact TEXT NOT NULL,"
            "phone TEXT NOT NULL,"
            "email TEXT NOT NULL,"
            "lifetime_value REAL NOT NULL DEFAULT 0);"
        );
        Exec(
            "CREATE TABLE IF NOT EXISTS sales ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "sale_date TEXT NOT NULL,"
            "client_id INTEGER NOT NULL,"
            "product_id INTEGER NOT NULL,"
            "quantity INTEGER NOT NULL,"
            "unit_price REAL NOT NULL,"
            "total REAL NOT NULL,"
            "status TEXT NOT NULL,"
            "FOREIGN KEY (client_id) REFERENCES clients(id),"
            "FOREIGN KEY (product_id) REFERENCES products(id));"
        );
    }

    int ScalarInt(const char* sql) {
        sqlite3_stmt* stmt = nullptr;
        int value = 0;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                value = sqlite3_column_int(stmt, 0);
            }
        }
        sqlite3_finalize(stmt);
        return value;
    }

    double ScalarDouble(const char* sql) {
        sqlite3_stmt* stmt = nullptr;
        double value = 0.0;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                value = sqlite3_column_double(stmt, 0);
            }
        }
        sqlite3_finalize(stmt);
        return value;
    }

    void SeedIfEmpty() {
        if (ScalarInt("SELECT COUNT(*) FROM products;") > 0) {
            return;
        }
        ResetDemoData(false);
    }

    void ResetDemoData(bool notify) {
        Exec("DELETE FROM sales;");
        Exec("DELETE FROM products;");
        Exec("DELETE FROM clients;");
        Exec("DELETE FROM sqlite_sequence WHERE name IN ('products','clients','sales');");
        Exec("INSERT INTO products(sku,name,category,stock,price) VALUES"
             "('P-100','Ноутбук Lenovo','Техника',10,650.00),"
             "('P-200','Клавиатура Logitech','Аксессуары',25,45.00),"
             "('P-300','Монитор Samsung','Техника',7,210.00),"
             "('P-400','Мышь A4Tech','Аксессуары',40,18.00);");
        Exec("INSERT INTO clients(company,contact,phone,email,lifetime_value) VALUES"
             "('Tech Market','Али Каримов','+998 90 111 22 33','ali@example.com',0),"
             "('Office Plus','Дилноза Рахимова','+998 91 222 33 44','office@example.com',0),"
             "('Smart Group','Сардор Нурматов','+998 93 333 44 55','smart@example.com',0);");
        Exec("INSERT INTO sales(sale_date,client_id,product_id,quantity,unit_price,total,status) VALUES"
             "('2026-05-11 09:00',1,1,2,650.00,1300.00,'Оплачено'),"
             "('2026-05-11 09:10',2,2,5,45.00,225.00,'Оплачено'),"
             "('2026-05-11 09:20',3,3,1,210.00,210.00,'Ожидает');");
        Exec("UPDATE products SET stock = stock - 2 WHERE id = 1;");
        Exec("UPDATE products SET stock = stock - 5 WHERE id = 2;");
        Exec("UPDATE products SET stock = stock - 1 WHERE id = 3;");
        RecalculateClients();
        if (notify) {
            MessageBox::Show("Демо-данные восстановлены.", "Готово", MessageBoxButtons::OK, MessageBoxIcon::Information);
            RefreshCurrentPage();
        }
    }

    void RecalculateClients() {
        Exec("UPDATE clients SET lifetime_value = COALESCE((SELECT SUM(total) FROM sales WHERE sales.client_id = clients.id AND status <> 'Отменено'),0);");
    }

    void BuildShell() {
        root_ = gcnew TableLayoutPanel();
        root_->Dock = DockStyle::Fill;
        root_->ColumnCount = 2;
        root_->RowCount = 1;
        root_->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 220.0f));
        root_->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));
        root_->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));
        root_->Margin = System::Windows::Forms::Padding(0);
        root_->Padding = System::Windows::Forms::Padding(0);
        Controls->Add(root_);

        sidebar_ = gcnew Panel();
        sidebar_->Dock = DockStyle::Fill;
        sidebar_->BackColor = dark_;
        sidebar_->Margin = System::Windows::Forms::Padding(0);
        root_->Controls->Add(sidebar_, 0, 0);

        mainLayout_ = gcnew TableLayoutPanel();
        mainLayout_->Dock = DockStyle::Fill;
        mainLayout_->ColumnCount = 1;
        mainLayout_->RowCount = 2;
        mainLayout_->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));
        mainLayout_->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 92.0f));
        mainLayout_->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));
        mainLayout_->Margin = System::Windows::Forms::Padding(0);
        mainLayout_->Padding = System::Windows::Forms::Padding(0);
        root_->Controls->Add(mainLayout_, 1, 0);

        logoPanel_ = gcnew Panel();
        logoPanel_->Left = 18;
        logoPanel_->Top = 18;
        logoPanel_->Width = 70;
        logoPanel_->Height = 54;
        logoPanel_->BackColor = dark_;
        logoPanel_->Paint += gcnew PaintEventHandler(this, &MainForm::OnLogoPaint);
        sidebar_->Controls->Add(logoPanel_);

        appLabel_ = gcnew Label();
        appLabel_->Text = "SalesFlow";
        appLabel_->ForeColor = Color::White;
        appLabel_->Font = gcnew Drawing::Font("Segoe UI", 18.0f, FontStyle::Bold);
        appLabel_->Left = 88;
        appLabel_->Top = 20;
        appLabel_->Width = 124;
        appLabel_->Height = 30;
        sidebar_->Controls->Add(appLabel_);

        appSubtitle_ = gcnew Label();
        appSubtitle_->Text = "учебный проект";
        appSubtitle_->ForeColor = Color::FromArgb(203, 213, 225);
        appSubtitle_->Left = 90;
        appSubtitle_->Top = 52;
        appSubtitle_->Width = 120;
        appSubtitle_->Height = 24;
        sidebar_->Controls->Add(appSubtitle_);

        navDashboard_ = CreateNavButton("Главная", 112);
        navProducts_ = CreateNavButton("Товары", 160);
        navClients_ = CreateNavButton("Клиенты", 208);
        navSales_ = CreateNavButton("Продажи", 256);
        navSettings_ = CreateNavButton("Настройки", 304);
        navDashboard_->Click += gcnew EventHandler(this, &MainForm::NavDashboardClick);
        navProducts_->Click += gcnew EventHandler(this, &MainForm::NavProductsClick);
        navClients_->Click += gcnew EventHandler(this, &MainForm::NavClientsClick);
        navSales_->Click += gcnew EventHandler(this, &MainForm::NavSalesClick);
        navSettings_->Click += gcnew EventHandler(this, &MainForm::NavSettingsClick);

        header_ = gcnew Panel();
        header_->Dock = DockStyle::Fill;
        header_->BackColor = Color::White;
        header_->Margin = System::Windows::Forms::Padding(0);
        mainLayout_->Controls->Add(header_, 0, 0);

        titleLabel_ = gcnew Label();
        titleLabel_->Left = 28;
        titleLabel_->Top = 20;
        titleLabel_->Width = 700;
        titleLabel_->Height = 32;
        titleLabel_->Font = gcnew Drawing::Font("Segoe UI", 18.0f, FontStyle::Bold);
        titleLabel_->ForeColor = text_;
        header_->Controls->Add(titleLabel_);

        subtitleLabel_ = gcnew Label();
        subtitleLabel_->Left = 30;
        subtitleLabel_->Top = 56;
        subtitleLabel_->Width = 900;
        subtitleLabel_->Height = 24;
        subtitleLabel_->ForeColor = muted_;
        header_->Controls->Add(subtitleLabel_);

        content_ = gcnew Panel();
        content_->Dock = DockStyle::Fill;
        content_->Padding = System::Windows::Forms::Padding(18);
        content_->BackColor = background_;
        content_->Margin = System::Windows::Forms::Padding(0);
        mainLayout_->Controls->Add(content_, 0, 1);
    }

    Button^ CreateNavButton(String^ text, int top) {
        Button^ button = gcnew Button();
        button->Text = text;
        button->Left = 18;
        button->Top = top;
        button->Width = 184;
        button->Height = 38;
        button->FlatStyle = FlatStyle::Flat;
        button->FlatAppearance->BorderSize = 0;
        button->BackColor = Color::FromArgb(243, 244, 246);
        button->ForeColor = Color::FromArgb(31, 41, 55);
        button->Font = gcnew Drawing::Font("Segoe UI", 10.5f, FontStyle::Regular);
        button->Cursor = Cursors::Hand;
        sidebar_->Controls->Add(button);
        return button;
    }

    void SetActive(Button^ active) {
        cli::array<Button^>^ buttons = gcnew cli::array<Button^>{ navDashboard_, navProducts_, navClients_, navSales_, navSettings_ };
        for each (Button^ button in buttons) {
            if (button == nullptr) continue;
            button->BackColor = button == active ? Color::FromArgb(219, 234, 254) : Color::FromArgb(243, 244, 246);
            button->ForeColor = button == active ? Color::FromArgb(30, 64, 175) : Color::FromArgb(31, 41, 55);
            button->Font = gcnew Drawing::Font("Segoe UI", 10.5f, button == active ? FontStyle::Bold : FontStyle::Regular);
        }
    }

    void OnLogoPaint(Object^, PaintEventArgs^ e) {
        Graphics^ g = e->Graphics;
        g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;
        Pen^ pen = gcnew Pen(Color::White, 3.0f);
        g->DrawLine(pen, 6, 34, 24, 34);
        g->DrawLine(pen, 24, 34, 35, 10);
        g->DrawLine(pen, 35, 10, 46, 34);
        g->DrawLine(pen, 46, 34, 64, 34);
        g->DrawLine(pen, 12, 24, 28, 24);
        g->DrawLine(pen, 42, 24, 58, 24);
        g->DrawLine(pen, 18, 14, 32, 14);
        g->DrawLine(pen, 38, 14, 52, 14);
        g->DrawLine(pen, 26, 42, 35, 20);
        g->DrawLine(pen, 35, 20, 44, 42);
    }

    Panel^ CreatePage() {
        content_->Controls->Clear();
        Panel^ page = gcnew Panel();
        page->Dock = DockStyle::Fill;
        page->BackColor = background_;
        page->Margin = System::Windows::Forms::Padding(0);
        content_->Controls->Add(page);
        return page;
    }

    TableLayoutPanel^ CreateThreeRowPage(int topHeight, int bottomHeight) {
        content_->Controls->Clear();
        TableLayoutPanel^ page = gcnew TableLayoutPanel();
        page->Dock = DockStyle::Fill;
        page->BackColor = background_;
        page->ColumnCount = 1;
        page->RowCount = 3;
        page->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));
        page->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, static_cast<float>(topHeight)));
        page->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));
        page->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, static_cast<float>(bottomHeight)));
        page->Margin = System::Windows::Forms::Padding(0);
        page->Padding = System::Windows::Forms::Padding(0);
        content_->Controls->Add(page);
        return page;
    }

    TableLayoutPanel^ CreateSettingsLayout() {
        content_->Controls->Clear();
        TableLayoutPanel^ page = gcnew TableLayoutPanel();
        page->Dock = DockStyle::Fill;
        page->BackColor = background_;
        page->ColumnCount = 1;
        page->RowCount = 3;
        page->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));
        page->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 230.0f));
        page->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 100.0f));
        page->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));
        page->Margin = System::Windows::Forms::Padding(0);
        page->Padding = System::Windows::Forms::Padding(0);
        content_->Controls->Add(page);
        return page;
    }

    Panel^ CreateCardPanel(DockStyle dock, int height) {
        Panel^ panel = gcnew Panel();
        panel->Dock = dock;
        panel->Height = height;
        panel->BackColor = card_;
        panel->Padding = System::Windows::Forms::Padding(14);
        panel->Margin = System::Windows::Forms::Padding(0, 0, 0, 10);
        panel->BorderStyle = BorderStyle::FixedSingle;
        return panel;
    }

    Label^ CreateSectionLabel(String^ text, int x, int y, int w, Control^ parent) {
        Label^ label = gcnew Label();
        label->Text = text;
        label->Left = x;
        label->Top = y;
        label->Width = w;
        label->Height = 22;
        label->Font = gcnew Drawing::Font("Segoe UI", 10.0f, FontStyle::Bold);
        label->ForeColor = text_;
        parent->Controls->Add(label);
        return label;
    }

    TextBox^ AddTextBox(Control^ parent, String^ label, int x, int y, int w) {
        CreateSectionLabel(label, x, y, w, parent);
        TextBox^ box = gcnew TextBox();
        box->Left = x;
        box->Top = y + 24;
        box->Width = w;
        box->Height = 26;
        parent->Controls->Add(box);
        return box;
    }

    ComboBox^ AddCombo(Control^ parent, String^ label, int x, int y, int w) {
        CreateSectionLabel(label, x, y, w, parent);
        ComboBox^ combo = gcnew ComboBox();
        combo->DropDownStyle = ComboBoxStyle::DropDownList;
        combo->Left = x;
        combo->Top = y + 24;
        combo->Width = w;
        combo->Height = 28;
        parent->Controls->Add(combo);
        return combo;
    }

    Button^ AddButton(Control^ parent, String^ text, int x, int y, int w) {
        Button^ button = gcnew Button();
        button->Text = text;
        button->Left = x;
        button->Top = y;
        button->Width = w;
        button->Height = 34;
        button->FlatStyle = FlatStyle::Flat;
        button->FlatAppearance->BorderColor = Color::FromArgb(148, 163, 184);
        button->BackColor = Color::FromArgb(248, 250, 252);
        button->ForeColor = text_;
        button->Cursor = Cursors::Hand;
        parent->Controls->Add(button);
        return button;
    }

    DataGridView^ CreateGrid() {
        DataGridView^ grid = gcnew DataGridView();
        grid->Dock = DockStyle::Fill;
        grid->BackgroundColor = Color::White;
        grid->BorderStyle = BorderStyle::FixedSingle;
        grid->AllowUserToAddRows = false;
        grid->AllowUserToDeleteRows = false;
        grid->ReadOnly = true;
        grid->SelectionMode = DataGridViewSelectionMode::FullRowSelect;
        grid->MultiSelect = false;
        grid->RowHeadersVisible = false;
        grid->AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode::Fill;
        grid->ColumnHeadersHeight = 32;
        grid->EnableHeadersVisualStyles = false;
        grid->ColumnHeadersDefaultCellStyle->BackColor = Color::FromArgb(241, 245, 249);
        grid->ColumnHeadersDefaultCellStyle->ForeColor = text_;
        grid->ColumnHeadersDefaultCellStyle->Font = gcnew Drawing::Font("Segoe UI", 10.0f, FontStyle::Bold);
        grid->DefaultCellStyle->SelectionBackColor = Color::FromArgb(219, 234, 254);
        grid->DefaultCellStyle->SelectionForeColor = text_;
        grid->DefaultCellStyle->Padding = System::Windows::Forms::Padding(6, 2, 6, 2);
        grid->RowsDefaultCellStyle->BackColor = Color::White;
        grid->AlternatingRowsDefaultCellStyle->BackColor = Color::FromArgb(248, 250, 252);
        grid->GridColor = Color::FromArgb(226, 232, 240);
        grid->AllowUserToResizeRows = false;
        grid->RowTemplate->Height = 28;
        return grid;
    }

    void ConfigureColumn(DataGridView^ grid, String^ name, String^ title, float weight) {
        grid->Columns->Add(name, title);
        grid->Columns[name]->FillWeight = weight;
    }

    void NavDashboardClick(Object^, EventArgs^) { ShowDashboard(); }
    void NavProductsClick(Object^, EventArgs^) { ShowProducts(); }
    void NavClientsClick(Object^, EventArgs^) { ShowClients(); }
    void NavSalesClick(Object^, EventArgs^) { ShowSales(); }
    void NavSettingsClick(Object^, EventArgs^) { ShowSettings(); }

    void ShowDashboard() {
        SetActive(navDashboard_);
        titleLabel_->Text = "Главная";
        subtitleLabel_->Text = "Краткая статистика по продажам и остаткам.";
        TableLayoutPanel^ page = CreateThreeRowPage(76, 34);

        Panel^ cards = CreateCardPanel(DockStyle::Fill, 76);
        page->Controls->Add(cards, 0, 0);
        revenueCard_ = CreateDashboardCard(cards, "Выручка", 14, 14, 220);
        productsCard_ = CreateDashboardCard(cards, "Товары", 250, 14, 170);
        clientsCard_ = CreateDashboardCard(cards, "Клиенты", 436, 14, 170);
        salesCard_ = CreateDashboardCard(cards, "Продажи", 622, 14, 170);

        Panel^ bottom = gcnew Panel();
        bottom->Dock = DockStyle::Fill;
        bottom->BackColor = background_;
        bottom->Margin = System::Windows::Forms::Padding(0);
        page->Controls->Add(bottom, 0, 2);
        lowStockLabel_ = gcnew Label();
        lowStockLabel_->Dock = DockStyle::Fill;
        lowStockLabel_->ForeColor = muted_;
        bottom->Controls->Add(lowStockLabel_);

        dashboardGrid_ = CreateGrid();
        dashboardGrid_->Margin = System::Windows::Forms::Padding(0, 10, 0, 10);
        page->Controls->Add(dashboardGrid_, 0, 1);
        ConfigureColumn(dashboardGrid_, "date", "Дата", 130);
        ConfigureColumn(dashboardGrid_, "client", "Клиент", 160);
        ConfigureColumn(dashboardGrid_, "product", "Товар", 190);
        ConfigureColumn(dashboardGrid_, "qty", "Кол-во", 70);
        ConfigureColumn(dashboardGrid_, "total", "Сумма", 90);
        ConfigureColumn(dashboardGrid_, "status", "Статус", 90);
        LoadDashboard();
    }

    Label^ CreateDashboardCard(Control^ parent, String^ title, int x, int y, int w) {
        Label^ label = gcnew Label();
        label->Left = x;
        label->Top = y;
        label->Width = w;
        label->Height = 48;
        label->BorderStyle = BorderStyle::FixedSingle;
        label->BackColor = Color::White;
        label->ForeColor = text_;
        label->Font = gcnew Drawing::Font("Segoe UI", 10.0f, FontStyle::Bold);
        label->Text = title + ": 0";
        label->TextAlign = ContentAlignment::MiddleLeft;
        label->Padding = System::Windows::Forms::Padding(10, 0, 0, 0);
        parent->Controls->Add(label);
        return label;
    }

    void LoadDashboard() {
        double revenue = ScalarDouble("SELECT COALESCE(SUM(total),0) FROM sales WHERE status <> 'Отменено';");
        int productCount = ScalarInt("SELECT COUNT(*) FROM products;");
        int clientCount = ScalarInt("SELECT COUNT(*) FROM clients;");
        int saleCount = ScalarInt("SELECT COUNT(*) FROM sales;");
        int lowStock = ScalarInt("SELECT COUNT(*) FROM products WHERE stock <= 3;");

        if (revenueCard_ != nullptr) revenueCard_->Text = "Выручка: " + revenue.ToString("F2");
        if (productsCard_ != nullptr) productsCard_->Text = "Товары: " + productCount.ToString();
        if (clientsCard_ != nullptr) clientsCard_->Text = "Клиенты: " + clientCount.ToString();
        if (salesCard_ != nullptr) salesCard_->Text = "Продажи: " + saleCount.ToString();
        if (lowStockLabel_ != nullptr) lowStockLabel_->Text = "Низкий остаток товаров: " + lowStock.ToString();

        if (dashboardGrid_ == nullptr) return;
        dashboardGrid_->Rows->Clear();
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT s.sale_date,c.company,p.name,s.quantity,s.total,s.status FROM sales s JOIN clients c ON c.id=s.client_id JOIN products p ON p.id=s.product_id ORDER BY s.id DESC LIMIT 20;";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                dashboardGrid_->Rows->Add(gcnew cli::array<Object^>{
                    FromUtf8(sqlite3_column_text(stmt,0)),
                    FromUtf8(sqlite3_column_text(stmt,1)),
                    FromUtf8(sqlite3_column_text(stmt,2)),
                    sqlite3_column_int(stmt,3),
                    sqlite3_column_double(stmt,4).ToString("F2"),
                    FromUtf8(sqlite3_column_text(stmt,5))
                });
            }
        }
        sqlite3_finalize(stmt);
    }

    void ShowProducts() {
        SetActive(navProducts_);
        selectedProductId_ = 0;
        titleLabel_->Text = "Товары";
        subtitleLabel_->Text = "Добавление, изменение, удаление и фильтрация товаров.";
        TableLayoutPanel^ page = CreateThreeRowPage(96, 150);

        Panel^ filter = CreateCardPanel(DockStyle::Fill, 96);
        page->Controls->Add(filter, 0, 0);
        CreateSectionLabel("Фильтры", 14, 8, 200, filter);
        productSearch_ = AddTextBox(filter, "Поиск", 14, 32, 240);
        productCategoryFilter_ = AddCombo(filter, "Категория", 270, 32, 180);
        productStockFilter_ = AddCombo(filter, "Остаток", 466, 32, 170);
        Button^ clearFilter = AddButton(filter, "Сброс фильтра", 652, 56, 130);
        productSearch_->TextChanged += gcnew EventHandler(this, &MainForm::ProductFilterChanged);
        productCategoryFilter_->SelectedIndexChanged += gcnew EventHandler(this, &MainForm::ProductFilterChanged);
        productStockFilter_->SelectedIndexChanged += gcnew EventHandler(this, &MainForm::ProductFilterChanged);
        clearFilter->Click += gcnew EventHandler(this, &MainForm::ClearProductFiltersClick);

        Panel^ form = CreateCardPanel(DockStyle::Fill, 150);
        form->Margin = System::Windows::Forms::Padding(0);
        page->Controls->Add(form, 0, 2);
        CreateSectionLabel("Форма товара", 14, 8, 200, form);
        productSku_ = AddTextBox(form, "Артикул", 14, 40, 120);
        productName_ = AddTextBox(form, "Название", 150, 40, 240);
        productCategory_ = AddTextBox(form, "Категория", 406, 40, 170);
        productStock_ = AddTextBox(form, "Остаток", 592, 40, 100);
        productPrice_ = AddTextBox(form, "Цена", 708, 40, 110);
        Button^ add = AddButton(form, "Добавить", 14, 94, 110);
        Button^ update = AddButton(form, "Изменить", 136, 94, 110);
        Button^ del = AddButton(form, "Удалить", 258, 94, 110);
        Button^ clear = AddButton(form, "Очистить", 380, 94, 110);
        add->Click += gcnew EventHandler(this, &MainForm::AddProductClick);
        update->Click += gcnew EventHandler(this, &MainForm::UpdateProductClick);
        del->Click += gcnew EventHandler(this, &MainForm::DeleteProductClick);
        clear->Click += gcnew EventHandler(this, &MainForm::ClearProductFormClick);

        productsGrid_ = CreateGrid();
        productsGrid_->Margin = System::Windows::Forms::Padding(0, 10, 0, 10);
        page->Controls->Add(productsGrid_, 0, 1);
        ConfigureColumn(productsGrid_, "id", "ID", 45);
        ConfigureColumn(productsGrid_, "sku", "Артикул", 90);
        ConfigureColumn(productsGrid_, "name", "Название", 230);
        ConfigureColumn(productsGrid_, "category", "Категория", 150);
        ConfigureColumn(productsGrid_, "stock", "Остаток", 80);
        ConfigureColumn(productsGrid_, "price", "Цена", 90);
        productsGrid_->SelectionChanged += gcnew EventHandler(this, &MainForm::ProductSelected);

        FillProductFilters();
        LoadProducts();
    }

    void FillProductFilters() {
        if (productCategoryFilter_ == nullptr) return;
        productCategoryFilter_->Items->Clear();
        productCategoryFilter_->Items->Add("Все категории");
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "SELECT DISTINCT category FROM products ORDER BY category;", -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                productCategoryFilter_->Items->Add(FromUtf8(sqlite3_column_text(stmt,0)));
            }
        }
        sqlite3_finalize(stmt);
        productCategoryFilter_->SelectedIndex = 0;

        productStockFilter_->Items->Clear();
        productStockFilter_->Items->Add("Все остатки");
        productStockFilter_->Items->Add("В наличии");
        productStockFilter_->Items->Add("Мало на складе");
        productStockFilter_->Items->Add("Нет на складе");
        productStockFilter_->SelectedIndex = 0;
    }

    void ProductFilterChanged(Object^, EventArgs^) { LoadProducts(); }
    void ClearProductFiltersClick(Object^, EventArgs^) {
        productSearch_->Text = "";
        productCategoryFilter_->SelectedIndex = 0;
        productStockFilter_->SelectedIndex = 0;
        LoadProducts();
    }

    void LoadProducts() {
        if (productsGrid_ == nullptr) return;
        productsGrid_->Rows->Clear();
        String^ search = productSearch_ != nullptr ? productSearch_->Text->Trim()->ToLower() : "";
        String^ categoryFilter = productCategoryFilter_ != nullptr && productCategoryFilter_->SelectedItem != nullptr ? productCategoryFilter_->SelectedItem->ToString() : "Все категории";
        String^ stockFilter = productStockFilter_ != nullptr && productStockFilter_->SelectedItem != nullptr ? productStockFilter_->SelectedItem->ToString() : "Все остатки";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "SELECT id,sku,name,category,stock,price FROM products ORDER BY id DESC;", -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt,0);
                String^ sku = FromUtf8(sqlite3_column_text(stmt,1));
                String^ name = FromUtf8(sqlite3_column_text(stmt,2));
                String^ category = FromUtf8(sqlite3_column_text(stmt,3));
                int stock = sqlite3_column_int(stmt,4);
                double price = sqlite3_column_double(stmt,5);
                String^ rowText = (sku + " " + name + " " + category)->ToLower();
                if (search->Length > 0 && !rowText->Contains(search)) continue;
                if (categoryFilter != "Все категории" && category != categoryFilter) continue;
                if (stockFilter == "В наличии" && stock <= 0) continue;
                if (stockFilter == "Мало на складе" && !(stock > 0 && stock <= 3)) continue;
                if (stockFilter == "Нет на складе" && stock != 0) continue;
                productsGrid_->Rows->Add(gcnew cli::array<Object^>{ id, sku, name, category, stock, price.ToString("F2") });
            }
        }
        sqlite3_finalize(stmt);
    }

    void ProductSelected(Object^, EventArgs^) {
        if (productsGrid_ == nullptr || productsGrid_->SelectedRows->Count == 0) return;
        DataGridViewRow^ row = productsGrid_->SelectedRows[0];
        selectedProductId_ = Convert::ToInt32(row->Cells[0]->Value);
        productSku_->Text = Convert::ToString(row->Cells[1]->Value);
        productName_->Text = Convert::ToString(row->Cells[2]->Value);
        productCategory_->Text = Convert::ToString(row->Cells[3]->Value);
        productStock_->Text = Convert::ToString(row->Cells[4]->Value);
        productPrice_->Text = Convert::ToString(row->Cells[5]->Value);
    }

    void AddProductClick(Object^, EventArgs^) {
        int stock = 0;
        double price = 0.0;
        if (!ValidateProduct(stock, price)) return;
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "INSERT INTO products(sku,name,category,stock,price) VALUES(?,?,?,?,?);";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            BindText(stmt,1,productSku_->Text); BindText(stmt,2,productName_->Text); BindText(stmt,3,productCategory_->Text);
            sqlite3_bind_int(stmt,4,stock); sqlite3_bind_double(stmt,5,price);
            if (sqlite3_step(stmt) != SQLITE_DONE) ShowDbMessage();
        }
        sqlite3_finalize(stmt);
        FillProductFilters(); LoadProducts(); ClearProductForm();
    }

    void UpdateProductClick(Object^, EventArgs^) {
        if (selectedProductId_ <= 0) { MessageBox::Show("Выберите товар в таблице."); return; }
        int stock = 0; double price = 0.0;
        if (!ValidateProduct(stock, price)) return;
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "UPDATE products SET sku=?,name=?,category=?,stock=?,price=? WHERE id=?;";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            BindText(stmt,1,productSku_->Text); BindText(stmt,2,productName_->Text); BindText(stmt,3,productCategory_->Text);
            sqlite3_bind_int(stmt,4,stock); sqlite3_bind_double(stmt,5,price); sqlite3_bind_int(stmt,6,selectedProductId_);
            if (sqlite3_step(stmt) != SQLITE_DONE) ShowDbMessage();
        }
        sqlite3_finalize(stmt);
        FillProductFilters(); LoadProducts();
    }

    void DeleteProductClick(Object^, EventArgs^) {
        if (selectedProductId_ <= 0) { MessageBox::Show("Выберите товар в таблице."); return; }
        if (ScalarIntForId("SELECT COUNT(*) FROM sales WHERE product_id=?;", selectedProductId_) > 0) {
            MessageBox::Show("Нельзя удалить товар, который уже есть в продажах. Сначала удалите связанные продажи.");
            return;
        }
        if (MessageBox::Show("Удалить выбранный товар?", "Подтверждение", MessageBoxButtons::YesNo, MessageBoxIcon::Question) != System::Windows::Forms::DialogResult::Yes) {
            return;
        }
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "DELETE FROM products WHERE id=?;", -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt,1,selectedProductId_);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
        selectedProductId_ = 0; FillProductFilters(); LoadProducts(); ClearProductForm();
    }

    void ClearProductFormClick(Object^, EventArgs^) { ClearProductForm(); }
    void ClearProductForm() {
        selectedProductId_ = 0;
        if (productSku_ == nullptr) return;
        productSku_->Text = ""; productName_->Text = ""; productCategory_->Text = ""; productStock_->Text = ""; productPrice_->Text = "";
    }

    bool ValidateProduct(int% stock, double% price) {
        if (String::IsNullOrWhiteSpace(productSku_->Text) || String::IsNullOrWhiteSpace(productName_->Text) || String::IsNullOrWhiteSpace(productCategory_->Text)) {
            MessageBox::Show("Заполните артикул, название и категорию.");
            return false;
        }
        if (!Int32::TryParse(productStock_->Text, stock) || stock < 0) {
            MessageBox::Show("Остаток должен быть целым числом не меньше 0.");
            return false;
        }
        if (!Double::TryParse(productPrice_->Text->Replace(L',', L'.'), Globalization::NumberStyles::Any, Globalization::CultureInfo::InvariantCulture, price) || price < 0) {
            MessageBox::Show("Цена должна быть числом не меньше 0.");
            return false;
        }
        return true;
    }

    void ShowClients() {
        SetActive(navClients_);
        selectedClientId_ = 0;
        titleLabel_->Text = "Клиенты";
        subtitleLabel_->Text = "Список клиентов и сумма их покупок.";
        TableLayoutPanel^ page = CreateThreeRowPage(96, 150);

        Panel^ filter = CreateCardPanel(DockStyle::Fill, 96);
        page->Controls->Add(filter, 0, 0);
        CreateSectionLabel("Фильтры", 14, 8, 200, filter);
        clientSearch_ = AddTextBox(filter, "Поиск", 14, 32, 300);
        Button^ clearFilter = AddButton(filter, "Сброс фильтра", 330, 56, 130);
        clientSearch_->TextChanged += gcnew EventHandler(this, &MainForm::ClientFilterChanged);
        clearFilter->Click += gcnew EventHandler(this, &MainForm::ClearClientFiltersClick);

        Panel^ form = CreateCardPanel(DockStyle::Fill, 150);
        form->Margin = System::Windows::Forms::Padding(0);
        page->Controls->Add(form, 0, 2);
        CreateSectionLabel("Форма клиента", 14, 8, 200, form);
        clientCompany_ = AddTextBox(form, "Компания", 14, 40, 220);
        clientContact_ = AddTextBox(form, "Контакт", 250, 40, 190);
        clientPhone_ = AddTextBox(form, "Телефон", 456, 40, 170);
        clientEmail_ = AddTextBox(form, "Email", 642, 40, 240);
        Button^ add = AddButton(form, "Добавить", 14, 94, 110);
        Button^ update = AddButton(form, "Изменить", 136, 94, 110);
        Button^ del = AddButton(form, "Удалить", 258, 94, 110);
        Button^ clear = AddButton(form, "Очистить", 380, 94, 110);
        add->Click += gcnew EventHandler(this, &MainForm::AddClientClick);
        update->Click += gcnew EventHandler(this, &MainForm::UpdateClientClick);
        del->Click += gcnew EventHandler(this, &MainForm::DeleteClientClick);
        clear->Click += gcnew EventHandler(this, &MainForm::ClearClientFormClick);

        clientsGrid_ = CreateGrid();
        clientsGrid_->Margin = System::Windows::Forms::Padding(0, 10, 0, 10);
        page->Controls->Add(clientsGrid_, 0, 1);
        ConfigureColumn(clientsGrid_, "id", "ID", 45);
        ConfigureColumn(clientsGrid_, "company", "Компания", 200);
        ConfigureColumn(clientsGrid_, "contact", "Контакт", 170);
        ConfigureColumn(clientsGrid_, "phone", "Телефон", 150);
        ConfigureColumn(clientsGrid_, "email", "Email", 210);
        ConfigureColumn(clientsGrid_, "sales", "Продажи", 90);
        clientsGrid_->SelectionChanged += gcnew EventHandler(this, &MainForm::ClientSelected);
        LoadClients();
    }

    void ClientFilterChanged(Object^, EventArgs^) { LoadClients(); }
    void ClearClientFiltersClick(Object^, EventArgs^) { clientSearch_->Text = ""; LoadClients(); }

    void LoadClients() {
        if (clientsGrid_ == nullptr) return;
        clientsGrid_->Rows->Clear();
        String^ search = clientSearch_ != nullptr ? clientSearch_->Text->Trim()->ToLower() : "";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "SELECT id,company,contact,phone,email,lifetime_value FROM clients ORDER BY id DESC;", -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt,0);
                String^ company = FromUtf8(sqlite3_column_text(stmt,1));
                String^ contact = FromUtf8(sqlite3_column_text(stmt,2));
                String^ phone = FromUtf8(sqlite3_column_text(stmt,3));
                String^ email = FromUtf8(sqlite3_column_text(stmt,4));
                double value = sqlite3_column_double(stmt,5);
                String^ rowText = (company + " " + contact + " " + phone + " " + email)->ToLower();
                if (search->Length > 0 && !rowText->Contains(search)) continue;
                clientsGrid_->Rows->Add(gcnew cli::array<Object^>{ id, company, contact, phone, email, value.ToString("F2") });
            }
        }
        sqlite3_finalize(stmt);
    }

    void ClientSelected(Object^, EventArgs^) {
        if (clientsGrid_ == nullptr || clientsGrid_->SelectedRows->Count == 0) return;
        DataGridViewRow^ row = clientsGrid_->SelectedRows[0];
        selectedClientId_ = Convert::ToInt32(row->Cells[0]->Value);
        clientCompany_->Text = Convert::ToString(row->Cells[1]->Value);
        clientContact_->Text = Convert::ToString(row->Cells[2]->Value);
        clientPhone_->Text = Convert::ToString(row->Cells[3]->Value);
        clientEmail_->Text = Convert::ToString(row->Cells[4]->Value);
    }

    void AddClientClick(Object^, EventArgs^) {
        if (!ValidateClient()) return;
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "INSERT INTO clients(company,contact,phone,email,lifetime_value) VALUES(?,?,?,?,0);", -1, &stmt, nullptr) == SQLITE_OK) {
            BindText(stmt,1,clientCompany_->Text); BindText(stmt,2,clientContact_->Text); BindText(stmt,3,clientPhone_->Text); BindText(stmt,4,clientEmail_->Text);
            if (sqlite3_step(stmt) != SQLITE_DONE) ShowDbMessage();
        }
        sqlite3_finalize(stmt); LoadClients(); ClearClientForm();
    }

    void UpdateClientClick(Object^, EventArgs^) {
        if (selectedClientId_ <= 0) { MessageBox::Show("Выберите клиента в таблице."); return; }
        if (!ValidateClient()) return;
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "UPDATE clients SET company=?,contact=?,phone=?,email=? WHERE id=?;", -1, &stmt, nullptr) == SQLITE_OK) {
            BindText(stmt,1,clientCompany_->Text); BindText(stmt,2,clientContact_->Text); BindText(stmt,3,clientPhone_->Text); BindText(stmt,4,clientEmail_->Text); sqlite3_bind_int(stmt,5,selectedClientId_);
            if (sqlite3_step(stmt) != SQLITE_DONE) ShowDbMessage();
        }
        sqlite3_finalize(stmt); LoadClients();
    }

    void DeleteClientClick(Object^, EventArgs^) {
        if (selectedClientId_ <= 0) { MessageBox::Show("Выберите клиента в таблице."); return; }
        if (ScalarIntForId("SELECT COUNT(*) FROM sales WHERE client_id=?;", selectedClientId_) > 0) {
            MessageBox::Show("Нельзя удалить клиента, у которого есть продажи. Сначала удалите связанные продажи."); return;
        }
        if (MessageBox::Show("Удалить выбранного клиента?", "Подтверждение", MessageBoxButtons::YesNo, MessageBoxIcon::Question) != System::Windows::Forms::DialogResult::Yes) {
            return;
        }
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "DELETE FROM clients WHERE id=?;", -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt,1,selectedClientId_); sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt); selectedClientId_ = 0; LoadClients(); ClearClientForm();
    }

    void ClearClientFormClick(Object^, EventArgs^) { ClearClientForm(); }
    void ClearClientForm() {
        selectedClientId_ = 0;
        if (clientCompany_ == nullptr) return;
        clientCompany_->Text = ""; clientContact_->Text = ""; clientPhone_->Text = ""; clientEmail_->Text = "";
    }

    bool ValidateClient() {
        if (String::IsNullOrWhiteSpace(clientCompany_->Text) || String::IsNullOrWhiteSpace(clientContact_->Text)) {
            MessageBox::Show("Заполните компанию и контактное лицо."); return false;
        }
        return true;
    }

    void ShowSales() {
        SetActive(navSales_);
        selectedSaleId_ = 0;
        titleLabel_->Text = "Продажи";
        subtitleLabel_->Text = "Создание продаж с автоматическим уменьшением остатка товара.";
        TableLayoutPanel^ page = CreateThreeRowPage(96, 150);

        Panel^ filter = CreateCardPanel(DockStyle::Fill, 96);
        page->Controls->Add(filter, 0, 0);
        CreateSectionLabel("Фильтры", 14, 8, 200, filter);
        saleSearch_ = AddTextBox(filter, "Поиск", 14, 32, 230);
        saleStatusFilter_ = AddCombo(filter, "Статус", 260, 32, 160);
        saleClientFilter_ = AddCombo(filter, "Клиент", 436, 32, 230);
        Button^ clearFilter = AddButton(filter, "Сброс фильтра", 682, 56, 130);
        saleSearch_->TextChanged += gcnew EventHandler(this, &MainForm::SaleFilterChanged);
        saleStatusFilter_->SelectedIndexChanged += gcnew EventHandler(this, &MainForm::SaleFilterChanged);
        saleClientFilter_->SelectedIndexChanged += gcnew EventHandler(this, &MainForm::SaleFilterChanged);
        clearFilter->Click += gcnew EventHandler(this, &MainForm::ClearSaleFiltersClick);

        Panel^ form = CreateCardPanel(DockStyle::Fill, 150);
        form->Margin = System::Windows::Forms::Padding(0);
        page->Controls->Add(form, 0, 2);
        CreateSectionLabel("Форма продажи", 14, 8, 200, form);
        saleClient_ = AddCombo(form, "Клиент", 14, 40, 210);
        saleProduct_ = AddCombo(form, "Товар", 240, 40, 260);
        saleQuantity_ = AddTextBox(form, "Кол-во", 516, 40, 90);
        salePrice_ = AddTextBox(form, "Цена", 622, 40, 110);
        saleStatus_ = AddCombo(form, "Статус", 748, 40, 150);
        saleStatus_->Items->Add("Оплачено"); saleStatus_->Items->Add("Ожидает"); saleStatus_->SelectedIndex = 0;
        saleQuantity_->Text = "1";
        Button^ add = AddButton(form, "Добавить", 14, 94, 110);
        Button^ update = AddButton(form, "Изменить", 136, 94, 110);
        Button^ del = AddButton(form, "Удалить", 258, 94, 110);
        Button^ clear = AddButton(form, "Очистить", 380, 94, 110);
        add->Click += gcnew EventHandler(this, &MainForm::AddSaleClick);
        update->Click += gcnew EventHandler(this, &MainForm::UpdateSaleClick);
        del->Click += gcnew EventHandler(this, &MainForm::DeleteSaleClick);
        clear->Click += gcnew EventHandler(this, &MainForm::ClearSaleFormClick);
        saleProduct_->SelectedIndexChanged += gcnew EventHandler(this, &MainForm::SaleProductChanged);

        salesGrid_ = CreateGrid();
        salesGrid_->Margin = System::Windows::Forms::Padding(0, 10, 0, 10);
        page->Controls->Add(salesGrid_, 0, 1);
        ConfigureColumn(salesGrid_, "id", "ID", 45);
        ConfigureColumn(salesGrid_, "date", "Дата", 130);
        ConfigureColumn(salesGrid_, "client", "Клиент", 170);
        ConfigureColumn(salesGrid_, "product", "Товар", 190);
        ConfigureColumn(salesGrid_, "qty", "Кол-во", 70);
        ConfigureColumn(salesGrid_, "price", "Цена", 85);
        ConfigureColumn(salesGrid_, "total", "Сумма", 90);
        ConfigureColumn(salesGrid_, "status", "Статус", 90);
        salesGrid_->SelectionChanged += gcnew EventHandler(this, &MainForm::SaleSelected);

        FillSaleLookups();
        LoadSales();
    }

    void FillSaleLookups() {
        if (saleClient_ == nullptr) return;
        saleClient_->Items->Clear(); saleClientFilter_->Items->Clear();
        saleClientFilter_->Items->Add(gcnew ComboItem(0, "Все клиенты"));
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "SELECT id,company FROM clients ORDER BY company;", -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt,0);
                String^ name = FromUtf8(sqlite3_column_text(stmt,1));
                saleClient_->Items->Add(gcnew ComboItem(id, name));
                saleClientFilter_->Items->Add(gcnew ComboItem(id, name));
            }
        }
        sqlite3_finalize(stmt);
        if (saleClient_->Items->Count > 0) saleClient_->SelectedIndex = 0;
        saleClientFilter_->SelectedIndex = 0;

        saleProduct_->Items->Clear();
        if (sqlite3_prepare_v2(db_, "SELECT id,name,price,stock FROM products ORDER BY name;", -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt,0);
                String^ name = FromUtf8(sqlite3_column_text(stmt,1));
                double price = sqlite3_column_double(stmt,2);
                int stock = sqlite3_column_int(stmt,3);
                saleProduct_->Items->Add(gcnew ComboItem(id, name + " | остаток: " + stock.ToString() + " | " + price.ToString("F2")));
            }
        }
        sqlite3_finalize(stmt);
        if (saleProduct_->Items->Count > 0) saleProduct_->SelectedIndex = 0;

        saleStatusFilter_->Items->Clear();
        saleStatusFilter_->Items->Add("Все статусы"); saleStatusFilter_->Items->Add("Оплачено"); saleStatusFilter_->Items->Add("Ожидает");
        saleStatusFilter_->SelectedIndex = 0;
    }

    void SaleFilterChanged(Object^, EventArgs^) { LoadSales(); }
    void ClearSaleFiltersClick(Object^, EventArgs^) {
        saleSearch_->Text = ""; saleStatusFilter_->SelectedIndex = 0; saleClientFilter_->SelectedIndex = 0; LoadSales();
    }

    void LoadSales() {
        if (salesGrid_ == nullptr) return;
        salesGrid_->Rows->Clear();
        String^ search = saleSearch_ != nullptr ? saleSearch_->Text->Trim()->ToLower() : "";
        String^ status = saleStatusFilter_ != nullptr && saleStatusFilter_->SelectedItem != nullptr ? saleStatusFilter_->SelectedItem->ToString() : "Все статусы";
        int clientFilter = SelectedComboId(saleClientFilter_);
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "SELECT s.id,s.sale_date,c.company,p.name,s.quantity,s.unit_price,s.total,s.status,s.client_id,s.product_id FROM sales s JOIN clients c ON c.id=s.client_id JOIN products p ON p.id=s.product_id ORDER BY s.id DESC;";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt,0);
                String^ date = FromUtf8(sqlite3_column_text(stmt,1));
                String^ client = FromUtf8(sqlite3_column_text(stmt,2));
                String^ product = FromUtf8(sqlite3_column_text(stmt,3));
                int qty = sqlite3_column_int(stmt,4);
                double price = sqlite3_column_double(stmt,5);
                double total = sqlite3_column_double(stmt,6);
                String^ rowStatus = FromUtf8(sqlite3_column_text(stmt,7));
                int rowClientId = sqlite3_column_int(stmt,8);
                int rowProductId = sqlite3_column_int(stmt,9);
                String^ rowText = (date + " " + client + " " + product + " " + rowStatus)->ToLower();
                if (search->Length > 0 && !rowText->Contains(search)) continue;
                if (status != "Все статусы" && rowStatus != status) continue;
                if (clientFilter > 0 && rowClientId != clientFilter) continue;
                int index = salesGrid_->Rows->Add(gcnew cli::array<Object^>{ id, date, client, product, qty, price.ToString("F2"), total.ToString("F2"), rowStatus });
                salesGrid_->Rows[index]->Tag = gcnew cli::array<int>{ rowClientId, rowProductId };
            }
        }
        sqlite3_finalize(stmt);
    }

    void SaleProductChanged(Object^, EventArgs^) {
        int productId = SelectedComboId(saleProduct_);
        double price = ProductPrice(productId);
        salePrice_->Text = price.ToString("F2");
    }

    void SaleSelected(Object^, EventArgs^) {
        if (salesGrid_ == nullptr || salesGrid_->SelectedRows->Count == 0) return;
        DataGridViewRow^ row = salesGrid_->SelectedRows[0];
        selectedSaleId_ = Convert::ToInt32(row->Cells[0]->Value);
        cli::array<int>^ ids = dynamic_cast<cli::array<int>^>(row->Tag);
        if (ids != nullptr) {
            SelectComboById(saleClient_, ids[0]);
            SelectComboById(saleProduct_, ids[1]);
        }
        saleQuantity_->Text = Convert::ToString(row->Cells[4]->Value);
        salePrice_->Text = Convert::ToString(row->Cells[5]->Value);
        saleStatus_->SelectedItem = Convert::ToString(row->Cells[7]->Value);
    }

    void AddSaleClick(Object^, EventArgs^) {
        int clientId = SelectedComboId(saleClient_);
        int productId = SelectedComboId(saleProduct_);
        int qty = 0; double price = 0.0;
        if (!ValidateSale(clientId, productId, qty, price)) return;
        int stock = ProductStock(productId);
        if (stock < qty) { MessageBox::Show("Недостаточно товара на складе."); return; }
        double total = qty * price;
        Exec("BEGIN TRANSACTION;");
        sqlite3_stmt* stmt = nullptr;
        const char* sql = "INSERT INTO sales(sale_date,client_id,product_id,quantity,unit_price,total,status) VALUES(?,?,?,?,?,?,?);";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            BindText(stmt,1,TodayText()); sqlite3_bind_int(stmt,2,clientId); sqlite3_bind_int(stmt,3,productId); sqlite3_bind_int(stmt,4,qty); sqlite3_bind_double(stmt,5,price); sqlite3_bind_double(stmt,6,total); BindText(stmt,7,saleStatus_->Text);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
        ChangeStock(productId, -qty);
        RecalculateClients();
        Exec("COMMIT;");
        FillSaleLookups(); LoadSales(); ClearSaleForm();
    }

    void UpdateSaleClick(Object^, EventArgs^) {
        if (selectedSaleId_ <= 0) { MessageBox::Show("Выберите продажу в таблице."); return; }
        int newClient = SelectedComboId(saleClient_);
        int newProduct = SelectedComboId(saleProduct_);
        int newQty = 0; double newPrice = 0.0;
        if (!ValidateSale(newClient, newProduct, newQty, newPrice)) return;
        int oldProduct = 0, oldQty = 0;
        LoadSaleStockInfo(selectedSaleId_, oldProduct, oldQty);
        Exec("BEGIN TRANSACTION;");
        ChangeStock(oldProduct, oldQty);
        if (ProductStock(newProduct) < newQty) {
            ChangeStock(oldProduct, -oldQty);
            Exec("ROLLBACK;");
            MessageBox::Show("Недостаточно товара на складе.");
            return;
        }
        ChangeStock(newProduct, -newQty);
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "UPDATE sales SET client_id=?,product_id=?,quantity=?,unit_price=?,total=?,status=? WHERE id=?;", -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt,1,newClient); sqlite3_bind_int(stmt,2,newProduct); sqlite3_bind_int(stmt,3,newQty); sqlite3_bind_double(stmt,4,newPrice); sqlite3_bind_double(stmt,5,newQty*newPrice); BindText(stmt,6,saleStatus_->Text); sqlite3_bind_int(stmt,7,selectedSaleId_);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
        RecalculateClients();
        Exec("COMMIT;");
        FillSaleLookups(); LoadSales();
    }

    void DeleteSaleClick(Object^, EventArgs^) {
        if (selectedSaleId_ <= 0) { MessageBox::Show("Выберите продажу в таблице."); return; }
        if (MessageBox::Show("Удалить выбранную продажу? Остаток товара будет возвращён на склад.", "Подтверждение", MessageBoxButtons::YesNo, MessageBoxIcon::Question) != System::Windows::Forms::DialogResult::Yes) {
            return;
        }
        int oldProduct = 0, oldQty = 0;
        LoadSaleStockInfo(selectedSaleId_, oldProduct, oldQty);
        Exec("BEGIN TRANSACTION;");
        ChangeStock(oldProduct, oldQty);
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "DELETE FROM sales WHERE id=?;", -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt,1,selectedSaleId_); sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
        RecalculateClients();
        Exec("COMMIT;");
        selectedSaleId_ = 0; FillSaleLookups(); LoadSales(); ClearSaleForm();
    }

    void ClearSaleFormClick(Object^, EventArgs^) { ClearSaleForm(); }
    void ClearSaleForm() {
        selectedSaleId_ = 0;
        if (saleQuantity_ == nullptr) return;
        saleQuantity_->Text = "1";
        if (saleClient_->Items->Count > 0) saleClient_->SelectedIndex = 0;
        if (saleProduct_->Items->Count > 0) saleProduct_->SelectedIndex = 0;
        if (saleStatus_->Items->Count > 0) saleStatus_->SelectedIndex = 0;
        SaleProductChanged(nullptr, EventArgs::Empty);
    }

    bool ValidateSale(int clientId, int productId, int% qty, double% price) {
        if (clientId <= 0 || productId <= 0) { MessageBox::Show("Выберите клиента и товар."); return false; }
        if (!Int32::TryParse(saleQuantity_->Text, qty) || qty <= 0) { MessageBox::Show("Количество должно быть больше 0."); return false; }
        if (!Double::TryParse(salePrice_->Text->Replace(L',', L'.'), Globalization::NumberStyles::Any, Globalization::CultureInfo::InvariantCulture, price) || price < 0) { MessageBox::Show("Цена должна быть числом не меньше 0."); return false; }
        return true;
    }

    void ShowSettings() {
        SetActive(navSettings_);
        titleLabel_->Text = "Настройки";
        subtitleLabel_->Text = "Служебные действия, информация о проекте и поддержке.";
        TableLayoutPanel^ page = CreateSettingsLayout();
        Panel^ info = CreateCardPanel(DockStyle::Fill, 230);
        page->Controls->Add(info, 0, 0);
        CreateSectionLabel("О проекте", 16, 16, 300, info);
        Label^ desc = gcnew Label();
        desc->Text = "Базовая CLR .NET версия программы для мониторинга продаж. Оставлены только основные функции: товары, клиенты, продажи, фильтры, CRUD-операции и простая статистика.";
        desc->Left = 16; desc->Top = 46; desc->Width = 760; desc->Height = 44; desc->ForeColor = muted_;
        info->Controls->Add(desc);
        CreateSectionLabel("Спонсор и поддержка", 16, 104, 300, info);
        Label^ sponsor = gcnew Label();
        sponsor->Text = "Fix It All";
        sponsor->Left = 16; sponsor->Top = 134; sponsor->Width = 240; sponsor->Height = 34;
        sponsor->ForeColor = Color::FromArgb(22, 101, 52);
        sponsor->Font = gcnew Drawing::Font("Segoe UI", 16.0f, FontStyle::Bold);
        info->Controls->Add(sponsor);
        Label^ sponsorText = gcnew Label();
        sponsorText->Text = "Качество. Надежность. Решения. Поддержка базовой версии проекта.";
        sponsorText->Left = 16; sponsorText->Top = 170; sponsorText->Width = 520; sponsorText->Height = 24; sponsorText->ForeColor = muted_;
        info->Controls->Add(sponsorText);

        Panel^ actions = CreateCardPanel(DockStyle::Fill, 100);
        actions->Margin = System::Windows::Forms::Padding(0);
        page->Controls->Add(actions, 0, 1);
        Button^ reset = AddButton(actions, "Сбросить демо-данные", 16, 28, 210);
        Button^ refresh = AddButton(actions, "Обновить данные", 242, 28, 170);
        reset->Click += gcnew EventHandler(this, &MainForm::ResetDemoClick);
        refresh->Click += gcnew EventHandler(this, &MainForm::RefreshClick);
    }

    void ResetDemoClick(Object^, EventArgs^) {
        if (MessageBox::Show("Сбросить базу и восстановить демо-данные? Текущие изменения будут удалены.", "Подтверждение", MessageBoxButtons::YesNo, MessageBoxIcon::Warning) != System::Windows::Forms::DialogResult::Yes) {
            return;
        }
        ResetDemoData(true);
    }
    void RefreshClick(Object^, EventArgs^) { RefreshCurrentPage(); }

    void RefreshCurrentPage() {
        if (navDashboard_->BackColor == Color::FromArgb(219, 234, 254)) { LoadDashboard(); return; }
        if (navProducts_->BackColor == Color::FromArgb(219, 234, 254)) { FillProductFilters(); LoadProducts(); return; }
        if (navClients_->BackColor == Color::FromArgb(219, 234, 254)) { LoadClients(); return; }
        if (navSales_->BackColor == Color::FromArgb(219, 234, 254)) { FillSaleLookups(); LoadSales(); return; }
    }

    void BindText(sqlite3_stmt* stmt, int index, String^ value) {
        std::string text = ToUtf8(value);
        sqlite3_bind_text(stmt, index, text.c_str(), -1, SQLITE_TRANSIENT);
    }

    void ShowDbMessage() {
        MessageBox::Show(FromUtf8(reinterpret_cast<const unsigned char*>(sqlite3_errmsg(db_))), "Ошибка SQLite", MessageBoxButtons::OK, MessageBoxIcon::Error);
    }

    int ScalarIntForId(const char* sql, int id) {
        sqlite3_stmt* stmt = nullptr;
        int value = 0;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt,1,id);
            if (sqlite3_step(stmt) == SQLITE_ROW) value = sqlite3_column_int(stmt,0);
        }
        sqlite3_finalize(stmt);
        return value;
    }

    int SelectedComboId(ComboBox^ combo) {
        if (combo == nullptr || combo->SelectedItem == nullptr) return 0;
        ComboItem^ item = dynamic_cast<ComboItem^>(combo->SelectedItem);
        return item == nullptr ? 0 : item->Id;
    }

    void SelectComboById(ComboBox^ combo, int id) {
        if (combo == nullptr) return;
        for (int i = 0; i < combo->Items->Count; ++i) {
            ComboItem^ item = dynamic_cast<ComboItem^>(combo->Items[i]);
            if (item != nullptr && item->Id == id) { combo->SelectedIndex = i; return; }
        }
    }

    int ProductStock(int productId) { return ScalarIntForId("SELECT stock FROM products WHERE id=?;", productId); }

    double ProductPrice(int productId) {
        sqlite3_stmt* stmt = nullptr;
        double value = 0.0;
        if (sqlite3_prepare_v2(db_, "SELECT price FROM products WHERE id=?;", -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt,1,productId);
            if (sqlite3_step(stmt) == SQLITE_ROW) value = sqlite3_column_double(stmt,0);
        }
        sqlite3_finalize(stmt);
        return value;
    }

    void ChangeStock(int productId, int delta) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "UPDATE products SET stock = stock + ? WHERE id=?;", -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt,1,delta); sqlite3_bind_int(stmt,2,productId); sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }

    void LoadSaleStockInfo(int saleId, int% productId, int% qty) {
        productId = 0; qty = 0;
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, "SELECT product_id,quantity FROM sales WHERE id=?;", -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt,1,saleId);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                productId = sqlite3_column_int(stmt,0);
                qty = sqlite3_column_int(stmt,1);
            }
        }
        sqlite3_finalize(stmt);
    }
};
}

[STAThreadAttribute]
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);
    SalesFlowCLR::MainForm^ form = gcnew SalesFlowCLR::MainForm();
    Application::Run(form);
    return 0;
}
