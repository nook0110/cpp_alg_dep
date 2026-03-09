#pragma once

#include <ginac/ginac.h>
#include <sqlite3.h>
#include <string>
#include <optional>
#include <map>
#include <vector>
#include "divisibility.h"

struct CachedResult {
    std::string f_poly;
    std::string g_poly;
    std::string f_hash;
    std::string g_hash;
    std::optional<std::string> q_poly;
    bool is_trivial;
    bool df_divisible;
    bool dg_divisible;
    bool both_divisible;
    std::string timestamp;
};

struct Statistics {
    int total;
    int with_dependency;
    int trivial_rejected;
    int nontrivial_found;
    int df_divisible_only;
    int dg_divisible_only;
    int both_divisible;
    int no_dependency;
};

struct StatRow {
    bool has_dependency;
    bool is_nontrivial;
    bool df_divisible;
    bool dg_divisible;
    bool both_divisible;
    int count;
};

struct ResultToSave {
    std::string f_str;
    std::string g_str;
    std::string f_hash;
    std::string g_hash;
    std::optional<std::string> q_str;
    bool is_trivial;
    DivisibilityResult divisibility;
};

class ResultCache {
public:
    explicit ResultCache(const std::string& db_path);
    ~ResultCache();
    
    ResultCache(const ResultCache&) = delete;
    ResultCache& operator=(const ResultCache&) = delete;
    
    std::optional<CachedResult> get_result(const GiNaC::ex& f, const GiNaC::ex& g);
    void save_result(const GiNaC::ex& f, const GiNaC::ex& g,
                     const std::optional<GiNaC::ex>& q,
                     const DivisibilityResult& divisibility,
                     bool is_trivial);
    void save_results_batch(const std::vector<ResultToSave>& results);
    Statistics get_statistics();
    std::vector<StatRow> get_detailed_statistics();
    std::vector<CachedResult> query_results(bool both_divisible = false, bool nontrivial_only = false, bool trivial_only = false);
    
    sqlite3* get_db() { return db_; }
    void close();

private:
    sqlite3* db_;
    
    void create_tables();
};