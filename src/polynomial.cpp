#include "polynomial.h"
#include <sstream>
#include <algorithm>

using namespace GiNaC;

namespace poly {

ex parse_polynomial(const std::string& expr_str) {
    symbol x("x");
    symbol y("y");
    
    symtab table;
    table["x"] = x;
    table["y"] = y;
    
    parser reader(table);
    
    return reader(expr_str);
}

ex parse_polynomial(const std::string& expr_str, const symbol& x_sym, const symbol& y_sym) {
    symtab table;
    table["x"] = x_sym;
    table["y"] = y_sym;
    
    parser reader(table);
    
    return reader(expr_str);
}

ex partial_derivative(const ex& poly, const symbol& var) {
    return poly.diff(var);
}

ex substitute(const ex& poly, const symbol& var, const ex& expr) {
    return poly.subs(var == expr);
}

bool is_divisible(const ex& dividend, const ex& divisor) {
    if (dividend.is_zero() && divisor.is_zero()) {
        return true;
    }
    
    if (divisor.is_zero()) {
        return false;
    }
    
    if (dividend.is_zero()) {
        return true;
    }
    
    try {
        symbol x("x");
        ex q = quo(dividend, divisor, x);
        ex r = dividend - q * divisor;
        r = expand(r);
        return r.is_zero();
    } catch (...) {
        return false;
    }
}

std::string poly_hash(const ex& poly) {
    symbol x("x"), y("y");
    auto expanded = expand(poly);
    
    // Extract all terms with their coefficients
    std::vector<std::pair<std::pair<int, int>, ex>> terms; // ((x_deg, y_deg), coeff)
    
    int max_x = expanded.degree(x);
    int max_y = expanded.degree(y);
    
    for (int i = 0; i <= max_x; ++i) {
        for (int j = 0; j <= max_y; ++j) {
            auto coeff = expanded.coeff(x, i).coeff(y, j);
            if (!coeff.is_zero()) {
                terms.push_back({{i, j}, coeff});
            }
        }
    }
    
    // Sort by total degree, then by x degree
    std::sort(terms.begin(), terms.end(), [](const auto& a, const auto& b) {
        int total_a = a.first.first + a.first.second;
        int total_b = b.first.first + b.first.second;
        if (total_a != total_b) return total_a < total_b;
        return a.first.first < b.first.first;
    });
    
    // Build canonical string
    std::ostringstream oss;
    for (size_t idx = 0; idx < terms.size(); ++idx) {
        auto [deg_pair, coeff] = terms[idx];
        auto [x_deg, y_deg] = deg_pair;
        
        if (idx > 0 && !coeff.info(info_flags::negative)) {
            oss << "+";
        }
        
        if (x_deg == 0 && y_deg == 0) {
            oss << coeff;
        } else {
            if (coeff != 1 && coeff != -1) {
                oss << coeff << "*";
            } else if (coeff == -1) {
                oss << "-";
            }
            
            if (x_deg > 0) {
                oss << "x";
                if (x_deg > 1) oss << "^" << x_deg;
            }
            if (y_deg > 0) {
                if (x_deg > 0) oss << "*";
                oss << "y";
                if (y_deg > 1) oss << "^" << y_deg;
            }
        }
    }
    
    return oss.str();
}

bool is_zero(const ex& poly) {
    return expand(poly).is_zero();
}

int total_degree(const ex& poly) {
    try {
        if (poly.is_zero() || is_a<numeric>(poly)) {
            return 0;
        }
        symbol x("x");
        symbol y("y");
        return poly.degree(x) + poly.degree(y);
    } catch (...) {
        return 0;
    }
}

std::optional<ex> extract_cube_root(const ex& poly) {
    try {
        auto expanded = expand(poly);
        
        if (expanded.is_zero()) {
            return std::nullopt;
        }
        
        auto factored = factor(expanded);
        
        if (is_a<power>(factored)) {
            auto base = factored.op(0);
            auto exponent = factored.op(1);
            
            if (is_a<numeric>(exponent)) {
                auto exp_num = ex_to<numeric>(exponent);
                if (exp_num.is_integer() && exp_num.to_int() == 3) {
                    return base;
                }
            }
        }
        
        if (is_a<mul>(factored)) {
            ex base = 1;
            
            for (size_t i = 0; i < factored.nops(); ++i) {
                auto term = factored.op(i);
                
                if (is_a<power>(term)) {
                    auto term_base = term.op(0);
                    auto term_exp = term.op(1);
                    
                    if (!is_a<numeric>(term_exp)) {
                        return std::nullopt;
                    }
                    
                    auto exp_num = ex_to<numeric>(term_exp);
                    if (!exp_num.is_integer() || exp_num.to_int() % 3 != 0) {
                        return std::nullopt;
                    }
                    
                    base *= pow(term_base, exp_num.to_int() / 3);
                } else if (is_a<numeric>(term)) {
                    base *= term;
                } else {
                    return std::nullopt;
                }
            }
            
            auto test_cube = expand(pow(base, 3));
            if (expand(test_cube - expanded).is_zero()) {
                return base;
            }
        }
        
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

}