#include "generator.h"
#include <iostream>
#include <cmath>

PolynomialGenerator::PolynomialGenerator(const Config& config, std::atomic<bool>& shutdown, const std::vector<int>& f_resume, const std::vector<int>& g_resume)
    : config_(config), shutdown_requested_(shutdown), x_("x"), y_("y"), f_resume_(f_resume), g_resume_(g_resume), f_coeffs_(f_resume), g_coeffs_(g_resume) {}

std::generator<GiNaC::ex> PolynomialGenerator::generate_polynomials_lazy(int max_degree, const std::vector<int>& resume_coeffs, PolyType type) {
    const int coeff_min = config_.coeff_min;
    const int coeff_max = config_.coeff_max;
    
    std::vector<std::pair<int, int>> monomials;
    for (int deg = 0; deg <= max_degree; ++deg) {
        for (int i = 0; i <= deg; ++i) {
            int j = deg - i;
            monomials.push_back({i, j});
        }
    }
    
    const int num_terms = monomials.size();
    std::vector<int> coeffs(num_terms, coeff_min);
    
    if (!resume_coeffs.empty() && resume_coeffs.size() == static_cast<size_t>(num_terms)) {
        coeffs = resume_coeffs;
    }
    
    size_t iteration_count = 0;
    while (true) {
        if (shutdown_requested_.load()) {
            break;
        }
        
        GiNaC::ex poly = 0;
        
        for (size_t idx = 0; idx < monomials.size(); ++idx) {
            if (coeffs[idx] != 0) {
                auto [i, j] = monomials[idx];
                poly += coeffs[idx] * GiNaC::pow(x_, i) * GiNaC::pow(y_, j);
            }
        }
        
        if (!poly.is_zero() && !GiNaC::is_a<GiNaC::numeric>(poly)) {
            // Save coefficients BEFORE yielding
            if (type == PolyType::F) {
                f_coeffs_ = coeffs;
            } else {
                g_coeffs_ = coeffs;
            }
            
            iteration_count++;
            co_yield poly;
            
            if (shutdown_requested_.load()) {
                break;
            }
        }
        
        int pos = 0;
        while (pos < num_terms && coeffs[pos] == coeff_max) {
            coeffs[pos] = coeff_min;
            pos++;
        }
        
        if (pos >= num_terms) {
            break;
        }
        
        coeffs[pos]++;
    }
}

bool PolynomialGenerator::should_skip(const GiNaC::ex& f, const GiNaC::ex& g) const {
    if (f.is_zero() || g.is_zero()) {
        return true;
    }
    
    if (GiNaC::is_a<GiNaC::numeric>(f) || GiNaC::is_a<GiNaC::numeric>(g)) {
        return true;
    }
    
    if (f.degree(x_) == 0 && f.degree(y_) == 0) {
        return true;
    }
    if (g.degree(x_) == 0 && g.degree(y_) == 0) {
        return true;
    }
    
    return false;
}

size_t PolynomialGenerator::count_total_pairs() const {
    auto count_polys = [this](int max_degree) -> size_t {
        int num_monomials = 0;
        for (int deg = 0; deg <= max_degree; ++deg) {
            num_monomials += (deg + 1);
        }
        
        const int range = config_.coeff_max - config_.coeff_min + 1;
        size_t total_combinations = std::pow(range, num_monomials);
        
        return total_combinations - 1;
    };
    
    const size_t f_count = count_polys(config_.max_degree_f);
    const size_t g_count = count_polys(config_.max_degree_g);
    
    return f_count * g_count;
}