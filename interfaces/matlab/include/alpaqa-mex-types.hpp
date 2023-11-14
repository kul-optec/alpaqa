#pragma once

#include <nlohmann/json.hpp>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace alpaqa::mex {

struct SolverResults {
    std::vector<double> x, y;
    nlohmann::json stats;
};

struct ProblemDescription {
    std::string f, g;
    std::vector<double> C_lb, C_ub, D_lb, D_ub, l1_reg, param;
};

using Options = nlohmann::json;

SolverResults minimize(const ProblemDescription &problem,
                       std::span<const double> x0, std::span<const double> y0,
                       std::string_view method, const Options &options,
                       std::function<void(std::string_view)> write_utf8);
std::u16string utf8to16(std::string_view in);
std::string utf16to8(std::u16string_view in);

} // namespace alpaqa::mex