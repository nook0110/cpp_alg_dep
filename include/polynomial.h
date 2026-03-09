#pragma once

#include <ginac/ginac.h>
#include <string>
#include <optional>

class PolynomialOps {
public:
    static GiNaC::ex parse_polynomial(const std::string& expr);
    static GiNaC::ex partial_derivative(const GiNaC::ex& poly, const GiNaC::symbol& var);
    static GiNaC::ex substitute(const GiNaC::ex& poly, const GiNaC::symbol& var, const GiNaC::ex& expr);
    static bool is_divisible(const GiNaC::ex& dividend, const GiNaC::ex& divisor);
    static std::string poly_hash(const GiNaC::ex& poly);
    static bool is_zero(const GiNaC::ex& poly);
    static int total_degree(const GiNaC::ex& poly);
};