#pragma once

#include "config.h"
#include "generator.h"
#include "state.h"
#include "worker_pool.h"
#include "checkpoint.h"
#include <memory>
#include <string>
#include <atomic>

class BruteForceRunner {
public:
    explicit BruteForceRunner(const Config& config);
    
    void run();

private:
    Config config_;
    BruteForceState state_;
    std::atomic<bool> shutdown_requested_;
    std::unique_ptr<PolynomialGenerator> generator_;
    std::unique_ptr<WorkerPool> worker_pool_;
    std::unique_ptr<CheckpointManager> checkpoint_manager_;
    
    void setup_signal_handlers();
    static void signal_handler(int signal);
    
    static BruteForceRunner* instance_;
};