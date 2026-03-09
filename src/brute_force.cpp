#include "brute_force.h"
#include "dependency_finder.h"
#include "divisibility.h"
#include "cache.h"
#include <iostream>
#include <thread>
#include <mutex>

BruteForceRunner::BruteForceRunner(const Config& config)
    : config_(config),
      generator_(std::make_unique<PolynomialGenerator>(config)),
      state_(BruteForceState::load(config.state_file)),
      checkpoint_counter_(0) {}

void BruteForceRunner::run() {
    std::cout << "Starting brute force search..." << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Max degree f: " << config_.max_degree_f << std::endl;
    std::cout << "  Max degree g: " << config_.max_degree_g << std::endl;
    std::cout << "  Max degree q: " << config_.max_degree_q << std::endl;
    std::cout << "  Coefficient range: [" << config_.coeff_min << ", " << config_.coeff_max << "]" << std::endl;
    std::cout << "  Workers: " << config_.num_workers << std::endl;
    
    if (state_.total_pairs_checked > 0) {
        std::cout << "Resuming from previous state:" << std::endl;
        std::cout << state_.get_summary() << std::endl;
    }
    
    try {
        std::cout << "Starting to process polynomial pairs..." << std::endl;
        std::vector<std::pair<GiNaC::ex, GiNaC::ex>> batch;
        
        generator_->generate_pairs([&](const GiNaC::ex& f, const GiNaC::ex& g) {
            batch.emplace_back(f, g);
            
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

void BruteForceRunner::process_batch(const std::vector<std::pair<GiNaC::ex, GiNaC::ex>>& pairs_batch) {
    ResultCache cache(config_.cache_file);
    DependencyFinder finder(config_);
    DivisibilityChecker checker;
    
    for (const auto& [f, g] : pairs_batch) {
        auto cached = cache.get_result(f, g);
        if (cached.has_value()) {
            std::cout << "[CACHED] f=" << f << ", g=" << g << std::endl;
            continue;
        }
        
        std::cout << "[CHECKING] f=" << f << ", g=" << g << std::endl;
        
        auto [q, was_trivial] = finder.find_dependency(f, g);
        DivisibilityResult divisibility = {false, false, false};
        
        if (q.has_value()) {
            if (was_trivial) {
                std::cout << "  Found TRIVIAL dependency (only linear x): q=" << q.value() << std::endl;
            } else {
                std::cout << "  Found NON-TRIVIAL dependency: q=" << q.value() << std::endl;
            }
            divisibility = checker.check_conditions(q.value(), f, g);
            std::cout << "  dq/df : dq/dx = " << (divisibility.df_divisible ? "true" : "false") << std::endl;
            std::cout << "  dq/dg : dq/dx = " << (divisibility.dg_divisible ? "true" : "false") << std::endl;
        } else {
            std::cout << "  No dependency found" << std::endl;
        }
        
        cache.save_result(f, g, q, divisibility, was_trivial);
        
        state_.update_progress(0, 0, q.has_value());
        checkpoint_counter_++;
        
        if (checkpoint_counter_ >= config_.checkpoint_interval) {
            state_.save(config_.state_file);
            checkpoint_counter_ = 0;
            std::cout << "[CHECKPOINT] " << state_.total_pairs_checked << " pairs checked" << std::endl;
        }
    }
}