#include "cache.h"
#include "polynomial.h"
#include <filesystem>
#include <stdexcept>
#include <sstream>

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
    const char* create_table_sql = R"(
        CREATE TABLE IF NOT EXISTS results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            f_poly TEXT NOT NULL,
            g_poly TEXT NOT NULL,
            f_hash TEXT NOT NULL,
            g_hash TEXT NOT NULL,
            q_poly TEXT,
            is_trivial INTEGER DEFAULT 0,
            df_divisible INTEGER,
            dg_divisible INTEGER,
            both_divisible INTEGER,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(f_hash, g_hash)
        )
    )";
    
    char* err_msg = nullptr;
    if (sqlite3_exec(db_, create_table_sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::string error = err_msg;
        sqlite3_free(err_msg);
        throw std::runtime_error("Failed to create table: " + error);
    }
    
    const char* create_index1 = "CREATE INDEX IF NOT EXISTS idx_hashes ON results(f_hash, g_hash)";
    const char* create_index2 = "CREATE INDEX IF NOT EXISTS idx_both_divisible ON results(both_divisible)";
    
    sqlite3_exec(db_, create_index1, nullptr, nullptr, nullptr);
    sqlite3_exec(db_, create_index2, nullptr, nullptr, nullptr);
}

std::optional<CachedResult> ResultCache::get_result(const GiNaC::ex& f, const GiNaC::ex& g) {
    std::string f_hash_val = PolynomialOps::poly_hash(f);
    std::string g_hash_val = PolynomialOps::poly_hash(g);
    
    const char* sql = "SELECT * FROM results WHERE f_hash = ? AND g_hash = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    
    sqlite3_bind_text(stmt, 1, f_hash_val.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, g_hash_val.c_str(), -1, SQLITE_TRANSIENT);
    
    std::optional<CachedResult> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        CachedResult r;
        r.f_poly = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        r.g_poly = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        r.f_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.g_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        
        if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
            r.q_poly = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        }
        
        r.is_trivial = sqlite3_column_int(stmt, 6) != 0;
        r.df_divisible = sqlite3_column_int(stmt, 7) != 0;
        r.dg_divisible = sqlite3_column_int(stmt, 8) != 0;
        r.both_divisible = sqlite3_column_int(stmt, 9) != 0;
        r.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        
        result = r;
    }
    
    sqlite3_finalize(stmt);
    return result;
}

void ResultCache::save_result(const GiNaC::ex& f, const GiNaC::ex& g,
                               const std::optional<GiNaC::ex>& q,
                               const DivisibilityResult& divisibility,
                               bool is_trivial) {
    std::ostringstream f_oss, g_oss;
    f_oss << f;
    g_oss << g;
    
    std::string f_str = f_oss.str();
    std::string g_str = g_oss.str();
    std::string f_hash_val = PolynomialOps::poly_hash(f);
    std::string g_hash_val = PolynomialOps::poly_hash(g);
    
    const char* sql = R"(
        INSERT OR REPLACE INTO results
        (f_poly, g_poly, f_hash, g_hash, q_poly, is_trivial, df_divisible, dg_divisible, both_divisible)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }
    
    sqlite3_bind_text(stmt, 1, f_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, g_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, f_hash_val.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, g_hash_val.c_str(), -1, SQLITE_TRANSIENT);
    
    if (q.has_value()) {
        std::ostringstream q_oss;
        q_oss << q.value();
        std::string q_str = q_oss.str();
        sqlite3_bind_text(stmt, 5, q_str.c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 5);
    }
    
    sqlite3_bind_int(stmt, 6, is_trivial ? 1 : 0);
    sqlite3_bind_int(stmt, 7, divisibility.df_divisible ? 1 : 0);
    sqlite3_bind_int(stmt, 8, divisibility.dg_divisible ? 1 : 0);
    sqlite3_bind_int(stmt, 9, divisibility.both_divisible ? 1 : 0);
    
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void ResultCache::save_results_batch(const std::vector<ResultToSave>& results) {
    if (results.empty()) {
        return;
    }
    
    sqlite3_exec(db_, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
    
    const char* sql = R"(
        INSERT OR REPLACE INTO results
        (f_poly, g_poly, f_hash, g_hash, q_poly, is_trivial, df_divisible, dg_divisible, both_divisible)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
        return;
    }
    
    for (const auto& result : results) {
        sqlite3_bind_text(stmt, 1, result.f_str.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, result.g_str.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, result.f_hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, result.g_hash.c_str(), -1, SQLITE_TRANSIENT);
        
        if (result.q_str.has_value()) {
            sqlite3_bind_text(stmt, 5, result.q_str.value().c_str(), -1, SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(stmt, 5);
        }
        
        sqlite3_bind_int(stmt, 6, result.is_trivial ? 1 : 0);
        sqlite3_bind_int(stmt, 7, result.divisibility.df_divisible ? 1 : 0);
        sqlite3_bind_int(stmt, 8, result.divisibility.dg_divisible ? 1 : 0);
        sqlite3_bind_int(stmt, 9, result.divisibility.both_divisible ? 1 : 0);
        
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
}

Statistics ResultCache::get_statistics() {
    const char* sql = R"(
        SELECT
            COUNT(*) as total,
            SUM(CASE WHEN q_poly IS NOT NULL THEN 1 ELSE 0 END) as with_dependency,
            SUM(CASE WHEN is_trivial = 1 THEN 1 ELSE 0 END) as trivial_rejected,
            SUM(CASE WHEN q_poly IS NOT NULL AND is_trivial = 0 THEN 1 ELSE 0 END) as nontrivial_found,
            SUM(CASE WHEN df_divisible = 1 AND dg_divisible = 0 THEN 1 ELSE 0 END) as df_divisible_only,
            SUM(CASE WHEN df_divisible = 0 AND dg_divisible = 1 THEN 1 ELSE 0 END) as dg_divisible_only,
            SUM(CASE WHEN both_divisible = 1 THEN 1 ELSE 0 END) as both_divisible,
            SUM(CASE WHEN q_poly IS NULL THEN 1 ELSE 0 END) as no_dependency
        FROM results
    )";
    
    sqlite3_stmt* stmt;
    Statistics stats = {0, 0, 0, 0, 0, 0, 0, 0};
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total = sqlite3_column_int(stmt, 0);
            stats.with_dependency = sqlite3_column_int(stmt, 1);
            stats.trivial_rejected = sqlite3_column_int(stmt, 2);
            stats.nontrivial_found = sqlite3_column_int(stmt, 3);
            stats.df_divisible_only = sqlite3_column_int(stmt, 4);
            stats.dg_divisible_only = sqlite3_column_int(stmt, 5);
            stats.both_divisible = sqlite3_column_int(stmt, 6);
            stats.no_dependency = sqlite3_column_int(stmt, 7);
        }
        sqlite3_finalize(stmt);
    }
    
    return stats;
}

