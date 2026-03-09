#pragma once

#include "config.h"
#include <ginac/ginac.h>
#include <vector>
#include <utility>
#include <generator>
#include <iostream>

class PolynomialGenerator {
public:
    explicit PolynomialGenerator(const Config& config);
    
    template<typename Callback>
    void generate_pairs(Callback callback);
    
    size_t count_total_pairs() const;

private:
    Config config_;
    GiNaC::symbol x_;
    GiNaC::symbol y_;
    
    std::generator<GiNaC::ex> generate_polynomials_lazy(int max_degree);
    bool should_skip(const GiNaC::ex& f, const GiNaC::ex& g) const;
};

template<typename Callback>
void PolynomialGenerator::generate_pairs(Callback callback) {
    std::cout << "Starting lazy pair generation..." << std::endl;
    size_t pair_count = 0;
    
    for (const auto& f : generate_polynomials_lazy(config_.max_degree_f)) {
        for (const auto& g : generate_polynomials_lazy(config_.max_degree_g)) {
            if (!should_skip(f, g)) {
                callback(f, g);
                pair_count++;
                if (pair_count % 1000 == 0) {
                    std::cout << "Processed " << pair_count << " pairs..." << std::endl;
                }
            }
        }
    }
}