#pragma once

#include <alpaqa/config/config.hpp>
#include <nlohmann/json.hpp>

namespace alpaqa::mex {

USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);

struct SolverResults {
    vec x, y;
    nlohmann::json stats;
};

using Options = nlohmann::json;

} // namespace alpaqa::mex