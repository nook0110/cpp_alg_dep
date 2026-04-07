#pragma once

#include <ginac/ginac.h>
#include <string>
#include <optional>

namespace poly {
    GiNaC::ex parse_polynomial(const std::string& expr);
    GiNaC::ex parse_polynomial(const std::string& expr, const GiNaC::symbol& x, const GiNaC::symbol& y);
    GiNaC::ex partial_derivative(const GiNaC::ex& poly, const GiNaC::symbol& var);
    GiNaC::ex substitute(const GiNaC::ex& poly, const GiNaC::symbol& var, const GiNaC::ex& expr);
    bool is_divisible(const GiNaC::ex& dividend, const GiNaC::ex& divisor);
    std::string poly_hash(const GiNaC::ex& poly);
    bool is_zero(const GiNaC::ex& poly);
    int total_degree(const GiNaC::ex& poly);
    std::optional<GiNaC::ex> extract_cube_root(const GiNaC::ex& poly);
    std::optional<GiNaC::ex> extract_min_poly(const GiNaC::ex& h, const GiNaC::symbol& v, const GiNaC::symbol& x, int n);
}