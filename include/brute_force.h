#pragma once

#include "config.h"
#include "generator.h"
#include "state.h"
#include "symbols.h"
#include "cache.h"
#include <memory>
#include <string>
#include <atomic>

class BruteForceRunner {
public:
    explicit BruteForceRunner(const Config& config);
    
    void run();

private:
    Config config_;
    std::unique_ptr<PolynomialGenerator> generator_;
    BruteForceState state_;
    std::atomic<int> checkpoint_counter_;
    
    void process_batch(const std::vector<std::pair<std::string, std::string>>& pairs_batch);
    void process_batch_in_worker(const std::vector<std::pair<std::string, std::string>>& pairs_batch, size_t start_idx, size_t end_idx);
    ResultToSave process_single_pair(const std::string& f_str, const std::string& g_str, const ThreadLocalSymbols& symbols);
};