#pragma once

#include "state.h"
#include <string>

struct CheckpointManager {
    BruteForceState& state;
    const std::string& state_file;
    const int checkpoint_interval;
    int counter;
    
    CheckpointManager(BruteForceState& st, const std::string& file, int interval)
        : state(st), state_file(file), checkpoint_interval(interval), counter(0) {}
    
    void increment_and_save_if_needed() {
        counter++;
        if (counter >= checkpoint_interval) {
            state.save(state_file);
            counter = 0;
        }
    }
    
    void force_save() {
        state.save(state_file);
        counter = 0;
    }
};
