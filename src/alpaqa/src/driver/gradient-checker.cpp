#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include <alpaqa/config/config.hpp>
#include <alpaqa/problem/sparsity-conversions.hpp>
#include <alpaqa/util/demangled-typename.hpp>
#include <alpaqa/util/print.hpp>
#include <alpaqa-version.h>

#include "options.hpp"
#include "problem.hpp"

#include <Eigen/Sparse>

#ifdef ALPAQA_WITH_EXTERNAL_CASADI
#include <casadi/config.h>
#endif
#ifdef ALPAQA_WITH_IPOPT
#include <IpoptConfig.h>
#endif

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
namespace fs = std::filesystem;

USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);

const auto *docs = R"==(
problem types:
    dl: Dynamically loaded problem using the DLProblem class.
        Specify the name of the registration function using the
        problem.register option, e.g. problem.register=register_alpaqa_problem.
        Further options can be passed to the problem using
        problem.<key>[=<value>].
    cs: Load a CasADi problem using the CasADiProblem class.
        If a .tsv file with the same name as the shared library file exists,
        the bounds and parameters will be loaded from that file. See
        CasADiProblem::load_numerical_data for more details.
        The problem parameter can be set using the problem.param option.
    cu: Load a CUTEst problem using the CUTEstProblem class.

options:
    --full-print
        Print the full gradients.
    --no-hessians
        Do not check any Hessian matrices.
    --seed=<seed>
        Seed for the random number generator.
)==";

void print_usage(const char *a0) {
    const auto *opts = " [<problem-type>:][<path>/]<name> [options...]\n";
    std::cout << "alpaqa gradient-checker (" ALPAQA_VERSION_FULL ")\n\n"
                 "    Command-line utility to check problem gradients.\n"
                 "    alpaqa is published under the LGPL-3.0.\n"
                 "    https://github.com/kul-optec/alpaqa"
                 "\n\n"
                 "    Usage: "
              << a0 << opts << docs << std::endl;
    std::cout << "Third-party libraries:\n"
              << "  * Eigen " << EIGEN_WORLD_VERSION << '.'
              << EIGEN_MAJOR_VERSION << '.' << EIGEN_MINOR_VERSION
              << " (https://gitlab.com/libeigen/eigen) - MPL-2.0\n"
#ifdef ALPAQA_WITH_EXTERNAL_CASADI
              << "  * CasADi " CASADI_VERSION_STRING
                 " (https://github.com/casadi/casadi) - LGPL-3.0-or-later\n"
#endif
#ifdef ALPAQA_WITH_CUTEST
              << "  * CUTEst"
                 " (https://github.com/ralna/CUTEst) - BSD-3-Clause\n"
#endif
              << std::endl;
}

/// Split the string @p s on the first occurrence of @p tok.
/// Returns ("", s) if tok was not found.
auto split_once(std::string_view s, char tok = '.') {
    auto tok_pos = s.find(tok);
    if (tok_pos == s.npos)
        return std::make_tuple(std::string_view{}, s);
    std::string_view key{s.begin(), s.begin() + tok_pos};
    std::string_view rem{s.begin() + tok_pos + 1, s.end()};
    return std::make_tuple(key, rem);
}

auto get_problem_path(const char *const *argv) {
    bool rel_to_exe              = argv[1][0] == '^';
    std::string_view prob_path_s = argv[1] + static_cast<ptrdiff_t>(rel_to_exe);
    std::string_view prob_type;
    std::tie(prob_type, prob_path_s) = split_once(prob_path_s, ':');
    fs::path prob_path{prob_path_s};
    if (rel_to_exe)
        prob_path = fs::canonical(fs::path(argv[0])).parent_path() / prob_path;
    return std::make_tuple(std::move(prob_path), prob_type);
}

struct CheckGradientsOpts {
    bool print_full;
    bool hessians;
    real_t scale_perturbations;
};

void check_gradients(LoadedProblem &, std::ostream &,
                     const CheckGradientsOpts &);