std::vector<StatRow> ResultCache::get_detailed_statistics() {
    const char* sql = R"(
        SELECT
            CASE WHEN q_poly IS NULL THEN 0 ELSE 1 END as has_dep,
            COALESCE(is_trivial, 0) as is_trivial,
            COALESCE(df_divisible, 0) as df_div,
            COALESCE(dg_divisible, 0) as dg_div,
            COUNT(*) as count
        FROM results
        GROUP BY has_dep, is_trivial, df_div, dg_div
        ORDER BY has_dep DESC, is_trivial ASC, df_div DESC, dg_div DESC
    )";
    
    sqlite3_stmt* stmt;
    std::vector<StatRow> rows;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            StatRow row;
            row.has_dependency = sqlite3_column_int(stmt, 0) != 0;
            bool is_trivial = sqlite3_column_int(stmt, 1) != 0;
            row.is_nontrivial = row.has_dependency && !is_trivial;
            row.df_divisible = sqlite3_column_int(stmt, 2) != 0;
            row.dg_divisible = sqlite3_column_int(stmt, 3) != 0;
            row.both_divisible = row.df_divisible && row.dg_divisible;
            row.count = sqlite3_column_int(stmt, 4);
            rows.push_back(row);
        }
        sqlite3_finalize(stmt);
    }
    
    return rows;
}

std::vector<CachedResult> ResultCache::query_results(bool both_divisible, bool nontrivial_only, bool trivial_only) {
    std::string sql = "SELECT * FROM results WHERE 1=1";
    
    if (both_divisible) {
        sql += " AND both_divisible = 1";
    }
    
    if (nontrivial_only) {
        sql += " AND is_trivial = 0";
    }
    
    if (trivial_only) {
        sql += " AND is_trivial = 1";
    }
    
    sql += " ORDER BY timestamp DESC";
    
    sqlite3_stmt* stmt;
    std::vector<CachedResult> results;
    
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            CachedResult r;
            r.f_poly = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            r.g_poly = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            r.f_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            r.g_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            
            if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
                r.q_poly = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            }
            
            r.is_trivial = sqlite3_column_int(stmt, 6) != 0;
            r.df_divisible = sqlite3_column_int(stmt, 7) != 0;
            r.dg_divisible = sqlite3_column_int(stmt, 8) != 0;
            r.both_divisible = sqlite3_column_int(stmt, 9) != 0;
            r.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
            
            results.push_back(r);
        }
        sqlite3_finalize(stmt);
    }
    
    return results;
}

void ResultCache::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}