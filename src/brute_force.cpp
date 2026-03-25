#include "brute_force.h"
#include "cache.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <csignal>

BruteForceRunner* BruteForceRunner::instance_ = nullptr;

BruteForceRunner::BruteForceRunner(const Config& config)
    : config_(config),
      state_(BruteForceState::load(config.state_file)),
      shutdown_requested_(false),
      generator_(nullptr),
      worker_pool_(nullptr),
      checkpoint_manager_(nullptr) {
    
    if (config_.num_workers == 0) {
        config_.num_workers = sysconf(_SC_NPROCESSORS_ONLN);
    }
    config_.batch_size = 1000 * config_.num_workers;
    
    generator_ = std::make_unique<PolynomialGenerator>(config_, shutdown_requested_, state_.f_coeffs_state, state_.g_coeffs_state);
    worker_pool_ = std::make_unique<WorkerPool>(config_, shutdown_requested_);
    checkpoint_manager_ = std::make_unique<CheckpointManager>(state_, config_.state_file, config_.checkpoint_interval);
    
    instance_ = this;
    setup_signal_handlers();
}

void BruteForceRunner::setup_signal_handlers() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

void BruteForceRunner::signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        if (instance_) {
            instance_->shutdown_requested_.store(true);
        }
    }
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
    
    std::cout << "Starting to process polynomial pairs..." << std::endl;
    std::vector<std::pair<std::string, std::string>> batch;
    bool shutdown_msg_printed = false;
    
    generator_->generate_pairs([&](const GiNaC::ex& f, const GiNaC::ex& g) {
        if (shutdown_requested_.load()) {
            if (!shutdown_msg_printed) {
                shutdown_msg_printed = true;
            }
            return;
        }
        
        std::ostringstream f_oss, g_oss;
        f_oss << f;
        g_oss << g;
        batch.emplace_back(f_oss.str(), g_oss.str());
        
        if (batch.size() >= static_cast<size_t>(config_.batch_size)) {
            if (shutdown_requested_.load()) {
                if (!shutdown_msg_printed) {
                    shutdown_msg_printed = true;
                }
                return;
            }
            
            // Save state BEFORE processing to avoid losing data on crash
            auto batch_size = batch.size();
            state_.total_pairs_generated += batch_size;
            state_.total_pairs_checked += batch_size;
            state_.f_coeffs_state = generator_->get_f_coeffs();
            state_.g_coeffs_state = generator_->get_g_coeffs();
            state_.last_f_index = generator_->get_f_index();
            state_.last_g_index = generator_->get_g_index();
            
            checkpoint_manager_->increment_and_save_if_needed();
            
            // Now process the batch
            worker_pool_->process_batch(batch);
            batch.clear();

            if (shutdown_requested_.load()) {
                if (!shutdown_msg_printed) {
                    shutdown_msg_printed = true;
                }
                return;
            }
        }
    });
    
    if (!batch.empty() && !shutdown_requested_.load()) {
        auto batch_size = batch.size();
        worker_pool_->process_batch(batch);
        state_.total_pairs_generated += batch_size;
        state_.total_pairs_checked += batch_size;
        state_.f_coeffs_state = generator_->get_f_coeffs();
        state_.g_coeffs_state = generator_->get_g_coeffs();
        state_.last_f_index = generator_->get_f_index();
        state_.last_g_index = generator_->get_g_index();
    }

    checkpoint_manager_->force_save();
    
    std::cout << "============================================================" << std::endl;
    std::cout << "FINAL STATISTICS" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << state_.get_summary() << std::endl;
    std::cout << std::endl;
    std::cout << "Note: Results are saved in worker database files." << std::endl;
    std::cout << "Use Python scripts to merge and query results." << std::endl;
}
