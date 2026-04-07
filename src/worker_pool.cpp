#include "worker_pool.h"
#include "dependency_finder.h"
#include "divisibility.h"
#include "polynomial.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>

void WorkerPool::process_batch(const std::vector<std::pair<std::string, std::string>>& pairs_batch) {
    const auto num_workers = static_cast<size_t>(config.num_workers);
    std::vector<pid_t> child_pids;

    const auto pairs_per_worker = (pairs_batch.size() + num_workers - 1) / num_workers;

    for (size_t w = 0; w < num_workers; ++w) {
        const auto start_idx = w * pairs_per_worker;
        const auto end_idx = std::min(start_idx + pairs_per_worker, pairs_batch.size());

        if (start_idx >= pairs_batch.size()) break;

        pid_t pid = fork();

        if (pid == 0) {
            process_batch_in_worker(pairs_batch, start_idx, end_idx, w);
            _exit(0);
        } else if (pid > 0) {
            child_pids.push_back(pid);
        }
    }

    for (size_t i = 0; i < child_pids.size(); ++i) {
        int status;
        waitpid(child_pids[i], &status, 0);
    }
}

void WorkerPool::process_batch_in_worker(
    const std::vector<std::pair<std::string, std::string>>& pairs_batch,
    size_t start_idx,
    size_t end_idx,
    size_t worker_id) {

    ThreadLocalSymbols symbols;
    std::vector<ResultToSave> results;

    const auto worker_db = config.cache_file + ".worker_" + std::to_string(worker_id);

    for (size_t i = start_idx; i < end_idx; ++i) {
        const auto& [f_str, g_str] = pairs_batch[i];
        auto result = process_single_pair(f_str, g_str, symbols);
        if (result.has_value()) {
            results.push_back(std::move(result.value()));
        }
    }

    if (!results.empty()) {
        ResultCache cache(worker_db);
        cache.save_results_batch(results);
    }
}

std::optional<ResultToSave> WorkerPool::process_single_pair(
    const std::string& f_str,
    const std::string& g_str,
    const ThreadLocalSymbols& symbols) {

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
        return std::nullopt;
    }

    DependencyFinder finder(config, symbols);
    DivisibilityChecker checker(symbols);

    auto [q, was_trivial] = finder.find_dependency(f, g);
    DivisibilityResult divisibility = {false, false, false, false, false, std::nullopt};

    if (q.has_value()) {
        divisibility = checker.check_conditions(q.value(), f, g);
    }

    if (!divisibility.both_divisible) {
        return std::nullopt;
    }

    const auto& effective_q = divisibility.min_poly.has_value()
        ? divisibility.min_poly.value()
        : (q.has_value() ? q.value() : GiNaC::ex(0));

    if (!is_nontrivial_in_x(effective_q, symbols)) {
        return std::nullopt;
    }

    ResultToSave result;
    result.f_str = f_str;
    result.g_str = g_str;

    if (q.has_value()) {
        std::ostringstream q_oss;
        q_oss << q.value();
        result.q_str = q_oss.str();
    }

    if (divisibility.min_poly.has_value()) {
        std::ostringstream mp_oss;
        mp_oss << divisibility.min_poly.value();
        result.q_min_str = mp_oss.str();
    }

    if (divisibility.needs_review && !divisibility.used_min_poly) {
        result.error = "all_zero: min_poly extraction failed";
    }

    return result;
}
