#pragma once

#include <string>

enum class EnumStrategy {
    Lexicographic,
    DegreeFirst
};

struct Config {
    int max_degree_f = 2;
    int max_degree_g = 2;
    int max_degree_q = 3;
    int coeff_min = -2;
    int coeff_max = 2;
    EnumStrategy enum_strategy = EnumStrategy::Lexicographic;
    bool skip_trivial = true;
    std::string cache_file = "data/results.db";
    std::string state_file = "data/state.json";
    int batch_size = 100;
    int checkpoint_interval = 10;
    int num_workers = 4;
    
    void validate() const;
};