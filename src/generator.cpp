#include "generator.h"
#include <cmath>
#include <sstream>

PolynomialGenerator::PolynomialGenerator(const Config& config, std::atomic<bool>& shutdown, const std::vector<int>& f_resume, const std::vector<int>& g_resume)
    : config_(config), shutdown_requested_(shutdown), f_resume_(f_resume), g_resume_(g_resume), f_coeffs_(f_resume), g_coeffs_(g_resume) {}

std::string poly_to_string(const Poly& poly) {
    std::ostringstream oss;
    bool first = true;
    for (const auto& term : poly) {
        if (!first && term.coeff > 0) oss << "+";
        first = false;
        if (term.deg_x == 0 && term.deg_y == 0) {
            oss << term.coeff;
        } else {
            if (term.coeff == -1) oss << "-";
            else if (term.coeff != 1) oss << term.coeff << "*";
            if (term.deg_x > 0) {
                oss << "x";
                if (term.deg_x > 1) oss << "^" << term.deg_x;
            }
            if (term.deg_y > 0) {
                if (term.deg_x > 0) oss << "*";
                oss << "y";
                if (term.deg_y > 1) oss << "^" << term.deg_y;
            }
        }
    }
    return oss.str();
}

std::generator<Poly> PolynomialGenerator::generate_polynomials_lazy(int max_degree, const std::vector<int>& resume_coeffs, PolyType type) {
    const int coeff_min = config_.coeff_min;
    const int coeff_max = config_.coeff_max;

    std::vector<std::pair<int, int>> monomials;
    for (int deg = 0; deg <= max_degree; ++deg) {
        for (int i = 0; i <= deg; ++i) {
            monomials.push_back({i, deg - i});
        }
    }

    const int num_terms = static_cast<int>(monomials.size());
    std::vector<int> coeffs(num_terms, coeff_min);

    if (!resume_coeffs.empty() && static_cast<int>(resume_coeffs.size()) == num_terms) {
        coeffs = resume_coeffs;
    }

    auto advance_coeffs = [&]() -> bool {
        int pos = 0;
        while (pos < num_terms && coeffs[pos] == coeff_max) {
            coeffs[pos] = coeff_min;
            pos++;
        }
        if (pos >= num_terms) return false;
        coeffs[pos]++;
        return true;
    };

    while (true) {
        if (shutdown_requested_.load()) break;

        Poly poly;
        for (int idx = 0; idx < num_terms; ++idx) {
            if (coeffs[idx] != 0) {
                poly.push_back({coeffs[idx], monomials[idx].first, monomials[idx].second});
            }
        }

        if (!poly.empty()) {
            if (type == PolyType::F) {
                f_coeffs_ = coeffs;
            } else {
                g_coeffs_ = coeffs;
            }
            co_yield poly;
            if (shutdown_requested_.load()) break;
        }

        if (!advance_coeffs()) break;
    }
}

bool PolynomialGenerator::should_skip(const Poly& f, const Poly& g) const {
    if (f.empty() || g.empty()) return true;

    const auto is_constant = [](const Poly& p) {
        return p.size() == 1 && p[0].deg_x == 0 && p[0].deg_y == 0;
    };

    if (is_constant(f) || is_constant(g)) return true;

    const auto apply_signs = [](const Poly& p, int sf, int sx, int sy) -> Poly {
        Poly result;
        for (const auto& term : p) {
            const int sign = sf
                * (sx == -1 && term.deg_x % 2 != 0 ? -1 : 1)
                * (sy == -1 && term.deg_y % 2 != 0 ? -1 : 1);
            result.push_back({term.coeff * sign, term.deg_x, term.deg_y});
        }
        return result;
    };

    const PolyPair canonical{f, g};

    for (const bool swap_fg : {false, true}) {
        const auto& pf = swap_fg ? g : f;
        const auto& pg = swap_fg ? f : g;
        for (const int sf : {-1, 1}) {
            for (const int sx : {-1, 1}) {
                for (const int sy : {-1, 1}) {
                    if (!swap_fg && sf == 1 && sx == 1 && sy == 1) continue;
                    const PolyPair variant{apply_signs(pf, sf, sx, sy), apply_signs(pg, sf, sx, sy)};
                    if (variant < canonical) return true;
                }
            }
        }
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
        return static_cast<size_t>(std::pow(range, num_monomials)) - 1;
    };

    return count_polys(config_.max_degree_f) * count_polys(config_.max_degree_g);
}
