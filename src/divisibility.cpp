#include "divisibility.h"
#include "polynomial.h"

using namespace GiNaC;

DivisibilityChecker::DivisibilityChecker(const ThreadLocalSymbols& symbols)
    : symbols_(symbols) {}

DivisibilityResult DivisibilityChecker::check_conditions(const ex& q, const ex& f, const ex& g) {
    try {
        ex dq_dx = q.diff(symbols_.x);
        ex dq_du = q.diff(symbols_.u);
        ex dq_dv = q.diff(symbols_.v);
        
        ex dq_dx_sub = dq_dx.subs(lst{symbols_.u == f, symbols_.v == g});
        ex dq_du_sub = dq_du.subs(lst{symbols_.u == f, symbols_.v == g});
        ex dq_dv_sub = dq_dv.subs(lst{symbols_.u == f, symbols_.v == g});
        
        bool all_zero = dq_dx_sub.is_zero() && dq_du_sub.is_zero() && dq_dv_sub.is_zero();
        
        if (all_zero) {
            auto cube_base = poly::extract_cube_root(q);
            if (cube_base.has_value()) {
                ex q_base = cube_base.value();
                ex dq_base_dx = q_base.diff(symbols_.x);
                ex dq_base_du = q_base.diff(symbols_.u);
                ex dq_base_dv = q_base.diff(symbols_.v);
                
                ex dq_base_dx_sub = dq_base_dx.subs(lst{symbols_.u == f, symbols_.v == g});
                ex dq_base_du_sub = dq_base_du.subs(lst{symbols_.u == f, symbols_.v == g});
                ex dq_base_dv_sub = dq_base_dv.subs(lst{symbols_.u == f, symbols_.v == g});
                
                bool df_divisible = poly::is_divisible(dq_base_du_sub, dq_base_dx_sub);
                bool dg_divisible = poly::is_divisible(dq_base_dv_sub, dq_base_dx_sub);
                
                return {df_divisible, dg_divisible, df_divisible && dg_divisible, true, true, cube_base};
            }
        }
        
        bool df_divisible = poly::is_divisible(dq_du_sub, dq_dx_sub);
        bool dg_divisible = poly::is_divisible(dq_dv_sub, dq_dx_sub);
        
        return {df_divisible, dg_divisible, df_divisible && dg_divisible, all_zero, false, std::nullopt};
    } catch (...) {
        return {false, false, false, false, false, std::nullopt};
    }
}