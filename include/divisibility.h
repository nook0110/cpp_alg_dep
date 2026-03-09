#pragma once

#include "symbols.h"
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
    explicit DivisibilityChecker(const ThreadLocalSymbols& symbols);
    
    DivisibilityResult check_conditions(const GiNaC::ex& q, const GiNaC::ex& f, const GiNaC::ex& g);

private:
    const ThreadLocalSymbols& symbols_;
};