#pragma once

#include <ginac/ginac.h>

struct ThreadLocalSymbols {
    GiNaC::symbol x;
    GiNaC::symbol y;
    GiNaC::symbol u;
    GiNaC::symbol v;
    
    ThreadLocalSymbols()
        : x("x"), y("y"), u("u"), v("v") {}
};