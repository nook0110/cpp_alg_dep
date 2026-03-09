#include "brute_force.h"
#include "dependency_finder.h"
#include "divisibility.h"
#include "cache.h"
#include "polynomial.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>

BruteForceRunner::BruteForceRunner(const Config& config)
    : config_(config),
      generator_(std::make_unique<PolynomialGenerator>(config)),
      state_(BruteForceState::load(config.state_file)),
      checkpoint_counter_(0) {
    if (config_.num_workers == 0) {
        config_.num_workers = sysconf(_SC_NPROCESSORS_ONLN);
    }
    config_.batch_size = 1000 * config_.num_workers;
}

void BruteForceRunner::run() {
    std::cout << "Starting brute force search..." << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Max degree f: " << config_.max_degree_f << std::endl;
    std::cout << "  Max degree g: " << config_.max_degree_g << std::endl;
    std::cout << "  Max degree q: " << config_.max_degree_q << std::endl;
    std::cout << "  Coefficient range: [" << config_.coeff_min << ", " << config_.coeff_max << "]" << std::endl;
    std::cout << "  Workers: " << config_.num_workers << std::endl;
    std::cout << "  Batch size: " << config_.batch_size << std::endl;
    
    if (state_.total_pairs_checked > 0) {
        std::cout << "Resuming from previous state:" << std::endl;
        std::cout << state_.get_summary() << std::endl;
    }
    
    try {
        std::cout << "Starting to process polynomial pairs..." << std::endl;
        std::vector<std::pair<std::string, std::string>> batch;
        
        generator_->generate_pairs([&](const GiNaC::ex& f, const GiNaC::ex& g) {
            std::ostringstream f_oss, g_oss;
            f_oss << f;
            g_oss << g;
            batch.emplace_back(f_oss.str(), g_oss.str());
            
            if (batch.size() >= static_cast<size_t>(config_.batch_size)) {
                process_batch(batch);
                batch.clear();
            }
        });
        
        if (!batch.empty()) {
            process_batch(batch);
        }
    } catch (const std::exception& e) {
        std::cout << "Interrupted: " << e.what() << std::endl;
        state_.save(config_.state_file);
        std::cout << "State saved. You can resume later with --resume flag." << std::endl;
    }
    
    state_.save(config_.state_file);
    
    std::cout << "============================================================" << std::endl;
    std::cout << "FINAL STATISTICS" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << state_.get_summary() << std::endl;
    
    ResultCache cache(config_.cache_file);
    auto stats = cache.get_statistics();
    std::cout << "Detailed Statistics:" << std::endl;
    std::cout << "  Total pairs checked: " << stats.total << std::endl;
    std::cout << "  Dependencies found: " << stats.with_dependency << std::endl;
    std::cout << "    - Trivial (rejected): " << stats.trivial_rejected << std::endl;
    std::cout << "    - Non-trivial (kept): " << stats.nontrivial_found << std::endl;
    std::cout << "  No dependency found: " << stats.no_dependency << std::endl;
    std::cout << "Divisibility Results (for non-trivial dependencies):" << std::endl;
    std::cout << "  dq/df : dq/dx only: " << stats.df_divisible_only << std::endl;
    std::cout << "  dq/dg : dq/dx only: " << stats.dg_divisible_only << std::endl;
    std::cout << "  Both conditions satisfied: " << stats.both_divisible << std::endl;
}

ResultToSave BruteForceRunner::process_single_pair(const std::string& f_str, const std::string& g_str, const ThreadLocalSymbols& symbols) {
    static thread_local GiNaC::symtab table;
    static thread_local bool table_initialized = false;
    
    if (!table_initialized) {
        table["x"] = symbols.x;
        table["y"] = symbols.y;
        table_initialized = true;
    }
    
    GiNaC::ex f, g;
    try {
        GiNaC::parser reader(table);
        f = reader(f_str);
        g = reader(g_str);
    } catch (...) {
        return {};
    }
    
    DependencyFinder finder(config_, symbols);
    DivisibilityChecker checker(symbols);
    
    auto [q, was_trivial] = finder.find_dependency(f, g);
    DivisibilityResult divisibility = {false, false, false, false};
    
    if (q.has_value()) {
        divisibility = checker.check_conditions(q.value(), f, g);
    }
    
    ResultToSave result;
    result.f_str = f_str;
    result.g_str = g_str;
    result.f_hash = PolynomialOps::poly_hash(f);
    result.g_hash = PolynomialOps::poly_hash(g);
    
    if (q.has_value()) {
        std::ostringstream q_oss;
        q_oss << q.value();
        result.q_str = q_oss.str();
    }
    
    result.is_trivial = was_trivial;
    result.divisibility = divisibility;
    
    state_.update_progress(0, 0, q.has_value());
    auto counter = checkpoint_counter_.fetch_add(1) + 1;
    
    if (counter >= config_.checkpoint_interval) {
        if (checkpoint_counter_.compare_exchange_strong(counter, 0)) {
            state_.save(config_.state_file);
        }
    }
    
    return result;
}

void BruteForceRunner::process_batch_in_worker(const std::vector<std::pair<std::string, std::string>>& pairs_batch, size_t start_idx, size_t end_idx) {
    ThreadLocalSymbols symbols;
    std::vector<ResultToSave> results;
    
    std::string worker_db = config_.cache_file + ".worker_" + std::to_string(getpid());
    
    for (size_t i = start_idx; i < end_idx; ++i) {
        const auto& [f_str, g_str] = pairs_batch[i];
        auto result = process_single_pair(f_str, g_str, symbols);
        if (!result.f_str.empty()) {
            results.push_back(std::move(result));
        }
    }
    
    if (!results.empty()) {
        ResultCache cache(worker_db);
        cache.save_results_batch(results);
    }
}

void BruteForceRunner::process_batch(const std::vector<std::pair<std::string, std::string>>& pairs_batch) {
    const auto num_workers = static_cast<size_t>(config_.num_workers);
    std::vector<pid_t> child_pids;
    
    const auto pairs_per_worker = (pairs_batch.size() + num_workers - 1) / num_workers;
    
    for (size_t w = 0; w < num_workers; ++w) {
        const auto start_idx = w * pairs_per_worker;
        const auto end_idx = std::min(start_idx + pairs_per_worker, pairs_batch.size());
        
        if (start_idx >= pairs_batch.size()) {
            break;
        }
        
        pid_t pid = fork();
        
        if (pid == 0) {
            process_batch_in_worker(pairs_batch, start_idx, end_idx);
            _exit(0);
        } else if (pid > 0) {
            child_pids.push_back(pid);
        } else {
            std::cerr << "Fork failed for worker " << w << std::endl;
        }
    }
    
    for (pid_t pid : child_pids) {
        int status;
        waitpid(pid, &status, 0);
    }
}
