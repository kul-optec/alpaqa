#pragma once

#include <alpaqa/config/config.hpp>

#include "options.hpp"
#include "problem.hpp"
#include "results.hpp"
#include "solver-driver.hpp"

SharedSolverWrapper make_panoc_driver(std::string_view direction,
                                      Options &opts);
SharedSolverWrapper make_zerofpr_driver(std::string_view direction,
                                        Options &opts);
