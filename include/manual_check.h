#pragma once

#include "config.h"
#include "dependency_finder.h"
#include "divisibility.h"
#include "cache.h"
#include <string>
#include <memory>

class ManualChecker {
public:
    explicit ManualChecker(const Config& config);
    ~ManualChecker();
    
    void check_pair(const std::string& f_str, const std::string& g_str);

private:
    Config config_;
    std::unique_ptr<DependencyFinder> finder_;
    std::unique_ptr<DivisibilityChecker> checker_;
    std::unique_ptr<ResultCache> cache_;
    
    void print_result(const CachedResult& result);
};