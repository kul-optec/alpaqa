#pragma once

#include <alpaqa/config/config.hpp>
#include <alpaqa/params/options.hpp>

#include <alpaqa/driver/solver-driver.hpp>

namespace alpaqa::driver {
SharedSolverWrapper make_lbfgsb_driver(std::string_view direction,
                                       Options &opts);
}
