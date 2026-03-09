#pragma once

#include "config.h"
#include "generator.h"
#include "state.h"
#include <memory>

class BruteForceRunner {
public:
    explicit BruteForceRunner(const Config& config);
    
    void run();

private:
    Config config_;
    std::unique_ptr<PolynomialGenerator> generator_;
    BruteForceState state_;
    int checkpoint_counter_;
    
    void process_batch(const std::vector<std::pair<GiNaC::ex, GiNaC::ex>>& pairs_batch);
};