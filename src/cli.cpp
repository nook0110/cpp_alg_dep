#include "cli.h"
#include "brute_force.h"
#include "manual_check.h"
#include "cache.h"
#include <iostream>
#include <cstring>
#include <sstream>

int CLI::run(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }
    
    std::string mode = argv[1];
    
    if (mode == "brute") {
        Config config;
        
        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--max-degree-f" && i + 1 < argc) {
                config.max_degree_f = std::stoi(argv[++i]);
            } else if (arg == "--max-degree-g" && i + 1 < argc) {
                config.max_degree_g = std::stoi(argv[++i]);
            } else if (arg == "--max-degree-q" && i + 1 < argc) {
                config.max_degree_q = std::stoi(argv[++i]);
            } else if (arg == "--coeff-min" && i + 1 < argc) {
                config.coeff_min = std::stoi(argv[++i]);
            } else if (arg == "--coeff-max" && i + 1 < argc) {
                config.coeff_max = std::stoi(argv[++i]);
            } else if (arg == "--strategy" && i + 1 < argc) {
                std::string strategy = argv[++i];
                if (strategy == "degree_first") {
                    config.enum_strategy = EnumStrategy::DegreeFirst;
                }
            } else if (arg == "--checkpoint-interval" && i + 1 < argc) {
                config.checkpoint_interval = std::stoi(argv[++i]);
            } else if (arg == "--workers" && i + 1 < argc) {
                config.num_workers = std::stoi(argv[++i]);
            }
        }
        
        config.validate();
        run_brute_force(config);
        return 0;
    }
    else if (mode == "check") {
        if (argc < 4) {
            std::cerr << "Usage: " << argv[0] << " check F G [OPTIONS]" << std::endl;
            return 1;
        }
        
        std::string f_str = argv[2];
        std::string g_str = argv[3];
        
        Config config;
        for (int i = 4; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--max-degree-q" && i + 1 < argc) {
                config.max_degree_q = std::stoi(argv[++i]);
            }
        }
        
        config.validate();
        run_manual_check(f_str, g_str, config);
        return 0;
    }
    else if (mode == "stats") {
        show_statistics("data/results.db");
        return 0;
    }
    else if (mode == "query") {
        bool both_divisible = false;
        bool nontrivial_only = false;
        bool trivial_only = false;
        int limit = 20;
        
        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--both-divisible") {
                both_divisible = true;
            } else if (arg == "--nontrivial") {
                nontrivial_only = true;
            } else if (arg == "--trivial") {
                trivial_only = true;
            } else if (arg == "--limit" && i + 1 < argc) {
                limit = std::stoi(argv[++i]);
            }
        }
        
        query_results("data/results.db", both_divisible, nontrivial_only, trivial_only, limit);
        return 0;
    }
    else {
        print_help();
        return 1;
    }
}

void CLI::run_brute_force(const Config& config) {
    BruteForceRunner runner(config);
    runner.run();
}

void CLI::run_manual_check(const std::string& f_str, const std::string& g_str, const Config& config) {
    ManualChecker checker(config);
    checker.check_pair(f_str, g_str);
}

