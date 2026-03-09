#pragma once

#include "config.h"
#include <ginac/ginac.h>
#include <optional>
#include <utility>

class DependencyFinder {
public:
    explicit DependencyFinder(const Config& config);
    
    std::pair<std::optional<GiNaC::ex>, bool> find_dependency(const GiNaC::ex& f, const GiNaC::ex& g);

private:
    Config config_;
    GiNaC::symbol x_;
    GiNaC::symbol y_;
    GiNaC::symbol u_;
    GiNaC::symbol v_;
    
    bool is_nontrivial_in_x(const GiNaC::ex& q) const;
    std::optional<GiNaC::ex> try_resultant(const GiNaC::ex& f, const GiNaC::ex& g);
};