#pragma once

#include <sqlite3.h>
#include <string>
#include <optional>
#include <vector>

struct CachedResult {
    std::string f_poly;
    std::string g_poly;
    std::optional<std::string> q_poly;
    std::optional<std::string> q_min;
    std::optional<std::string> error;
};

struct ResultToSave {
    std::string f_str;
    std::string g_str;
    std::optional<std::string> q_str;
    std::optional<std::string> q_min_str;
    std::optional<std::string> error;
};

class ResultCache {
public:
    explicit ResultCache(const std::string& db_path);
    ~ResultCache();

    ResultCache(const ResultCache&) = delete;
    ResultCache& operator=(const ResultCache&) = delete;

    void save_results_batch(const std::vector<ResultToSave>& results);
    void save_result(const CachedResult& result);
    std::vector<CachedResult> query_all(int limit = 0) const;

    void close();

private:
    sqlite3* db_;

    void create_tables();
};
