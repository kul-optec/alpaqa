#pragma once

#include <nlohmann/json.hpp>
#include <vector>

namespace alpaqa::mex {

struct SolverResults {
    std::vector<double> x, y;
    nlohmann::json stats;
};

using Options = nlohmann::json;

} // namespace alpaqa::mex