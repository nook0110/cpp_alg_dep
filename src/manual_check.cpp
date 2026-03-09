#include "manual_check.h"
#include "polynomial.h"
#include <iostream>

ManualChecker::ManualChecker(const Config& config)
    : config_(config),
      symbols_(),
      finder_(std::make_unique<DependencyFinder>(config, symbols_)),
      checker_(std::make_unique<DivisibilityChecker>(symbols_)),
      cache_(std::make_unique<ResultCache>(config.cache_file)) {}

ManualChecker::~ManualChecker() = default;

void ManualChecker::check_pair(const std::string& f_str, const std::string& g_str) {
    std::cout << "============================================================" << std::endl;
    std::cout << "MANUAL CHECK" << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << "f = " << f_str << std::endl;
    std::cout << "g = " << g_str << std::endl;
    std::cout << std::endl;
    
    GiNaC::ex f, g;
    try {
        f = PolynomialOps::parse_polynomial(f_str);
        g = PolynomialOps::parse_polynomial(g_str);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing polynomials: " << e.what() << std::endl;
        return;
    }
    
    std::cout << "Parsed f = " << f << std::endl;
    std::cout << "Parsed g = " << g << std::endl;
    std::cout << std::endl;
    
    auto cached = cache_->get_result(f, g);
    if (cached.has_value()) {
        std::cout << "Result found in cache:" << std::endl;
        print_result(cached.value());
        return;
    }
    
    std::cout << "Searching for dependency q(x, u, v) where q(x, f, g) = 0..." << std::endl;
    std::cout << "Max degree for q: " << config_.max_degree_q << std::endl;
    std::cout << std::endl;
    
    auto [q, was_trivial] = finder_->find_dependency(f, g);
    
    if (!q.has_value()) {
        std::cout << "No dependency found within degree bounds." << std::endl;
        std::cout << std::endl;
        cache_->save_result(f, g, std::nullopt, {false, false, false}, false);
        return;
    }
    
    if (was_trivial) {
        std::cout << "- Found TRIVIAL dependency (only linear x):" << std::endl;
    } else {
        std::cout << "+ Found NON-TRIVIAL dependency:" << std::endl;
    }
    std::cout << "  q = " << q.value() << std::endl;
    std::cout << std::endl;
    
    std::cout << "Checking divisibility conditions..." << std::endl;
    auto divisibility = checker_->check_conditions(q.value(), f, g);
    
    std::cout << "  dq/du : dq/dx = " << (divisibility.df_divisible ? "true" : "false") << std::endl;
    std::cout << "  dq/dv : dq/dx = " << (divisibility.dg_divisible ? "true" : "false") << std::endl;
    std::cout << std::endl;
    
    if (divisibility.needs_review) {
        std::cout << "⚠ WARNING: All derivatives are zero (0/0 case) - flagged for review" << std::endl;
    }
    
    if (divisibility.both_divisible) {
        std::cout << "+ Both divisibility conditions satisfied!" << std::endl;
    } else {
        std::cout << "- Not all divisibility conditions satisfied." << std::endl;
    }
    
    std::cout << std::endl;
    
    cache_->save_result(f, g, q, divisibility, was_trivial, divisibility.needs_review);
    std::cout << "Result saved to cache." << std::endl;
}

void ManualChecker::print_result(const CachedResult& result) {
    std::cout << "  f = " << result.f_poly << std::endl;
    std::cout << "  g = " << result.g_poly << std::endl;
    
    if (result.is_trivial) {
        std::cout << "  - Dependency found but rejected as trivial (only linear x)" << std::endl;
    } else if (result.q_poly.has_value()) {
        std::cout << "  q = " << result.q_poly.value() << std::endl;
        std::cout << "  dq/du : dq/dx = " << (result.df_divisible ? "true" : "false") << std::endl;
        std::cout << "  dq/dv : dq/dx = " << (result.dg_divisible ? "true" : "false") << std::endl;
        
        if (result.both_divisible) {
            std::cout << "  + Both conditions satisfied" << std::endl;
        } else {
            std::cout << "  - Not all conditions satisfied" << std::endl;
        }
    } else {
        std::cout << "  No dependency found" << std::endl;
    }
    
    std::cout << "  Cached at: " << result.timestamp << std::endl;
}