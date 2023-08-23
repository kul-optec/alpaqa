#include <sparse-logistic-regression-export.h>

#include <alpaqa/config/config.hpp>
#include <alpaqa/dl/dl-problem.h>
#include <alpaqa/params/params.hpp>
#include <alpaqa/util/io/csv.hpp>
USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);

#include <algorithm>
#include <any>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
namespace fs = std::filesystem;

struct Problem {
    alpaqa_problem_functions_t funcs{};
    length_t n;         ///< Number of features
    length_t m;         ///< Number of data points
    real_t λ;           ///< Regularization factor
    real_t μ;           ///< Scaling factor
    mat A;              ///< Data matrix (m×n)
    vec b;              ///< Binary labels (m)
    vec Aᵀb;            ///< Work vector (n)
    mutable vec Ax;     ///< Work vector (m)
    fs::path data_file; ///< File we loaded the data from

    /// φ(x) = ∑ ln(1 + exp(-b x))
    real_t logistic_loss(crvec x) const {
        auto &&xa = x.array();
        auto &&ba = b.array();
        return ((-ba * xa).exp() + 1).log().sum();
    }

    /// -dφ(x)/dx = b / (exp(b x) + 1)
    void neg_deriv_logistic_loss(crvec x, rvec g) const {
        auto &&xa = x.array();
        auto &&ba = b.array();
        g         = ba / ((ba * xa).exp() + 1);
    }

    /// σ₂(x) = b exp(b x) / (exp(b x) + 1)²
    /// @note Assumes that b is a binary vector (i.e. b² = b).
    void sigmoid2(crvec x, rvec g) const {
        auto &&xa = x.array();
        auto &&ba = b.array();
        g         = (ba * xa).exp();
        g         = ba * g.array() / ((g.array() + 1) * (g.array() + 1));
    }

    /// Objective function.
    real_t eval_f(const real_t *x_) const {
        cmvec x{x_, n};
        Ax.noalias() = A * x;
        return μ * logistic_loss(Ax);
    }

    /// Gradient of objective.
    void eval_grad_f(const real_t *x_, real_t *g_) const {
        cmvec x{x_, n};
        mvec g{g_, n};
        Ax.noalias() = A * x;
        // ∇(f∘A)(x) = Aᵀ∇f(Ax)
        neg_deriv_logistic_loss(Ax, Ax);         // Ax ← -∇f(Ax)
        g.noalias() = -μ * (A.transpose() * Ax); // g ← μAᵀ∇f(Ax)
    }

    /// Hessian-vector product of objective.
    void eval_hess_f_prod(const real_t *x_, const real_t *v_,
                          real_t *Hv_) const {
        cmvec x{x_, n};
        cmvec v{v_, n};
        mvec Hv{Hv_, n};
        Ax.noalias() = A * x;
        sigmoid2(Ax, Ax);
        Ax.noalias() = Ax.asDiagonal() * (A * v);
        Hv.noalias() = μ * (A.transpose() * Ax);
    }

    /// Hessian of objective.
    void eval_hess_f(const real_t *x_, real_t *H_) const {
        cmvec x{x_, n};
        mmat H{H_, n, n};
        Ax.noalias() = A * x;
        sigmoid2(Ax, Ax);
        H.noalias() = A.transpose() * (μ * Ax.asDiagonal()) * A;
    }

    /// Hessian-vector product of Lagrangian.
    void eval_hess_L_prod(const real_t *x, [[maybe_unused]] const real_t *y,
                          real_t scale, const real_t *v, real_t *Hv) const {
        eval_hess_f_prod(x, v, Hv);
        if (scale != 1)
            mvec{Hv, n} *= scale;
    }

    /// Hessian-vector product of augmented Lagrangian.
    void eval_hess_ψ_prod(const real_t *x, const real_t *y,
                          [[maybe_unused]] const real_t *Σ, real_t scale,
                          [[maybe_unused]] const real_t *zl,
                          [[maybe_unused]] const real_t *zu, const real_t *v,
                          real_t *Hv) const {
        eval_hess_L_prod(x, y, scale, v, Hv);
    }

    /// Hessian of Lagrangian.
    void eval_hess_L(const real_t *x, [[maybe_unused]] const real_t *y,
                     real_t scale, [[maybe_unused]] index_t *inner_idx,
                     [[maybe_unused]] index_t *outer_ptr, real_t *H) const {
        eval_hess_f(x, H);
        if (scale != 1)
            mmat{H, n, n} *= scale;
    }

    /// Hessian of augmented Lagrangian.
    void eval_hess_ψ(const real_t *x, const real_t *y,
                     [[maybe_unused]] const real_t *Σ, real_t scale,
                     [[maybe_unused]] const real_t *zl,
                     [[maybe_unused]] const real_t *zu,
                     [[maybe_unused]] index_t *inner_idx,
                     [[maybe_unused]] index_t *outer_ptr, real_t *H) const {
        eval_hess_L(x, y, scale, inner_idx, outer_ptr, H);
    }

    /// Both the objective and its gradient.
    real_t eval_f_grad_f(const real_t *x_, real_t *g_) const {
        cmvec x{x_, n};
        mvec g{g_, n};
        Ax.noalias() = A * x;
        real_t f     = μ * logistic_loss(Ax);
        // ∇(f∘A)(x) = Aᵀ∇f(Ax)
        neg_deriv_logistic_loss(Ax, Ax);         // Ax ← -∇f(Ax)
        g.noalias() = -μ * (A.transpose() * Ax); // g ← μAᵀ∇f(Ax)
        return f;
    }

