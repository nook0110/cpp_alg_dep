#include "dependency_finder.h"
#include "polynomial.h"

using namespace GiNaC;

DependencyFinder::DependencyFinder(const Config& config, const ThreadLocalSymbols& symbols)
    : config_(config), symbols_(symbols) {}

bool DependencyFinder::is_nontrivial_in_x(const ex& q) const {
    try {
        for (const auto& term : q) {
            int x_deg = term.degree(symbols_.x);
            int u_deg = term.degree(symbols_.u);
            int v_deg = term.degree(symbols_.v);
            
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
        ex p1 = symbols_.u - f;
        ex p2 = symbols_.v - g;
        
        p1 = expand(p1);
        p2 = expand(p2);
        
        ex res = resultant(p1, p2, symbols_.y);
        res = expand(res);
        
        if (!res.is_zero() && !res.has(symbols_.y)) {
            return res;
        }
        
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}