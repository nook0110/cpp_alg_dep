#include "divisibility.h"
#include "polynomial.h"
#include "symbols.h"

using namespace GiNaC;
using namespace PolySymbols;

DivisibilityChecker::DivisibilityChecker()
    : x_(x), u_(u), v_(v) {}

DivisibilityResult DivisibilityChecker::check_conditions(const ex& q, const ex& f, const ex& g) {
    try {
        ex dq_dx = q.diff(x_);
        ex dq_du = q.diff(u_);
        ex dq_dv = q.diff(v_);
        
        ex dq_dx_sub = dq_dx.subs(lst{u_ == f, v_ == g});
        ex dq_du_sub = dq_du.subs(lst{u_ == f, v_ == g});
        ex dq_dv_sub = dq_dv.subs(lst{u_ == f, v_ == g});
        
        bool df_divisible = PolynomialOps::is_divisible(dq_du_sub, dq_dx_sub);
        bool dg_divisible = PolynomialOps::is_divisible(dq_dv_sub, dq_dx_sub);
        
        return {df_divisible, dg_divisible, df_divisible && dg_divisible};
    } catch (...) {
        return {false, false, false};
    }
}