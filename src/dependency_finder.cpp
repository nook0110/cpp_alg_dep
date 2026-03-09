#include "dependency_finder.h"
#include "polynomial.h"
#include "symbols.h"

using namespace GiNaC;
using namespace PolySymbols;

DependencyFinder::DependencyFinder(const Config& config)
    : config_(config), x_(x), y_(y), u_(u), v_(v) {}

bool DependencyFinder::is_nontrivial_in_x(const ex& q) const {
    try {
        for (const auto& term : q) {
            int x_deg = term.degree(x_);
            int u_deg = term.degree(u_);
            int v_deg = term.degree(v_);
            
            if (x_deg == 0) continue;
            if (x_deg >= 2) return true;
            if (x_deg == 1 && (u_deg > 0 || v_deg > 0)) return true;
        }
        return false;
    } catch (...) {
        return true;
    }
}

std::pair<std::optional<ex>, bool> DependencyFinder::find_dependency(const ex& f, const ex& g) {
    auto q = try_resultant(f, g);
    if (q.has_value()) {
        bool is_trivial = !is_nontrivial_in_x(q.value());
        return {q, is_trivial};
    }
    
    return {std::nullopt, false};
}

std::optional<ex> DependencyFinder::try_resultant(const ex& f, const ex& g) {
    try {
        ex p1 = u_ - f;
        ex p2 = v_ - g;
        
        p1 = expand(p1);
        p2 = expand(p2);
        
        ex res = resultant(p1, p2, y_);
        res = expand(res);
        
        if (!res.is_zero() && !res.has(y_)) {
            return res;
        }
        
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}