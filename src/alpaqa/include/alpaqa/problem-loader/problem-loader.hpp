#pragma once

#include <alpaqa/problem-loader-export.h>

#include <alpaqa/config/config.hpp>
#include <alpaqa/params/options.hpp>
#include <alpaqa/problem/problem-counters.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>

#include <filesystem>
#include <iosfwd>
#include <memory>
#include <optional>

namespace alpaqa {

namespace fs = std::filesystem;

struct PROBLEM_LOADER_EXPORT ConstrCount {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    ConstrCount() = default;
    length_t lb   = 0; ///< Number of variables with only lower bound
    length_t ub   = 0; ///< Number of variables with only upper bound
    length_t lbub = 0; ///< Number of variables with both bounds
    length_t eq   = 0; ///< Number of variables with equal bounds
};

struct PROBLEM_LOADER_EXPORT LoadedProblem {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    alpaqa::TypeErasedProblem<config_t> problem;
    fs::path abs_path;
    fs::path path;
    std::string name                                 = path.filename().string();
    std::shared_ptr<alpaqa::EvalCounter> evaluations = nullptr;
    vec initial_guess_x = vec::Zero(problem.get_num_variables()); ///< Unknowns
    vec initial_guess_y =
        vec::Zero(problem.get_num_constraints());     ///< Multipliers g
    vec initial_guess_w = alpaqa::null_vec<config_t>; ///< Multipliers bounds
    std::optional<ConstrCount> box_constr_count     = std::nullopt,
                               general_constr_count = std::nullopt;
    std::optional<length_t> nnz_jac_g = std::nullopt, nnz_hess_L = std::nullopt,
                            nnz_hess_ψ = std::nullopt;
};

PROBLEM_LOADER_EXPORT
LoadedProblem load_problem(std::string_view type, const fs::path &file,
                           Options &opts);

PROBLEM_LOADER_EXPORT
void print_problem_description(std::ostream &os, LoadedProblem &problem,
                               bool show_funcs = true);

} // namespace alpaqa