void CLI::show_statistics(const std::string& db_path) {
    ResultCache cache(db_path);
    auto stats = cache.get_statistics();
    auto detailed = cache.get_detailed_statistics();
    
    std::cout << "====================================================================================================" << std::endl;
    std::cout << "STATISTICS: ALL COMBINATIONS" << std::endl;
    std::cout << "====================================================================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Total pairs checked: " << stats.total << std::endl;
    std::cout << std::endl;
    
    std::cout << "+--------------+-----------------+-----------------+-----------------+-------------+---------+-----------+" << std::endl;
    std::cout << "| Dependency   | Non-trivial     | dq/df : dq/dx   | dq/dg : dq/dx   | Both        |   Count | Percent   |" << std::endl;
    std::cout << "| Found        | (x^2/x*u/x*v)   |                 |                 | Divisible   |         |           |" << std::endl;
    std::cout << "+==============+=================+=================+=================+=============+=========+===========+" << std::endl;
    
    for (const auto& row : detailed) {
        auto dep_status = row.has_dependency ? "+" : "-";
        auto nontrivial_status = row.is_nontrivial ? "+" : "-";
        auto df_status = row.df_divisible ? "+" : "-";
        auto dg_status = row.dg_divisible ? "+" : "-";
        auto both_status = row.both_divisible ? "+" : "-";
        
        double pct = stats.total > 0 ? (100.0 * row.count / stats.total) : 0.0;
        
        char line[256];
        snprintf(line, sizeof(line), "| %-12s | %-15s | %-15s | %-15s | %-11s | %7d | %8.2f%% |",
                 dep_status, nontrivial_status, df_status, dg_status, both_status, row.count, pct);
        std::cout << line << std::endl;
        std::cout << "+--------------+-----------------+-----------------+-----------------+-------------+---------+-----------+" << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Legend:" << std::endl;
    std::cout << "  + = Passed" << std::endl;
    std::cout << "  - = Failed" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: Non-trivial means x appears as x^2 or x*u or x*v (not just linear like 4x-5)" << std::endl;
}

void CLI::query_results(const std::string& db_path, bool both_divisible, bool nontrivial_only, bool trivial_only, int limit) {
    ResultCache cache(db_path);
    auto results = cache.query_results(both_divisible, nontrivial_only, trivial_only);
    
    std::cout << "============================================================" << std::endl;
    std::cout << "QUERY RESULTS" << std::endl;
    std::cout << "============================================================" << std::endl;
    
    if (trivial_only) {
        std::cout << "Showing TRIVIAL dependencies only" << std::endl;
    } else if (both_divisible && nontrivial_only) {
        std::cout << "Showing NON-TRIVIAL pairs where both divisibility conditions hold" << std::endl;
    } else if (both_divisible) {
        std::cout << "Showing pairs where both divisibility conditions hold" << std::endl;
    } else if (nontrivial_only) {
        std::cout << "Showing non-trivial results only" << std::endl;
    } else {
        std::cout << "Showing all results" << std::endl;
    }
    
    std::cout << "Limit: " << limit << std::endl;
    std::cout << std::endl;
    
    if (results.empty()) {
        std::cout << "No results found." << std::endl;
        return;
    }
    
    int count = 0;
    for (const auto& result : results) {
        if (count >= limit) break;
        count++;
        
        std::cout << "Result #" << count << std::endl;
        std::cout << "  f = " << result.f_poly << std::endl;
        std::cout << "  g = " << result.g_poly << std::endl;
        
        if (result.q_poly.has_value()) {
            std::cout << "  q = " << result.q_poly.value() << std::endl;
            std::cout << "  Trivial: " << (result.is_trivial ? "YES" : "NO") << std::endl;
            std::cout << "  dq/du : dq/dx = " << (result.df_divisible ? "true" : "false") << std::endl;
            std::cout << "  dq/dv : dq/dx = " << (result.dg_divisible ? "true" : "false") << std::endl;
        } else {
            std::cout << "  No dependency found" << std::endl;
        }
        
        std::cout << "  Timestamp: " << result.timestamp << std::endl;
        std::cout << std::endl;
    }
    
    if (results.size() > static_cast<size_t>(limit)) {
        std::cout << "... and " << (results.size() - limit) << " more results" << std::endl;
    }
}

void CLI::print_help() {
    std::cout << "Polynomial Algebraic Dependency Checker (C++)" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  poly_dep_check brute [OPTIONS]" << std::endl;
    std::cout << "  poly_dep_check check F G [OPTIONS]" << std::endl;
    std::cout << "  poly_dep_check stats" << std::endl;
    std::cout << "  poly_dep_check query [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  brute    Run brute force search" << std::endl;
    std::cout << "  check    Check specific polynomial pair" << std::endl;
    std::cout << "  stats    Show statistics" << std::endl;
    std::cout << "  query    Query results from cache" << std::endl;
    std::cout << std::endl;
    std::cout << "Brute Force Options:" << std::endl;
    std::cout << "  --max-degree-f N         Maximum degree for f (default: 2)" << std::endl;
    std::cout << "  --max-degree-g N         Maximum degree for g (default: 2)" << std::endl;
    std::cout << "  --max-degree-q N         Maximum degree for q (default: 3)" << std::endl;
    std::cout << "  --coeff-min N            Minimum coefficient (default: -2)" << std::endl;
    std::cout << "  --coeff-max N            Maximum coefficient (default: 2)" << std::endl;
    std::cout << "  --strategy S             Strategy: lexicographic|degree_first" << std::endl;
    std::cout << "  --checkpoint-interval N  Save every N pairs (default: 10)" << std::endl;
    std::cout << "  --workers N              Number of workers (default: 4)" << std::endl;
    std::cout << "  --resume                 Resume from checkpoint" << std::endl;
    std::cout << std::endl;
    std::cout << "Check Options:" << std::endl;
    std::cout << "  --max-degree-q N         Maximum degree for q (default: 3)" << std::endl;
    std::cout << std::endl;
    std::cout << "Query Options:" << std::endl;
    std::cout << "  --both-divisible         Show only pairs with both conditions" << std::endl;
    std::cout << "  --nontrivial             Show only non-trivial dependencies" << std::endl;
    std::cout << "  --trivial                Show only trivial dependencies" << std::endl;
    std::cout << "  --limit N                Maximum results to show (default: 20)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  poly_dep_check check \"x^2 + y^2\" \"x*y\"" << std::endl;
    std::cout << "  poly_dep_check brute --max-degree-f 3 --workers 8" << std::endl;
    std::cout << "  poly_dep_check query --both-divisible --nontrivial --limit 10" << std::endl;
}