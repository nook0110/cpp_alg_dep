#pragma once

#include <ginac/ginac.h>

namespace PolySymbols {
    extern GiNaC::symbol x;
    extern GiNaC::symbol y;
    extern GiNaC::symbol u;
    extern GiNaC::symbol v;
}

struct ThreadLocalSymbols {
    GiNaC::symbol x;
    GiNaC::symbol y;
    GiNaC::symbol u;
    GiNaC::symbol v;
    
    ThreadLocalSymbols()
        : x("x"), y("y"), u("u"), v("v") {}
};