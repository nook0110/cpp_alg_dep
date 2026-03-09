#pragma once

#include <string>
#include <optional>

class BruteForceState {
public:
    BruteForceState();
    
    void save(const std::string& filepath);
    static BruteForceState load(const std::string& filepath);
    
    void update_progress(int f_index, int g_index, bool found_dependency);
    std::string get_summary() const;
    
    int last_f_index = 0;
    int last_g_index = 0;
    int total_pairs_checked = 0;
    int pairs_with_dependency = 0;
    std::optional<std::string> start_time;
    std::optional<std::string> last_checkpoint;
};