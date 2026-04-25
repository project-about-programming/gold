#pragma once

#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace data {

std::string ToUtf8(const std::wstring& value);
std::wstring FromUtf8(const char* value);

class SqliteStatement {
public:
    SqliteStatement();
    explicit SqliteStatement(sqlite3_stmt* statement);
    ~SqliteStatement();

    SqliteStatement(const SqliteStatement&) = delete;
    SqliteStatement& operator=(const SqliteStatement&) = delete;
    SqliteStatement(SqliteStatement&& other) noexcept;
    SqliteStatement& operator=(SqliteStatement&& other) noexcept;

    bool IsValid() const;
    void Reset();
    void ClearBindings();

    bool BindInt(int index, int value);
    bool BindDouble(int index, double value);
    bool BindText(int index, const std::wstring& value);
    bool BindNull(int index);

    int Step();
    int ColumnInt(int index) const;
    double ColumnDouble(int index) const;
    std::wstring ColumnText(int index) const;

    sqlite3_stmt* Get() const;

private:
    sqlite3_stmt* statement_;
};

class SqliteDatabase {
public:
    SqliteDatabase();
    ~SqliteDatabase();

    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;

    bool Open(const std::wstring& path, std::wstring* error = nullptr);
    void Close();

    bool Execute(const std::string& sql, std::wstring* error = nullptr) const;
    bool Prepare(const std::string& sql, SqliteStatement* statement, std::wstring* error = nullptr) const;

    int QueryScalarInt(const std::string& sql, int fallback = 0) const;
    std::wstring QueryScalarText(const std::string& sql, const std::wstring& fallback = L"") const;

    bool IsOpen() const;
    sqlite3* Handle() const;
    const std::wstring& LastError() const;

private:
    void SetLastError(const std::wstring& error) const;

    sqlite3* db_;
    mutable std::wstring lastError_;
};

} // namespace data