    /// Constraints function (unconstrained).
    void eval_g(const real_t *, real_t *) const {}

    /// Gradient-vector product of constraints.
    void eval_grad_g_prod(const real_t *, const real_t *, real_t *gr_) const {
        mvec{gr_, n}.setZero();
    }

    /// Jacobian of constraints.
    void eval_jac_g(const real_t *, index_t *, index_t *, real_t *) const {}

    /// ℓ₁-regularization term.
    void initialize_l1_reg(real_t *lambda, length_t *size) const {
        if (!lambda) {
            *size = 1;
        } else {
            assert(*size == 1);
            *lambda = λ;
        }
    }

    /// Loads classification data from a CSV file.
    /// The first row contains the number of data points and the number of
    /// features, separated by a space.
    /// The second row contains the binary labels for all data points.
    /// Every following row contains the values of one feature for all data
    /// points.
    void load_data() {
        std::ifstream csv_file{data_file};
        if (!csv_file)
            throw std::runtime_error("Unable to open file '" +
                                     data_file.string() + "'");
        // Load dimensions (#data points, #features)
        csv_file >> m >> n;
        csv_file.ignore(1, '\n');
        if (!csv_file)
            throw std::runtime_error(
                "Unable to read dimensions from data file");
        b.resize(m);
        A.resize(m, n);
        Aᵀb.resize(n);
        Ax.resize(m);
        // Read the target labels
        alpaqa::csv::read_row(csv_file, b);
        // Read the data
        for (length_t i = 0; i < n; ++i)
            alpaqa::csv::read_row(csv_file, A.col(i));
    }

    std::string get_name() const {
        return "sparse logistic regression (\"" + data_file.string() + "\")";
    }

    /// Constructor loads CSV data file and exposes the problem functions by
    /// initializing the @c funcs member.
    Problem(fs::path csv_filename, real_t λ_factor)
        : data_file(std::move(csv_filename)) {
        load_data();
        Aᵀb.noalias() = A.transpose() * b;
        real_t λ_max = Aᵀb.lpNorm<Eigen::Infinity>() / static_cast<real_t>(m);
        λ            = λ_factor * λ_max;
        μ            = 1. / static_cast<real_t>(m);

        using P = Problem;
        using alpaqa::member_caller;
        funcs.n                = n;
        funcs.m                = 0;
        funcs.eval_f           = member_caller<&P::eval_f>();
        funcs.eval_grad_f      = member_caller<&P::eval_grad_f>();
        funcs.eval_f_grad_f    = member_caller<&P::eval_f_grad_f>();
        funcs.eval_g           = member_caller<&P::eval_g>();
        funcs.eval_grad_g_prod = member_caller<&P::eval_grad_g_prod>();
        funcs.eval_jac_g       = member_caller<&P::eval_jac_g>();
        funcs.eval_hess_L_prod = member_caller<&P::eval_hess_L_prod>();
        funcs.eval_hess_ψ_prod = member_caller<&P::eval_hess_ψ_prod>();
        funcs.eval_hess_L      = member_caller<&P::eval_hess_L>();
        funcs.eval_hess_ψ      = member_caller<&P::eval_hess_ψ>();
        if (λ > 0)
            funcs.initialize_l1_reg = member_caller<&P::initialize_l1_reg>();
    }
};

/// Main entry point of this file, it is called by the
/// @ref alpaqa::dl::DLProblem class.
extern "C" SPARSE_LOGISTIC_REGRESSION_EXPORT alpaqa_problem_register_t
alpaqa_problem_register(void *user_data_v) noexcept try {
    // Check and convert user arguments
    if (!user_data_v)
        throw std::invalid_argument("Missing user data");
    const auto &user_data = *reinterpret_cast<std::any *>(user_data_v);
    using param_t         = std::span<std::string_view>;
    auto opts             = std::any_cast<param_t>(user_data);
    std::vector<unsigned> used(opts.size());
    // CSV file to load dataset from
    std::string_view datafilename;
    alpaqa::params::set_params(datafilename, "datafile", opts, used);
    if (datafilename.empty())
        throw std::invalid_argument("Missing option problem.datafile");
    // Regularization factor
    real_t λ_factor = 0.1;
    alpaqa::params::set_params(λ_factor, "λ_factor", opts, used);
    // Check any unused options
    auto unused_opt = std::find(used.begin(), used.end(), 0);
    auto unused_idx = static_cast<size_t>(unused_opt - used.begin());
    if (unused_opt != used.end())
        throw std::invalid_argument("Unused problem option: " +
                                    std::string(opts[unused_idx]));
    // Build and expose problem
    auto problem = std::make_unique<Problem>(datafilename, λ_factor);
    alpaqa_problem_register_t result;
    alpaqa::register_member_function(result, "get_name", &Problem::get_name);
    result.functions = &problem->funcs;
    result.instance  = problem.release();
    result.cleanup   = [](void *instance) {
        delete reinterpret_cast<Problem *>(instance);
    };
    return result;
} catch (...) {
    return {.exception = new alpaqa_exception_ptr_t{std::current_exception()}};
}
