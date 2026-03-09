#include "config.h"
#include <stdexcept>

void Config::validate() const {
    if (max_degree_f < 0 || max_degree_g < 0 || max_degree_q < 0) {
        throw std::invalid_argument("Degrees must be non-negative");
    }
    if (coeff_min > coeff_max) {
        throw std::invalid_argument("coeff_min must be <= coeff_max");
    }
    if (checkpoint_interval < 1) {
        throw std::invalid_argument("checkpoint_interval must be >= 1");
    }
}