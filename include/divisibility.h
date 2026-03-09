#pragma once

#include <ginac/ginac.h>
#include <map>
#include <string>

struct DivisibilityResult {
    bool df_divisible;
    bool dg_divisible;
    bool both_divisible;
};

class DivisibilityChecker {
public:
    DivisibilityChecker();
    
    DivisibilityResult check_conditions(const GiNaC::ex& q, const GiNaC::ex& f, const GiNaC::ex& g);

private:
    GiNaC::symbol x_;
    GiNaC::symbol u_;
    GiNaC::symbol v_;
};