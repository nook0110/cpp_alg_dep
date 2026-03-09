#include "polynomial.h"
#include "symbols.h"
#include <sstream>
#include <algorithm>

using namespace GiNaC;
using namespace PolySymbols;

ex PolynomialOps::parse_polynomial(const std::string& expr_str) {
    symtab table;
    table["x"] = x;
    table["y"] = y;
    
    parser reader(table);
    
    return reader(expr_str);
}

ex PolynomialOps::partial_derivative(const ex& poly, const symbol& var) {
    return poly.diff(var);
}

ex PolynomialOps::substitute(const ex& poly, const symbol& var, const ex& expr) {
    return poly.subs(var == expr);
}

bool PolynomialOps::is_divisible(const ex& dividend, const ex& divisor) {
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
        ex q = quo(dividend, divisor, x);
        ex r = dividend - q * divisor;
        r = expand(r);
        return r.is_zero();
    } catch (...) {
        return false;
    }
}

std::string PolynomialOps::poly_hash(const ex& poly) {
    std::ostringstream oss;
    oss << expand(poly);
    return oss.str();
}

bool PolynomialOps::is_zero(const ex& poly) {
    return expand(poly).is_zero();
}

int PolynomialOps::total_degree(const ex& poly) {
    try {
        if (poly.is_zero() || is_a<numeric>(poly)) {
            return 0;
        }
        return poly.degree(x) + poly.degree(y);
    } catch (...) {
        return 0;
    }
}