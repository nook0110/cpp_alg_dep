#pragma once

#include "symbols.h"
#include <ginac/ginac.h>
#include <map>
#include <string>

struct DivisibilityResult {
    bool df_divisible = false;
    bool dg_divisible = false;
    bool both_divisible = false;
    bool needs_review = false;
};

class DivisibilityChecker {
public:
    explicit DivisibilityChecker(const ThreadLocalSymbols& symbols);
    
    DivisibilityResult check_conditions(const GiNaC::ex& q, const GiNaC::ex& f, const GiNaC::ex& g);

private:
    const ThreadLocalSymbols& symbols_;
};
