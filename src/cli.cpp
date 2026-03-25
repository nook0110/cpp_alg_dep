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

void CLI::print_help() {
    std::cout << "Polynomial Algebraic Dependency Checker (C++)" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  poly_dep_check brute [OPTIONS]" << std::endl;
    std::cout << "  poly_dep_check check F G [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  brute    Run brute force search" << std::endl;
    std::cout << "  check    Check specific polynomial pair" << std::endl;
    std::cout << std::endl;
    std::cout << "Brute Force Options:" << std::endl;
    std::cout << "  --max-degree-f N         Maximum degree for f (default: 2)" << std::endl;
    std::cout << "  --max-degree-g N         Maximum degree for g (default: 2)" << std::endl;
    std::cout << "  --max-degree-q N         Maximum degree for q (default: 3)" << std::endl;
    std::cout << "  --coeff-min N            Minimum coefficient (default: -2)" << std::endl;
    std::cout << "  --coeff-max N            Maximum coefficient (default: 2)" << std::endl;
    std::cout << "  --strategy S             Strategy: lexicographic|degree_first" << std::endl;
    std::cout << "  --checkpoint-interval N  Save every N batches (default: 1000)" << std::endl;
    std::cout << "  --workers N              Number of workers (default: auto)" << std::endl;
    std::cout << std::endl;
    std::cout << "Check Options:" << std::endl;
    std::cout << "  --max-degree-q N         Maximum degree for q (default: 3)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  poly_dep_check check \"x^2 + y^2\" \"x*y\"" << std::endl;
    std::cout << "  poly_dep_check brute --max-degree-f 3 --workers 8" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: Use Python scripts for statistics and querying results." << std::endl;
}