int main(int argc, const char *argv[]) try {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    // Check command line options
    if (argc < 1)
        return -1;
    if (argc == 1)
        return print_usage(argv[0]), 0;
    if (argc < 2)
        return print_usage(argv[0]), -1;
    std::span args{argv, static_cast<size_t>(argc)};
    Options opts{argc - 2, argv + 2};

    // Check where to write the output to
    std::ostream &os = std::cout;

    // Check which problem to load
    auto [prob_path, prob_type] = get_problem_path(argv);

    // Load problem
    os << "Loading problem " << prob_path << std::endl;
    auto problem = load_problem(prob_type, prob_path.parent_path(),
                                prob_path.filename(), opts);
    os << "Loaded problem " << problem.path.stem().string() << " from "
       << problem.path << "\nnvar: " << problem.problem.get_num_variables()
       << "\nncon: " << problem.problem.get_num_constraints()
       << "\nProvided functions:\n";
    alpaqa::print_provided_functions(os, problem.problem);
    os << std::endl;

    // Options
    auto has_opt = [&opts](std::string_view o) {
        auto o_it = std::ranges::find(opts.options(), o);
        if (o_it == opts.options().end())
            return false;
        auto index = static_cast<size_t>(o_it - opts.options().begin());
        ++opts.used()[index];
        return true;
    };
    CheckGradientsOpts cg_opts{
        .print_full          = has_opt("--full-print"),
        .hessians            = !has_opt("--no-hessians"),
        .scale_perturbations = 1e-2,
    };
    set_params(cg_opts.scale_perturbations, "--scale-perturbations", opts);

    // Seed rand
    auto seed = static_cast<unsigned int>(std::time(nullptr));
    set_params(seed, "--seed", opts);
    std::srand(seed);

    // Check options
    auto used       = opts.used();
    auto unused_opt = std::ranges::find(used, 0);
    auto unused_idx = static_cast<size_t>(unused_opt - used.begin());
    if (unused_opt != used.end())
        throw std::invalid_argument("Unused option: " +
                                    std::string(opts.options()[unused_idx]));

    // Check gradients
    check_gradients(problem, os, cg_opts);

} catch (std::exception &e) {
    std::cerr << "Error: " << demangled_typename(typeid(e)) << ":\n  "
              << e.what() << std::endl;
    return -1;
}

void default_eval_lagrangian_gradient(
    const alpaqa::TypeErasedProblem<config_t> problem, crvec x, crvec y,
    rvec grad_L, rvec work_n) {
    if (y.size() == 0) /* [[unlikely]] */
        return problem.eval_objective_gradient(x, grad_L);
    problem.eval_objective_gradient_and_constraints_gradient_product(
        x, y, grad_L, work_n);
    grad_L += work_n;
}

auto default_eval_augmented_lagrangian_and_gradient(
    const alpaqa::TypeErasedProblem<config_t> &problem, crvec x, crvec y,
    crvec Σ, rvec grad_ψ, rvec work_n, rvec work_m) -> real_t {
    if (y.size() == 0) /* [[unlikely]] */
        return problem.eval_objective_and_gradient(x, grad_ψ);

    auto &ŷ = work_m;
    // ψ(x) = f(x) + ½ dᵀŷ
    auto f   = problem.eval_objective_and_constraints(x, ŷ);
    auto dᵀŷ = problem.calc_ŷ_dᵀŷ(ŷ, y, Σ);
    auto ψ   = f + real_t(0.5) * dᵀŷ;
    // ∇ψ(x) = ∇f(x) + ∇g(x) ŷ
    try {
        default_eval_lagrangian_gradient(problem, x, ŷ, grad_ψ, work_n);
    } catch (alpaqa::not_implemented_error &) {
        problem.eval_lagrangian_gradient(x, ŷ, grad_ψ, work_n);
    }
    return ψ;
}

auto finite_diff(const std::function<real_t(crvec)> &f, crvec x) {
    const auto n = x.size();
    vec grad(n);
    vec h           = vec::Zero(n);
    const auto ε    = 5e-6;
    const auto δ    = 1e-2 * ε;
    const real_t fx = f(x);
    for (index_t i = 0; i < n; ++i) {
        real_t hh        = std::abs(x(i)) * ε > δ ? x(i) * ε : δ;
        h(i)             = hh;
        grad.coeffRef(i) = (f(x + h) - fx) / hh;
        h(i)             = 0;
    }
    return std::make_tuple(fx, std::move(grad));
}

auto finite_diff_hess_sparse(const std::function<void(crvec, rvec)> &grad_L,
                             crvec x) {
    const auto n = x.size();
    std::vector<Eigen::Triplet<real_t, index_t>> coo;
    vec h        = vec::Zero(n);
    const auto ε = 5e-6;
    const auto δ = 1e-2 * ε;
    vec grad_x(n), grad_xh(n);
    grad_L(x, grad_x);
    for (index_t i = 0; i < n; ++i) {
        real_t hh = std::abs(x(i)) * ε > δ ? x(i) * ε : δ;
        h(i)      = hh;
        grad_L(x + h, grad_xh);
        grad_xh = (grad_xh - grad_x) / hh;
        for (index_t j = 0; j < n; ++j)
            if (real_t v = grad_xh(j); v != 0)
                coo.emplace_back(std::min(j, i), std::max(i, j),
                                 v * (i == j ? 1 : 0.5));
        h(i) = 0;
    }
    Eigen::SparseMatrix<real_t, 0, index_t> hess(n, n);
    hess.setFromTriplets(coo.begin(), coo.end());
    return hess;
}

