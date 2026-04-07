#include "cache.h"
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

ResultCache::ResultCache(const std::string& db_path) : db_(nullptr) {
    fs::path path(db_path);
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }

    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
    }

    create_tables();
}

ResultCache::~ResultCache() {
    close();
}

void ResultCache::create_tables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            f_poly TEXT NOT NULL,
            g_poly TEXT NOT NULL,
            q_poly TEXT,
            q_min TEXT,
            error TEXT,
            UNIQUE(f_poly, g_poly)
        )
    )";

    char* err_msg = nullptr;
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::string error = err_msg;
        sqlite3_free(err_msg);
        throw std::runtime_error("Failed to create table: " + error);
    }

    sqlite3_exec(db_, "CREATE INDEX IF NOT EXISTS idx_fg ON results(f_poly, g_poly)", nullptr, nullptr, nullptr);
}

void ResultCache::save_results_batch(const std::vector<ResultToSave>& results) {
    if (results.empty()) return;

    sqlite3_exec(db_, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    const char* sql = R"(
        INSERT OR REPLACE INTO results (f_poly, g_poly, q_poly, q_min, error)
        VALUES (?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
        return;
    }

    for (const auto& r : results) {
        sqlite3_bind_text(stmt, 1, r.f_str.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, r.g_str.c_str(), -1, SQLITE_TRANSIENT);

        if (r.q_str.has_value()) {
            sqlite3_bind_text(stmt, 3, r.q_str.value().c_str(), -1, SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(stmt, 3);
        }

        if (r.q_min_str.has_value()) {
            sqlite3_bind_text(stmt, 4, r.q_min_str.value().c_str(), -1, SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(stmt, 4);
        }

        if (r.error.has_value()) {
            sqlite3_bind_text(stmt, 5, r.error.value().c_str(), -1, SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(stmt, 5);
        }

        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
}

void ResultCache::save_result(const CachedResult& r) {
    const char* sql = R"(
        INSERT OR REPLACE INTO results (f_poly, g_poly, q_poly, q_min, error)
        VALUES (?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    sqlite3_bind_text(stmt, 1, r.f_poly.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, r.g_poly.c_str(), -1, SQLITE_TRANSIENT);

    if (r.q_poly.has_value()) {
        sqlite3_bind_text(stmt, 3, r.q_poly.value().c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 3);
    }

    if (r.q_min.has_value()) {
        sqlite3_bind_text(stmt, 4, r.q_min.value().c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 4);
    }

    if (r.error.has_value()) {
        sqlite3_bind_text(stmt, 5, r.error.value().c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 5);
    }

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<CachedResult> ResultCache::query_all(int limit) const {
    std::string sql = "SELECT f_poly, g_poly, q_poly, q_min, error FROM results";
    if (limit > 0) sql += " LIMIT " + std::to_string(limit);

    sqlite3_stmt* stmt;
    std::vector<CachedResult> results;

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return results;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CachedResult r;
        r.f_poly = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        r.g_poly = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
            r.q_poly = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL)
            r.q_min = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
            r.error = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

        results.push_back(std::move(r));
    }

    sqlite3_finalize(stmt);
    return results;
}

void ResultCache::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}
