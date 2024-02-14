#include <alpaqa/export.h>
#include <alpaqa/params/params.hpp>
#include <alpaqa/params/vec-from-file.hpp>
#include <alpaqa/problem/problem-with-counters.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>
#include <alpaqa/util/io/csv.hpp>
#if ALPAQA_WITH_DL
#include <alpaqa/dl/dl-problem.hpp>
#endif
#if ALPAQA_WITH_CASADI
#include <alpaqa/casadi/CasADiProblem.hpp>
#endif
#ifdef ALPAQA_WITH_CUTEST
#include <alpaqa/cutest/cutest-loader.hpp>
#endif

#include <filesystem>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
namespace fs = std::filesystem;

#include "options.hpp"
#include "problem.hpp"

namespace {

USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);

std::string get_reg_name_option(std::span<const std::string_view> prob_opts) {
    std::string name          = "register_alpaqa_problem";
    std::string_view name_key = "register=";
    auto name_it              = std::find_if(
        prob_opts.rbegin(), prob_opts.rend(),
        [&](std::string_view opt) { return opt.starts_with(name_key); });
    if (name_it != prob_opts.rend())
        name = name_it->substr(name_key.size());
    return name;
}

void load_initial_guess(Options &opts, LoadedProblem &problem) {
    const auto n = problem.problem.get_n(), m = problem.problem.get_m();
    alpaqa::params::vec_from_file<config_t> x0{n}, y0{m}, w0{2 * n};
    set_params(x0, "x0", opts);
    if (x0.value)
        problem.initial_guess_x = std::move(*x0.value);
    set_params(y0, "mul_g0", opts);
    if (y0.value)
        problem.initial_guess_y = std::move(*y0.value);
    set_params(w0, "mul_x0", opts);
    if (w0.value)
        problem.initial_guess_w = std::move(*w0.value);
}

void count_constr(ConstrCount &cnt, const alpaqa::Box<config_t> &C) {
    const auto n = C.lowerbound.size();
    cnt.lb       = 0;
    cnt.ub       = 0;
    cnt.lbub     = 0;
    cnt.eq       = 0;
    for (index_t i = 0; i < n; ++i) {
        bool lb = C.lowerbound(i) > -alpaqa::inf<config_t>;
        bool ub = C.upperbound(i) < +alpaqa::inf<config_t>;
        bool eq = C.lowerbound(i) == C.upperbound(i);
        if (eq)
            ++cnt.eq;
        else if (lb && ub)
            ++cnt.lbub;
        else if (lb)
            ++cnt.lb;
        else if (ub)
            ++cnt.ub;
    }
}

void count_problem(LoadedProblem &p) {
    if (p.problem.provides_get_box_C())
        count_constr(p.box_constr_count.emplace(), p.problem.get_box_C());
    if (p.problem.provides_get_box_D())
        count_constr(p.general_constr_count.emplace(), p.problem.get_box_D());
    if (p.problem.provides_get_jac_g_sparsity())
        p.nnz_jac_g = get_nnz(p.problem.get_jac_g_sparsity());
    if (p.problem.provides_get_hess_L_sparsity())
        p.nnz_hess_L = get_nnz(p.problem.get_hess_L_sparsity());
    if (p.problem.provides_get_hess_ψ_sparsity())
        p.nnz_hess_ψ = get_nnz(p.problem.get_hess_ψ_sparsity());
}

#if ALPAQA_WITH_DL
LoadedProblem load_dl_problem(const fs::path &full_path,
                              std::span<std::string_view> prob_opts,
                              Options &opts) {
    using TEProblem    = alpaqa::TypeErasedProblem<config_t>;
    using DLProblem    = alpaqa::dl::DLProblem;
    using CntProblem   = alpaqa::ProblemWithCounters<DLProblem>;
    auto register_name = get_reg_name_option(prob_opts);
    LoadedProblem problem{
        .problem  = TEProblem::make<CntProblem>(std::in_place, full_path,
                                               register_name, prob_opts),
        .abs_path = fs::absolute(full_path),
        .path     = full_path,
    };
    auto &cnt_problem   = problem.problem.as<CntProblem>();
    problem.name        = cnt_problem.problem.get_name();
    problem.evaluations = cnt_problem.evaluations;
    load_initial_guess(opts, problem);
    count_problem(problem);
    return problem;
}
#endif