auto finite_diff_hess(const std::function<void(crvec, rvec)> &grad_L, crvec x) {
    const auto n = x.size();
    vec h        = vec::Zero(n);
    const auto ε = 5e-6;
    const auto δ = 1e-2 * ε;
    vec grad_x(n), grad_xh(n);
    mat hess(n, n);
    grad_L(x, grad_x);
    for (index_t i = 0; i < n; ++i) {
        real_t hh = std::abs(x(i)) * ε > δ ? x(i) * ε : δ;
        h(i)      = hh;
        grad_L(x + h, grad_xh);
        hess.col(i) = (grad_xh - grad_x) / hh;
        h(i)        = 0;
    }
    return hess;
}

void check_gradients(LoadedProblem &lproblem, std::ostream &log,
                     const CheckGradientsOpts &opts) {
    auto &te_problem = lproblem.problem;

    auto x0 = lproblem.initial_guess_x;
    auto y0 = lproblem.initial_guess_y;
    auto sc = opts.scale_perturbations + x0.norm();
    auto n  = te_problem.get_num_variables(),
         m  = te_problem.get_num_constraints();

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    vec Σ = 1.5 * vec::Random(m).array() + 2;
    vec y = y0 + (opts.scale_perturbations + y0.norm()) * vec::Random(m);
    vec x = x0 + sc * vec::Random(n);
    vec v = 5e-6 * sc * vec::Random(n);
    vec gx(m);
    vec wn(n), wm(m);

    auto print_compare = [&log, &opts](const auto &other, const auto &ref) {
        auto abs_err =
            (ref - other).reshaped().template lpNorm<Eigen::Infinity>();
        auto rel_err =
            abs_err / ref.reshaped().template lpNorm<Eigen::Infinity>();
        log << "  abs error = " << alpaqa::float_to_str(abs_err) << '\n';
        log << "  rel error = " << alpaqa::float_to_str(rel_err) << '\n';
        if (opts.print_full) {
            alpaqa::print_python(log << "  (1) = ", other);
            alpaqa::print_python(log << "  (2) = ", ref) << std::endl;
        }
    };
    auto print_compare_scal = [&log, &opts](const auto &fd, const auto &ad) {
        auto abs_err = std::abs(fd - ad);
        auto rel_err = abs_err / std::abs(fd);
        log << "  abs error = " << alpaqa::float_to_str(abs_err) << '\n';
        log << "  rel error = " << alpaqa::float_to_str(rel_err) << '\n';
        if (opts.print_full) {
            log << "  (1) = " << alpaqa::float_to_str(fd) << '\n';
            log << "  (2) = " << alpaqa::float_to_str(ad) << '\n' << std::endl;
        }
    };

    auto f = [&](crvec x) { return te_problem.eval_objective(x); };
    log << "Gradient verification: ∇f(x) (grad_f compared to finite "
           "differences of f)\n";
    auto [fx, fd_grad_f] = finite_diff(f, x);
    vec grad_f(n);
    te_problem.eval_objective_gradient(x, grad_f);
    print_compare(grad_f, fd_grad_f);

    if (te_problem.provides_eval_objective_and_gradient()) {
        log << "Gradient verification: ∇f(x) (f_grad_f compared to grad_f)\n";
        vec f_grad_f(n);
        auto f2 = te_problem.eval_objective_and_gradient(x, f_grad_f);
        print_compare(f_grad_f, grad_f);
        log << "Function verification: f(x) (f_grad_f compared to f)\n";
        print_compare_scal(f2, fx);
    }

    log << "Gradient verification: ∇L(x) (grad_L compared to finite "
           "differences of f + yᵀg)\n";
    auto L = [&](crvec x) {
        te_problem.eval_constraints(x, gx);
        return te_problem.eval_objective(x) + gx.dot(y);
    };
    auto [Lx, fd_grad_L] = finite_diff(L, x);
    vec grad_L(n);
    te_problem.eval_lagrangian_gradient(x, y, grad_L, wn);
    print_compare(grad_L, fd_grad_L);

    log << "Gradient verification: ∇ψ(x) (grad_ψ compared to finite "
           "differences of ψ)\n";
    auto ψ = [&](crvec x) {
        return te_problem.eval_augmented_lagrangian(x, y, Σ, wm);
    };
    auto [ψx, fd_grad_ψ] = finite_diff(ψ, x);
    vec grad_ψ(n);
    te_problem.eval_augmented_lagrangian_gradient(x, y, Σ, grad_ψ, wn, wm);
    print_compare(grad_ψ, fd_grad_ψ);

    log << "Gradient verification: ∇ψ(x) (grad_ψ compared to reference "
           "implementation based on g, ∇f, ∇g)\n";
    vec grad_ψ_default(n);
    auto ψ_default = default_eval_augmented_lagrangian_and_gradient(
        te_problem, x, y, Σ, grad_ψ_default, wn, wm);
    print_compare(grad_ψ, grad_ψ_default);
    log << "Function verification: ψ(x) (ψ compared to reference "
           "implementation based on f, g)\n";
    print_compare_scal(ψx, ψ_default);

    if (te_problem.provides_eval_augmented_lagrangian_and_gradient()) {
        log << "Gradient verification: ∇ψ(x) (grad_ψ compared to ψ_grad_ψ)\n";
        vec ψ_grad_ψ(n);
        real_t ψ2 = te_problem.eval_augmented_lagrangian_and_gradient(
            x, y, Σ, ψ_grad_ψ, wn, wm);
        print_compare(grad_ψ, ψ_grad_ψ);
        log << "Function verification: ψ(x) (ψ compared to ψ_grad_ψ)\n";
        print_compare_scal(ψx, ψ2);
    }

    if (te_problem.provides_eval_lagrangian_hessian_product()) {
        log << "Hessian product verification: ∇²L(x) (hess_L_prod compared to "
               "finite differences of grad_L)\n";
        vec grad_Lv(n);
        vec xv = x + v;
        te_problem.eval_lagrangian_gradient(xv, y, grad_Lv, wn);
        vec fd_hess_Lv = grad_Lv - grad_L;
        vec hess_Lv(n);
        te_problem.eval_lagrangian_hessian_product(x, y, 1, v, hess_Lv);
        print_compare(hess_Lv, fd_hess_Lv);
    }

    if (te_problem.provides_eval_augmented_lagrangian_hessian_product()) {
        log << "Hessian product verification: ∇²ψ(x) (hess_ψ_prod compared to "
               "finite differences of grad_ψ)\n";
        vec grad_ψv(n);
        vec xv = x + v;
        te_problem.eval_augmented_lagrangian_gradient(xv, y, Σ, grad_ψv, wn,
                                                      wm);
        vec fd_hess_ψv = grad_ψv - grad_ψ;
        vec hess_ψv(n);
        te_problem.eval_augmented_lagrangian_hessian_product(x, y, Σ, 1, v,
                                                             hess_ψv);
        print_compare(hess_ψv, fd_hess_ψv);
    }

    if (opts.hessians && te_problem.provides_eval_lagrangian_hessian()) {
        log << "Hessian verification: ∇²L(x) (hess_L compared to finite "
               "differences of grad_L)\n";
        namespace sp  = alpaqa::sparsity;
        auto sparsity = te_problem.get_lagrangian_hessian_sparsity();
        sp::SparsityConverter<sp::Sparsity<config_t>, sp::Dense<config_t>> cvt{
            sparsity};
        mat hess_L(n, n);
        auto eval_h = [&](rvec v) {
            te_problem.eval_lagrangian_hessian(x, y, 1., v);
        };
        cvt.convert_values(eval_h, hess_L.reshaped());
        mat fd_hess_L = finite_diff_hess(
            [&](crvec x, rvec g) {
                te_problem.eval_lagrangian_gradient(x, y, g, wn);
            },
            x);
        print_compare(hess_L, fd_hess_L);
    }

    if (opts.hessians &&
        te_problem.provides_eval_augmented_lagrangian_hessian()) {
        log << "Hessian verification: ∇²ψ(x) (hess_ψ compared to finite "
               "differences of grad_ψ)\\n";
        namespace sp  = alpaqa::sparsity;
        auto sparsity = te_problem.get_augmented_lagrangian_hessian_sparsity();
        sp::SparsityConverter<sp::Sparsity<config_t>, sp::Dense<config_t>> cvt{
            sparsity};
        mat hess_ψ(n, n);
        auto eval_h = [&](rvec v) {
            te_problem.eval_augmented_lagrangian_hessian(x, y, Σ, 1., v);
        };
        cvt.convert_values(eval_h, hess_ψ.reshaped());
        mat fd_hess_ψ = finite_diff_hess(
            [&](crvec x, rvec g) {
                te_problem.eval_augmented_lagrangian_gradient(x, y, Σ, g, wn,
                                                              wm);
            },
            x);
        print_compare(hess_ψ, fd_hess_ψ);
    }
}
