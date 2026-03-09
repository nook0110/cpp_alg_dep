#pragma once

#include "config.h"
#include <string>
#include <vector>

class CLI {
public:
    int run(int argc, char* argv[]);

private:
    void run_brute_force(const Config& config);
    void run_manual_check(const std::string& f_str, const std::string& g_str, const Config& config);
    void print_help();
};