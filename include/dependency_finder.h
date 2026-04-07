#pragma once

#include "config.h"
#include "symbols.h"
#include <ginac/ginac.h>
#include <optional>
#include <utility>

[[nodiscard]] bool is_nontrivial_in_x(const GiNaC::ex& q, const ThreadLocalSymbols& symbols);

class DependencyFinder {
public:
    explicit DependencyFinder(const Config& config, const ThreadLocalSymbols& symbols);

    std::pair<std::optional<GiNaC::ex>, bool> find_dependency(const GiNaC::ex& f, const GiNaC::ex& g);

private:
    Config config_;
    const ThreadLocalSymbols& symbols_;

    std::optional<GiNaC::ex> try_resultant(const GiNaC::ex& f, const GiNaC::ex& g);
};