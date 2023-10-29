#include <alpaqa/params/params.hpp>
#include <alpaqa/problem/problem-with-counters.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>
#include <alpaqa/util/io/csv.hpp>
#if ALPAQA_HAVE_DL
#include <alpaqa/dl/dl-problem.hpp>
#endif
#if ALPAQA_HAVE_CASADI
#include <alpaqa/casadi/CasADiProblem.hpp>
#endif
#ifdef ALPAQA_HAVE_CUTEST
#include <alpaqa/cutest/cutest-loader.hpp>
#endif

#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>

#include "options.hpp"
#include "problem.hpp"

namespace fs = std::filesystem;

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

#if ALPAQA_HAVE_DL
LoadedProblem load_dl_problem(const fs::path &full_path,
                              std::span<std::string_view> prob_opts,
                              Options &opts) {
    using TEProblem    = alpaqa::TypeErasedProblem<config_t>;
    using DLProblem    = alpaqa::dl::DLProblem;
    using CntProblem   = alpaqa::ProblemWithCounters<DLProblem>;
    auto register_name = get_reg_name_option(prob_opts);
    std::any dl_opt    = prob_opts;
    LoadedProblem problem{
        .problem = TEProblem::make<CntProblem>(std::in_place, full_path.c_str(),
                                               register_name, &dl_opt),
        .abs_path = fs::absolute(full_path),
        .path     = full_path,
    };
    auto &cnt_problem = problem.problem.as<CntProblem>();
    try {
        using sig_t  = std::string(const DLProblem::instance_t *);
        problem.name = cnt_problem.problem.call_extra_func<sig_t>("get_name");
    } catch (std::out_of_range &) {
        problem.name = problem.path.filename();
    }
    problem.evaluations = cnt_problem.evaluations;
    load_initial_guess(opts, problem);
    return problem;
}
#endif

#if ALPAQA_HAVE_CASADI
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
    problem.name        = problem.path.filename().string();
    problem.evaluations = cnt_problem.evaluations;
    auto param_size     = cs_problem.param.size();
    alpaqa::params::set_params(cs_problem.param, "param", prob_opts);
    if (cs_problem.param.size() != param_size)
        throw alpaqa::params::invalid_param(
            "Incorrect problem parameter size: got " +
            std::to_string(cs_problem.param.size()) + ", should be " +
            std::to_string(param_size));
    load_initial_guess(opts, problem);
    return problem;
}
#endif

#ifdef ALPAQA_HAVE_CUTEST
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
    problem.name            = cu_problem.name;
    problem.evaluations     = cnt_problem.evaluations;
    problem.initial_guess_x = std::move(cu_problem.x0);
    problem.initial_guess_y = std::move(cu_problem.y0);
    load_initial_guess(opts, problem);
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
#if ALPAQA_HAVE_DL
        return load_dl_problem(full_path, prob_opts, opts);
#else
        throw std::logic_error("This version of alpaqa was compiled without "
                               "support for dynamic problem loading");
#endif
    } else if (type == "cs") {
#if ALPAQA_HAVE_CASADI
        if constexpr (std::is_same_v<config_t, alpaqa::EigenConfigd>)
            return load_cs_problem(full_path, prob_opts, opts);
        else
            throw std::logic_error("CasADi only supports double precision.");
#else
        throw std::logic_error(
            "This version of alpaqa was compiled without CasADi support");
#endif
    } else if (type == "cu") {
#ifdef ALPAQA_HAVE_CUTEST
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
