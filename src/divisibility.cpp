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
        ex dq_dv_sub = dq_dv.subs(lst{symbols_.v == f, symbols_.v == g});
        
        bool df_divisible = PolynomialOps::is_divisible(dq_du_sub, dq_dx_sub);
        bool dg_divisible = PolynomialOps::is_divisible(dq_dv_sub, dq_dx_sub);
        
        return {df_divisible, dg_divisible, df_divisible && dg_divisible};
    } catch (...) {
        return {false, false, false};
    }
}