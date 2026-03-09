#include "state.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

BruteForceState::BruteForceState() = default;

std::string get_current_time_iso() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

void BruteForceState::save(const std::string& filepath) {
    fs::path path(filepath);
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }
    
    last_checkpoint = get_current_time_iso();
    
    json j;
    j["last_f_index"] = last_f_index;
    j["last_g_index"] = last_g_index;
    j["total_pairs_checked"] = total_pairs_checked;
    j["pairs_with_dependency"] = pairs_with_dependency;
    j["start_time"] = start_time.value_or("");
    j["last_checkpoint"] = last_checkpoint.value_or("");
    
    std::ofstream file(filepath);
    file << j.dump(2);
}

BruteForceState BruteForceState::load(const std::string& filepath) {
    BruteForceState state;
    
    if (!fs::exists(filepath)) {
        state.start_time = get_current_time_iso();
        return state;
    }
    
    try {
        std::ifstream file(filepath);
        json j;
        file >> j;
        
        state.last_f_index = j.value("last_f_index", 0);
        state.last_g_index = j.value("last_g_index", 0);
        state.total_pairs_checked = j.value("total_pairs_checked", 0);
        state.pairs_with_dependency = j.value("pairs_with_dependency", 0);
        
        std::string start = j.value("start_time", "");
        if (!start.empty()) state.start_time = start;
        
        std::string checkpoint = j.value("last_checkpoint", "");
        if (!checkpoint.empty()) state.last_checkpoint = checkpoint;
    } catch (...) {
        state.start_time = get_current_time_iso();
    }
    
    return state;
}

void BruteForceState::update_progress(int f_index, int g_index, bool found_dependency) {
    last_f_index = f_index;
    last_g_index = g_index;
    total_pairs_checked++;
    if (found_dependency) {
        pairs_with_dependency++;
    }
}

std::string BruteForceState::get_summary() const {
    std::ostringstream oss;
    oss << "Total pairs checked: " << total_pairs_checked << "\n";
    oss << "Pairs with dependency: " << pairs_with_dependency << "\n";
    oss << "Last position: f_index=" << last_f_index << ", g_index=" << last_g_index << "\n";
    
    if (start_time.has_value()) {
        oss << "Started: " << start_time.value() << "\n";
    }
    if (last_checkpoint.has_value()) {
        oss << "Last checkpoint: " << last_checkpoint.value() << "\n";
    }
    
    return oss.str();
}