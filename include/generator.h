#pragma once

#include "config.h"
#include "state.h"
#include <vector>
#include <utility>
#include <tuple>
#include <generator>
#include <iostream>
#include <atomic>
#include <sstream>

struct PolyTerm {
    int coeff;
    int deg_x;
    int deg_y;

    [[nodiscard]] auto operator<=>(const PolyTerm&) const = default;
};

using Poly = std::vector<PolyTerm>;

struct PolyPair {
    Poly f;
    Poly g;

    [[nodiscard]] bool operator<(const PolyPair& other) const {
        return std::tie(f, g) < std::tie(other.f, other.g);
    }
};

std::string poly_to_string(const Poly& poly);

enum class PolyType { F, G };

class PolynomialGenerator {
public:
    explicit PolynomialGenerator(const Config& config, std::atomic<bool>& shutdown, const std::vector<int>& f_resume = {}, const std::vector<int>& g_resume = {});

    template<typename Callback>
    void generate_pairs(Callback callback);

    size_t count_total_pairs() const;

    const std::vector<int>& get_f_coeffs() const { return f_coeffs_; }
    const std::vector<int>& get_g_coeffs() const { return g_coeffs_; }
    int get_f_index() const { return f_index_; }
    int get_g_index() const { return g_index_; }

private:
    Config config_;
    std::atomic<bool>& shutdown_requested_;
    std::vector<int> f_resume_;
    std::vector<int> g_resume_;
    std::vector<int> f_coeffs_;
    std::vector<int> g_coeffs_;
    bool is_first_f_ = true;

    std::generator<Poly> generate_polynomials_lazy(int max_degree, const std::vector<int>& resume_coeffs, PolyType type);
    bool should_skip(const Poly& f, const Poly& g) const;
    int f_index_ = 0;
    int g_index_ = 0;
};

template<typename Callback>
void PolynomialGenerator::generate_pairs(Callback callback) {
    std::cout << "Starting lazy pair generation..." << std::endl;
    size_t pair_count = 0;

    if (!f_resume_.empty() || !g_resume_.empty()) {
        std::cout << "Resuming from previous state" << std::endl;
    }

    f_index_ = 0;
    is_first_f_ = true;
    for (const auto& f : generate_polynomials_lazy(config_.max_degree_f, f_resume_, PolyType::F)) {
        if (shutdown_requested_.load()) {
            std::cout << "[DEBUG] Shutdown detected in outer f loop at f_index=" << f_index_ << std::endl;
            break;
        }

        g_index_ = 0;
        for (const auto& g : generate_polynomials_lazy(config_.max_degree_g, {}, PolyType::G)) {
            if (shutdown_requested_.load()) {
                std::cout << "[DEBUG] Shutdown detected in inner g loop at f_index=" << f_index_
                          << ", g_index=" << g_index_ << std::endl;
                break;
            }

            if (!should_skip(f, g)) {
                callback(f, g);
                pair_count++;
                if (pair_count % 1000 == 0) {
                    std::cout << "Processed " << pair_count << " pairs..." << std::endl;
                }
            }
            g_index_++;
        }

        if (shutdown_requested_.load()) {
            std::cout << "[DEBUG] Shutdown detected after inner g loop, breaking outer loop" << std::endl;
            break;
        }

        is_first_f_ = false;
        f_index_++;
    }

    std::cout << "[DEBUG] Exited generate_pairs loops. Total pairs processed: " << pair_count << std::endl;
}
