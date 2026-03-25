#include <gtest/gtest.h>
#include "generator.h"
#include "config.h"
#include <atomic>
#include <vector>
#include <sstream>

TEST(ResumeTest, GeneratorSavesAndRestoresCoefficients) {
    Config config;
    config.max_degree_f = 1;
    config.max_degree_g = 1;
    config.coeff_min = 0;
    config.coeff_max = 1;
    
    std::atomic<bool> shutdown{false};
    
    std::vector<int> saved_f_coeffs, saved_g_coeffs;
    int first_run_count = 0;
    {
        PolynomialGenerator gen(config, shutdown);
        gen.generate_pairs([&](const GiNaC::ex&, const GiNaC::ex&) {
            first_run_count++;
            if (first_run_count == 5) {
                saved_f_coeffs = gen.get_f_coeffs();
                saved_g_coeffs = gen.get_g_coeffs();
            }
            if (first_run_count >= 10) {
                shutdown.store(true);
            }
        });
    }
    
    ASSERT_EQ(first_run_count, 10);
    ASSERT_FALSE(saved_f_coeffs.empty());
    ASSERT_FALSE(saved_g_coeffs.empty());
    
    shutdown.store(false);
    
    int resumed_count = 0;
    {
        PolynomialGenerator gen_resumed(config, shutdown, saved_f_coeffs, saved_g_coeffs);
        
        auto initial_f_coeffs = gen_resumed.get_f_coeffs();
        auto initial_g_coeffs = gen_resumed.get_g_coeffs();
        
        EXPECT_EQ(initial_f_coeffs, saved_f_coeffs) << "Generator should start with saved f coefficients";
        EXPECT_EQ(initial_g_coeffs, saved_g_coeffs) << "Generator should start with saved g coefficients";
        
        gen_resumed.generate_pairs([&](const GiNaC::ex&, const GiNaC::ex&) {
            resumed_count++;
            if (resumed_count >= 5) {
                shutdown.store(true);
            }
        });
    }
    
    EXPECT_EQ(resumed_count, 5);
}

TEST(ResumeTest, EmptyResumeStartsFromBeginning) {
    Config config;
    config.max_degree_f = 1;
    config.max_degree_g = 1;
    config.coeff_min = 0;
    config.coeff_max = 1;
    
    std::atomic<bool> shutdown{false};
    
    PolynomialGenerator gen(config, shutdown);
    int count = 0;
    gen.generate_pairs([&](const GiNaC::ex& f, const GiNaC::ex& g) {
        count++;
        if (count >= 3) {
            shutdown.store(true);
        }
    });
    
    EXPECT_EQ(count, 3);
}

TEST(ResumeTest, CoefficientsUpdateDuringGeneration) {
    Config config;
    config.max_degree_f = 1;
    config.max_degree_g = 1;
    config.coeff_min = 0;
    config.coeff_max = 1;
    
    std::atomic<bool> shutdown{false};
    
    PolynomialGenerator gen(config, shutdown);
    
    std::vector<int> prev_f_coeffs;
    std::vector<int> prev_g_coeffs;
    bool coeffs_changed = false;
    
    int count = 0;
    gen.generate_pairs([&](const GiNaC::ex& f, const GiNaC::ex& g) {
        auto f_coeffs = gen.get_f_coeffs();
        auto g_coeffs = gen.get_g_coeffs();
        
        if (count > 0) {
            if (f_coeffs != prev_f_coeffs || g_coeffs != prev_g_coeffs) {
                coeffs_changed = true;
            }
        }
        
        prev_f_coeffs = f_coeffs;
        prev_g_coeffs = g_coeffs;
        
        count++;
        if (count >= 10) {
            shutdown.store(true);
        }
    });
    
    EXPECT_TRUE(coeffs_changed) << "Coefficients should change during generation";
}
