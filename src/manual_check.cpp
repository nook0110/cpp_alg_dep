#include "manual_check.h"
#include "polynomial.h"
#include <iostream>
#include <sstream>

ManualChecker::ManualChecker(const Config& config)
    : config_(config),
      symbols_(),
      finder_(std::make_unique<DependencyFinder>(config, symbols_)),
      checker_(std::make_unique<DivisibilityChecker>(symbols_)),
      cache_(std::make_unique<ResultCache>(config.cache_file)) {}

ManualChecker::~ManualChecker() = default;

void ManualChecker::check_pair(const std::string& f_str, const std::string& g_str) {
    std::cout << "============================================================\n";
    std::cout << "MANUAL CHECK\n";
    std::cout << "============================================================\n";
    std::cout << "f = " << f_str << "\n";
    std::cout << "g = " << g_str << "\n\n";

    GiNaC::ex f, g;
    try {
        f = poly::parse_polynomial(f_str, symbols_.x, symbols_.y);
        g = poly::parse_polynomial(g_str, symbols_.x, symbols_.y);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing polynomials: " << e.what() << "\n";
        return;
    }

    std::cout << "Parsed f = " << f << "\n";
    std::cout << "Parsed g = " << g << "\n\n";

    std::cout << "Searching for dependency q(x, u, v) where q(x, f, g) = 0...\n";
    std::cout << "Max degree for q: " << config_.max_degree_q << "\n\n";

    auto [q, was_trivial] = finder_->find_dependency(f, g);

    if (!q.has_value()) {
        std::cout << "No dependency found within degree bounds.\n\n";
        CachedResult result;
        result.f_poly = f_str;
        result.g_poly = g_str;
        cache_->save_result(result);
        return;
    }

    if (was_trivial) {
        std::cout << "- Found TRIVIAL dependency (only linear x):\n";
    } else {
        std::cout << "+ Found NON-TRIVIAL dependency:\n";
    }

    std::ostringstream q_oss;
    q_oss << q.value();
    const auto q_str = q_oss.str();
    std::cout << "  q = " << q_str << "\n\n";

    std::cout << "Checking divisibility conditions...\n";
    const auto divisibility = checker_->check_conditions(q.value(), f, g);

    if (divisibility.min_poly.has_value()) {
        std::cout << "  ℹ GCD min-poly extracted: q_min = " << divisibility.min_poly.value() << "\n";
        std::cout << "  Checking divisibility with min poly...\n";
    }

    std::cout << "  dq/du : dq/dx = " << (divisibility.df_divisible ? "true" : "false") << "\n";
    std::cout << "  dq/dv : dq/dx = " << (divisibility.dg_divisible ? "true" : "false") << "\n\n";

    if (divisibility.needs_review && !divisibility.used_min_poly) {
        std::cout << "⚠ WARNING: All derivatives are zero — min_poly extraction failed\n";
    }

    if (divisibility.both_divisible) {
        std::cout << "+ Both divisibility conditions satisfied!\n";
    } else {
        std::cout << "- Not all divisibility conditions satisfied.\n";
    }

    std::cout << "\n";

    CachedResult result;
    result.f_poly = f_str;
    result.g_poly = g_str;
    result.q_poly = q_str;

    if (divisibility.min_poly.has_value()) {
        std::ostringstream mp_oss;
        mp_oss << divisibility.min_poly.value();
        result.q_min = mp_oss.str();
    }

    if (divisibility.needs_review && !divisibility.used_min_poly) {
        result.error = "all_zero: min_poly extraction failed";
    }

    cache_->save_result(result);
    std::cout << "Result saved to cache.\n";
}
