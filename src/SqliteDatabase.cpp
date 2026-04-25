#include "SqliteDatabase.h"

#include "sqlite3.h"

#include <windows.h>

namespace data {
namespace {

std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return L"";
    }
    int needed = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(static_cast<size_t>(needed), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), needed);
    return result;
}

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return "";
    }
    int needed = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), needed, nullptr, nullptr);
    return result;
}

} // namespace

std::string ToUtf8(const std::wstring& value) {
    return WideToUtf8(value);
}

std::wstring FromUtf8(const char* value) {
    return value ? Utf8ToWide(value) : L"";
}

SqliteStatement::SqliteStatement() : statement_(nullptr) {}

SqliteStatement::SqliteStatement(sqlite3_stmt* statement) : statement_(statement) {}

SqliteStatement::~SqliteStatement() {
    if (statement_) {
        sqlite3_finalize(statement_);
        statement_ = nullptr;
    }
}

SqliteStatement::SqliteStatement(SqliteStatement&& other) noexcept : statement_(other.statement_) {
    other.statement_ = nullptr;
}

SqliteStatement& SqliteStatement::operator=(SqliteStatement&& other) noexcept {
    if (this != &other) {
        if (statement_) {
            sqlite3_finalize(statement_);
        }
        statement_ = other.statement_;
        other.statement_ = nullptr;
    }
    return *this;
}

bool SqliteStatement::IsValid() const {
    return statement_ != nullptr;
}

void SqliteStatement::Reset() {
    if (statement_) {
        sqlite3_reset(statement_);
    }
}

void SqliteStatement::ClearBindings() {
    if (statement_) {
        sqlite3_clear_bindings(statement_);
    }
}

bool SqliteStatement::BindInt(int index, int value) {
    return statement_ && sqlite3_bind_int(statement_, index, value) == SQLITE_OK;
}

bool SqliteStatement::BindDouble(int index, double value) {
    return statement_ && sqlite3_bind_double(statement_, index, value) == SQLITE_OK;
}

bool SqliteStatement::BindText(int index, const std::wstring& value) {
    const std::string utf8 = WideToUtf8(value);
    return statement_ && sqlite3_bind_text(statement_, index, utf8.c_str(), static_cast<int>(utf8.size()), SQLITE_TRANSIENT) == SQLITE_OK;
}

bool SqliteStatement::BindNull(int index) {
    return statement_ && sqlite3_bind_null(statement_, index) == SQLITE_OK;
}

int SqliteStatement::Step() {
    return statement_ ? sqlite3_step(statement_) : SQLITE_MISUSE;
}

int SqliteStatement::ColumnInt(int index) const {
    return statement_ ? sqlite3_column_int(statement_, index) : 0;
}

double SqliteStatement::ColumnDouble(int index) const {
    return statement_ ? sqlite3_column_double(statement_, index) : 0.0;
}

std::wstring SqliteStatement::ColumnText(int index) const {
    return statement_ ? FromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(statement_, index))) : L"";
}

sqlite3_stmt* SqliteStatement::Get() const {
    return statement_;
}

SqliteDatabase::SqliteDatabase() : db_(nullptr) {}

SqliteDatabase::~SqliteDatabase() {
    Close();
}

bool SqliteDatabase::Open(const std::wstring& path, std::wstring* error) {
    Close();
    const std::string utf8Path = WideToUtf8(path);
    if (sqlite3_open_v2(utf8Path.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        SetLastError(FromUtf8(sqlite3_errmsg(db_)));
        if (error) {
            *error = lastError_;
        }
        Close();
        return false;
    }
    sqlite3_busy_timeout(db_, 1500);
    Execute("PRAGMA foreign_keys = ON;", nullptr);
    Execute("PRAGMA journal_mode = WAL;", nullptr);
    Execute("PRAGMA synchronous = NORMAL;", nullptr);
    return true;
}

void SqliteDatabase::Close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool SqliteDatabase::Execute(const std::string& sql, std::wstring* error) const {
    if (!db_) {
        SetLastError(L"Database connection is not open.");
        if (error) {
            *error = lastError_;
        }
        return false;
    }

    char* rawError = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &rawError) != SQLITE_OK) {
        SetLastError(FromUtf8(rawError));
        sqlite3_free(rawError);
        if (error) {
            *error = lastError_;
        }
        return false;
    }
    return true;
}

bool SqliteDatabase::Prepare(const std::string& sql, SqliteStatement* statement, std::wstring* error) const {
    if (!db_ || !statement) {
        SetLastError(L"Database connection is not open.");
        if (error) {
            *error = lastError_;
        }
        return false;
    }

    sqlite3_stmt* rawStatement = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), static_cast<int>(sql.size()), &rawStatement, nullptr) != SQLITE_OK) {
        SetLastError(FromUtf8(sqlite3_errmsg(db_)));
        if (error) {
            *error = lastError_;
        }
        return false;
    }

    *statement = SqliteStatement(rawStatement);
    return true;
}

int SqliteDatabase::QueryScalarInt(const std::string& sql, int fallback) const {
    SqliteStatement statement;
    if (!Prepare(sql, &statement, nullptr)) {
        return fallback;
    }
    return statement.Step() == SQLITE_ROW ? statement.ColumnInt(0) : fallback;
}

std::wstring SqliteDatabase::QueryScalarText(const std::string& sql, const std::wstring& fallback) const {
    SqliteStatement statement;
    if (!Prepare(sql, &statement, nullptr)) {
        return fallback;
    }
    return statement.Step() == SQLITE_ROW ? statement.ColumnText(0) : fallback;
}

bool SqliteDatabase::IsOpen() const {
    return db_ != nullptr;
}

sqlite3* SqliteDatabase::Handle() const {
    return db_;
}

const std::wstring& SqliteDatabase::LastError() const {
    return lastError_;
}

void SqliteDatabase::SetLastError(const std::wstring& error) const {
    lastError_ = error;
}

} // namespace data
