#pragma once

#include "config.h"
#include "symbols.h"
#include "cache.h"
#include <vector>
#include <string>
#include <utility>
#include <optional>
#include <atomic>

struct WorkerPool {
    const Config& config;
    std::atomic<bool>& shutdown_requested;

    WorkerPool(const Config& cfg, std::atomic<bool>& shutdown)
        : config(cfg), shutdown_requested(shutdown) {}

    void process_batch(const std::vector<std::pair<std::string, std::string>>& pairs_batch);

private:
    void process_batch_in_worker(
        const std::vector<std::pair<std::string, std::string>>& pairs_batch,
        size_t start_idx,
        size_t end_idx,
        size_t worker_id);

    std::optional<ResultToSave> process_single_pair(
        const std::string& f_str,
        const std::string& g_str,
        const ThreadLocalSymbols& symbols);
};
