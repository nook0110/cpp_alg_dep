#include "divisibility.h"
#include "polynomial.h"

using namespace GiNaC;

DivisibilityChecker::DivisibilityChecker(const ThreadLocalSymbols& symbols)
    : symbols_(symbols) {}

DivisibilityResult DivisibilityChecker::check_conditions(const ex& h, const ex& f, const ex& g) {
    try {
        ex dh_dx = h.diff(symbols_.x);
        ex dh_du = h.diff(symbols_.u);
        ex dh_dv = h.diff(symbols_.v);

        ex dh_dx_sub = expand(dh_dx.subs(lst{symbols_.u == f, symbols_.v == g}));
        ex dh_du_sub = expand(dh_du.subs(lst{symbols_.u == f, symbols_.v == g}));
        ex dh_dv_sub = expand(dh_dv.subs(lst{symbols_.u == f, symbols_.v == g}));

        const auto all_zero = dh_dx_sub.is_zero() && dh_du_sub.is_zero() && dh_dv_sub.is_zero();

        if (all_zero) {
            const auto n = f.degree(symbols_.y);
            auto min_poly = poly::extract_min_poly(h, symbols_.v, symbols_.x, n);
            if (min_poly.has_value()) {
                const auto& q = min_poly.value();
                ex dq_dx = q.diff(symbols_.x);
                ex dq_du = q.diff(symbols_.u);
                ex dq_dv = q.diff(symbols_.v);

                ex dq_dx_sub = expand(dq_dx.subs(lst{symbols_.u == f, symbols_.v == g}));
                ex dq_du_sub = expand(dq_du.subs(lst{symbols_.u == f, symbols_.v == g}));
                ex dq_dv_sub = expand(dq_dv.subs(lst{symbols_.u == f, symbols_.v == g}));

                const auto df_divisible = poly::is_divisible(dq_du_sub, dq_dx_sub);
                const auto dg_divisible = poly::is_divisible(dq_dv_sub, dq_dx_sub);

                return {df_divisible, dg_divisible, df_divisible && dg_divisible, true, true, min_poly};
            }
        }

        const auto df_divisible = poly::is_divisible(dh_du_sub, dh_dx_sub);
        const auto dg_divisible = poly::is_divisible(dh_dv_sub, dh_dx_sub);

        if (df_divisible && dg_divisible) {
            const auto n = f.degree(symbols_.y);
            auto min_poly = poly::extract_min_poly(h, symbols_.v, symbols_.x, n);
            return {true, true, true, all_zero, min_poly.has_value(), min_poly};
        }

        return {df_divisible, dg_divisible, df_divisible && dg_divisible, all_zero, false, std::nullopt};
    } catch (...) {
        return {false, false, false, false, false, std::nullopt};
    }
}