#if ALPAQA_WITH_CASADI
template <bool = true>
LoadedProblem load_cs_problem(const fs::path &full_path,
                              std::span<std::string_view> prob_opts,
                              Options &opts) {
    static std::mutex mtx;
    std::unique_lock lck{mtx};
    using TEProblem  = alpaqa::TypeErasedProblem<config_t>;
    using CsProblem  = alpaqa::CasADiProblem<config_t>;
    using CntProblem = alpaqa::ProblemWithCounters<CsProblem>;
    LoadedProblem problem{
        .problem  = TEProblem::make<CntProblem>(std::in_place,
                                               full_path.string().c_str()),
        .abs_path = fs::absolute(full_path),
        .path     = full_path,
    };
    lck.unlock();
    auto &cnt_problem   = problem.problem.as<CntProblem>();
    auto &cs_problem    = cnt_problem.problem;
    problem.name        = cs_problem.get_name();
    problem.evaluations = cnt_problem.evaluations;
    auto param_size     = cs_problem.param.size();
    alpaqa::params::set_params(cs_problem.param, "param", prob_opts);
    if (cs_problem.param.size() != param_size)
        throw alpaqa::params::invalid_param(
            "Incorrect problem parameter size (expected " +
            std::to_string(param_size) + ", but got " +
            std::to_string(cs_problem.param.size()) + ")");
    load_initial_guess(opts, problem);
    count_problem(problem);
    return problem;
}
#endif

#ifdef ALPAQA_WITH_CUTEST
template <bool = true>
LoadedProblem load_cu_problem(const fs::path &full_path,
                              std::span<std::string_view> prob_opts,
                              Options &opts) {
    std::string outsdif_path;
    alpaqa::params::set_params(outsdif_path, "outsdif", prob_opts);
    bool sparse = false;
    alpaqa::params::set_params(sparse, "sparse", prob_opts);
    static std::mutex mtx;
    std::unique_lock lck{mtx};
    using TEProblem  = alpaqa::TypeErasedProblem<config_t>;
    using CuProblem  = alpaqa::CUTEstProblem;
    using CntProblem = alpaqa::ProblemWithCounters<CuProblem>;
    LoadedProblem problem{
        .problem = TEProblem::make<CntProblem>(std::in_place, full_path.c_str(),
                                               outsdif_path.c_str(), sparse),
        .abs_path = fs::absolute(full_path),
        .path     = full_path,
    };
    lck.unlock();
    auto &cnt_problem       = problem.problem.as<CntProblem>();
    auto &cu_problem        = cnt_problem.problem;
    problem.name            = cu_problem.get_name();
    problem.evaluations     = cnt_problem.evaluations;
    problem.initial_guess_x = std::move(cu_problem.x0);
    problem.initial_guess_y = std::move(cu_problem.y0);
    load_initial_guess(opts, problem);
    count_problem(problem);
    return problem;
}
#endif

} // namespace

LoadedProblem load_problem(std::string_view type, const fs::path &dir,
                           const fs::path &file, Options &opts) {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    // Isolate problem-specific options
    std::vector<std::string_view> prob_opts;
    std::string_view prob_prefix = "problem.";
    auto options                 = opts.options();
    auto used                    = opts.used();
    for (auto opt = options.begin(); opt != options.end(); ++opt) {
        if (opt->starts_with(prob_prefix)) {
            prob_opts.push_back(opt->substr(prob_prefix.size()));
            ++used.begin()[opt - options.begin()];
        }
    }
    // Load problem
    auto full_path = dir / file;
    if (type == "dl" || type.empty()) {
#if ALPAQA_WITH_DL
        return load_dl_problem(full_path, prob_opts, opts);
#else
        throw std::logic_error("This version of alpaqa was compiled without "
                               "support for dynamic problem loading");
#endif
    } else if (type == "cs") {
#if ALPAQA_WITH_CASADI
        if constexpr (std::is_same_v<config_t, alpaqa::EigenConfigd>)
            return load_cs_problem(full_path, prob_opts, opts);
        else
            throw std::logic_error("CasADi only supports double precision.");
#else
        throw std::logic_error(
            "This version of alpaqa was compiled without CasADi support");
#endif
    } else if (type == "cu") {
#ifdef ALPAQA_WITH_CUTEST
        if constexpr (std::is_same_v<config_t, alpaqa::EigenConfigd>)
            return load_cu_problem(full_path, prob_opts, opts);
        else
            throw std::logic_error("CUTEst only supports double precision.");
#else
        throw std::logic_error(
            "This version of alpaqa was compiled without CUTEst support");
#endif
    }
    throw std::invalid_argument("Unknown problem type '" + std::string(type) +
                                "'");
}
