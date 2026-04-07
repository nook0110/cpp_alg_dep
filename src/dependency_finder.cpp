#include "dependency_finder.h"
#include "polynomial.h"

using namespace GiNaC;

bool is_nontrivial_in_x(const ex& q, const ThreadLocalSymbols& symbols) {
    try {
        for (const auto& term : q) {
            const auto x_deg = term.degree(symbols.x);
            const auto u_deg = term.degree(symbols.u);
            const auto v_deg = term.degree(symbols.v);

            if (x_deg == 0) continue;
            if (x_deg >= 2) return true;
            if (x_deg == 1 && (u_deg > 0 || v_deg > 0)) return true;
        }
        return false;
    } catch (...) {
        return true;
    }
}

DependencyFinder::DependencyFinder(const Config& config, const ThreadLocalSymbols& symbols)
    : config_(config), symbols_(symbols) {}

std::pair<std::optional<ex>, bool> DependencyFinder::find_dependency(const ex& f, const ex& g) {
    auto q = try_resultant(f, g);
    if (q.has_value()) {
        const auto is_trivial = !is_nontrivial_in_x(q.value(), symbols_);
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