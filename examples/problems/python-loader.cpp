#include <pybind11/detail/common.h>
#include <python-loader/export.h>

#include <alpaqa/config/config.hpp>
#include <alpaqa/dl/dl-problem.h>
#include <alpaqa/params/params.hpp>
#include <guanaqo/string-util.hpp>

#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <pybind11/eigen.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
namespace py = pybind11;

namespace alpaqa {

struct PYTHON_LOADER_EXPORT python_loader_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

namespace {

USING_ALPAQA_CONFIG(DefaultConfig);

struct PYTHON_LOADER_NO_EXPORT Problem {
    alpaqa_problem_functions_t funcs{};
    py::scoped_interpreter python_interpreter;
    std::string ref;
    std::string name;
    struct ObjWrapper {
        std::optional<py::object> o;
        operator py::object &() { return o.value(); }
        operator const py::object &() const { return o.value(); }
        ObjWrapper() = default;
        ObjWrapper &operator=(py::object &&o) {
            this->o = std::move(o);
            return *this;
        }
        ObjWrapper(const ObjWrapper &)            = delete;
        ObjWrapper &operator=(const ObjWrapper &) = delete;
        ~ObjWrapper() {
            if (o) {
                py::gil_scoped_acquire gil;
                o.reset();
            }
        }
        template <class... Args>
        decltype(auto) attr(Args &&...args) {
            return o.value().attr(std::forward<Args>(args)...);
        }
    } o;

    Problem(std::string_view ref, std::span<const std::string_view> opts)
        : ref{ref} {
        py::gil_scoped_acquire gil;
        try {
            auto [mod_name, attr_name] = guanaqo::split(ref, ":");
            auto mod = py::module_::import(std::string(mod_name).c_str());
            std::string attr_name_str{attr_name};
            auto constructor = mod.attr(attr_name_str.c_str());
            py::list constr_args;
            for (auto a : opts)
                constr_args.append(py::str(a));
            o    = constructor(constr_args);
            name = std::string{py::str(o)} + " (from " + this->ref + ")";

            using P = Problem;
            using alpaqa::member_caller;
            funcs.n              = py::cast<length_t>(o.attr("n"));
            funcs.m              = py::cast<length_t>(o.attr("m"));
            funcs.name           = name.c_str();
            funcs.eval_objective = member_caller<&P::eval_objective>();
            funcs.eval_objective_gradient =
                member_caller<&P::eval_objective_gradient>();
            funcs.eval_constraints = member_caller<&P::eval_constraints>();
            funcs.eval_constraints_gradient_product =
                member_caller<&P::eval_constraints_gradient_product>();
            // clang-format off
            if (py::hasattr(o, "eval_projecting_difference_constraints"))
                funcs.eval_projecting_difference_constraints = member_caller<&P::eval_projecting_difference_constraints>();
            if (py::hasattr(o, "eval_projection_multipliers"))
                funcs.eval_projection_multipliers = member_caller<&P::eval_projection_multipliers>();
            if (py::hasattr(o, "eval_proximal_gradient_step"))
                funcs.eval_proximal_gradient_step = member_caller<&P::eval_proximal_gradient_step>();
            if (py::hasattr(o, "eval_inactive_indices_res_lna"))
                funcs.eval_inactive_indices_res_lna = member_caller<&P::eval_inactive_indices_res_lna>();
            if (py::hasattr(o, "eval_constraints_jacobian"))
                funcs.eval_constraints_jacobian = member_caller<&P::eval_constraints_jacobian>();
            if (py::hasattr(o, "eval_grad_gi"))
                funcs.eval_grad_gi = member_caller<&P::eval_grad_gi>();
            if (py::hasattr(o, "eval_objective_and_gradient"))
                funcs.eval_objective_and_gradient = member_caller<&P::eval_objective_and_gradient>();
            if (py::hasattr(o, "eval_objective_and_constraints"))
                funcs.eval_objective_and_constraints = member_caller<&P::eval_objective_and_constraints>();
            if (py::hasattr(o, "eval_objective_gradient_and_constraints_gradient_product"))
                funcs.eval_objective_gradient_and_constraints_gradient_product = member_caller<&P::eval_objective_gradient_and_constraints_gradient_product>();
            if (py::hasattr(o, "eval_lagrangian_gradient"))
                funcs.eval_lagrangian_gradient = member_caller<&P::eval_lagrangian_gradient>();
            if (py::hasattr(o, "eval_augmented_lagrangian"))
                funcs.eval_augmented_lagrangian = member_caller<&P::eval_augmented_lagrangian>();
            if (py::hasattr(o, "eval_augmented_lagrangian_gradient"))
                funcs.eval_augmented_lagrangian_gradient = member_caller<&P::eval_augmented_lagrangian_gradient>();
            if (py::hasattr(o, "eval_augmented_lagrangian_and_gradient"))
                funcs.eval_augmented_lagrangian_and_gradient = member_caller<&P::eval_augmented_lagrangian_and_gradient>();
            if (py::hasattr(o, "eval_objective_and_gradient"))
                funcs.eval_objective_and_gradient = member_caller<&P::eval_objective_and_gradient>();
            if (py::hasattr(o, "eval_lagrangian_hessian_product"))
                funcs.eval_lagrangian_hessian_product = member_caller<&P::eval_lagrangian_hessian_product>();
            if (py::hasattr(o, "eval_lagrangian_hessian"))
                funcs.eval_lagrangian_hessian = member_caller<&P::eval_lagrangian_hessian>();
            if (py::hasattr(o, "eval_augmented_lagrangian_hessian_product"))
                funcs.eval_augmented_lagrangian_hessian_product = member_caller<&P::eval_augmented_lagrangian_hessian_product>();
            if (py::hasattr(o, "eval_augmented_lagrangian_hessian"))
                funcs.eval_augmented_lagrangian_hessian = member_caller<&P::eval_augmented_lagrangian_hessian>();
            if (py::hasattr(o, "C"))
                funcs.initialize_box_C = member_caller<&P::initialize_box_C>();
            if (py::hasattr(o, "D"))
                funcs.initialize_box_D = member_caller<&P::initialize_box_D>();
            if (py::hasattr(o, "l1_reg"))
                funcs.initialize_l1_reg = member_caller<&P::initialize_l1_reg>();
            // clang-format on
        } catch (py::error_already_set &e) {
            // Needs to be caught between the interpreter is destroyed
            throw python_loader_error(e.what());
        }
    }

    cmvec vecn(const real_t *x) const { return cmvec{x, funcs.n}; }
    mvec vecn(real_t *x) const { return mvec{x, funcs.n}; }
    mindexvec vecn(index_t *x) const { return mindexvec{x, funcs.n}; }
    cmvec vecm(const real_t *x) const { return cmvec{x, funcs.m}; }
    mvec vecm(real_t *x) const { return mvec{x, funcs.m}; }

    template <class... Args>
    decltype(auto) call_func(const char *name, Args &&...args) {
        py::gil_scoped_acquire gil;
        try {
            return o.attr(name)(std::forward<Args>(args)...);
        } catch (py::error_already_set &e) {
            throw python_loader_error(e.what());
        }
    }

    /// Cost function.
    /// @see @ref alpaqa::TypeErasedProblem::eval_objective()
    real_t eval_objective(const real_t *x) {
        return py::cast<real_t>(call_func("eval_objective", vecn(x)));
    }
    /// Gradient of the cost function.
    /// @see @ref alpaqa::TypeErasedProblem::eval_objective_gradient()
    void eval_objective_gradient(const real_t *x, real_t *grad_fx) {
        call_func("eval_objective_gradient", vecn(x), vecn(grad_fx));
    }
    /// Constraints function.
    /// @see @ref alpaqa::TypeErasedProblem::eval_constraints()
    void eval_constraints(const real_t *x, real_t *gx) {
        call_func("eval_constraints", vecn(x), vecm(gx));
    }
    /// Gradient-vector product of the constraints function.
    /// @see @ref alpaqa::TypeErasedProblem::eval_constraints_gradient_product()
    void eval_constraints_gradient_product(const real_t *x, const real_t *y,
                                           real_t *grad_gxy) {
        call_func("eval_constraints_gradient_product", vecn(x), vecm(y),
                  vecn(grad_gxy));
    }

    /// Difference between point and its projection onto the general constraint
    /// set D.
    /// @see @ref alpaqa::TypeErasedProblem::eval_projecting_difference_constraints()
    void eval_projecting_difference_constraints(const real_t *z, real_t *e) {
        call_func("eval_projecting_difference_constraints", vecm(z), vecm(e));
    }
    /// Project the Lagrange multipliers.
    /// @see @ref alpaqa::TypeErasedProblem::eval_projection_multipliers()
    void eval_projection_multipliers(real_t *y, real_t M) {
        call_func("eval_projection_multipliers", vecm(y), M);
    }
    /// Proximal gradient step.
    /// @see @ref alpaqa::TypeErasedProblem::eval_proximal_gradient_step()
    /// If not set, the default implementation from
    /// @ref alpaqa::BoxConstrProblem is used.
    real_t eval_proximal_gradient_step(real_t γ, const real_t *x,
                                       const real_t *grad_ψ, real_t *x̂,
                                       real_t *p) {
        return py::cast<real_t>(call_func("eval_proximal_gradient_step", γ,
                                          vecn(x), vecn(grad_ψ), vecn(x̂),
                                          vecn(p)));
    }
    /// Active indices for proximal operator.
    /// @see @ref alpaqa::TypeErasedProblem::eval_inactive_indices_res_lna()
    /// If not set, the default implementation from
    /// @ref alpaqa::BoxConstrProblem is used.
    index_t eval_inactive_indices_res_lna(real_t γ, const real_t *x,
                                          const real_t *grad_ψ, index_t *J) {
        return py::cast<index_t>(call_func("eval_inactive_indices_res_lna", γ,
                                           vecn(x), vecn(grad_ψ), vecn(J)));
    }

    /// Jacobian of the constraints function.
    /// @see @ref alpaqa::TypeErasedProblem::eval_constraints_jacobian()
    void eval_constraints_jacobian(const real_t *x, real_t *J_values) {
        call_func("eval_constraints_jacobian", vecn(x),
                  mmat{J_values, funcs.m, funcs.n});
    }
#if 0
    /// Get the sparsity pattern of the Jacobian of the constraints function.
    /// @see @ref alpaqa::TypeErasedProblem::get_constraints_jacobian_sparsity()
    alpaqa_sparsity_t get_constraints_jacobian_sparsity() {
        // TODO: support sparse matrices
    }
#endif
    /// Gradient of specific constraint function.
    /// @see @ref alpaqa::TypeErasedProblem::eval_grad_gi()
    void eval_grad_gi(const real_t *x, index_t i, real_t *grad_gi) {
        call_func("eval_grad_gi", vecn(x), i, vecn(grad_gi));
    }
    /// Hessian-vector product of the Lagrangian.
    /// @see @ref alpaqa::TypeErasedProblem::eval_lagrangian_hessian_product()
    void eval_lagrangian_hessian_product(const real_t *x, const real_t *y,
                                         real_t scale, const real_t *v,
                                         real_t *Hv) {
        call_func("eval_lagrangian_hessian_product", vecn(x), vecm(y), scale,
                  vecn(v), vecn(Hv));
    }
    /// Hessian of the Lagrangian.
    /// @see @ref alpaqa::TypeErasedProblem::eval_lagrangian_hessian()
    void eval_lagrangian_hessian(const real_t *x, const real_t *y, real_t scale,
                                 real_t *H_values) {
        call_func("eval_lagrangian_hessian", vecn(x), vecm(y), scale,
                  mmat{H_values, funcs.n, funcs.n});
    }
#if 0
    /// Get the sparsity pattern of the Hessian of the Lagrangian.
    /// @see @ref alpaqa::TypeErasedProblem::get_lagrangian_hessian_sparsity()
    alpaqa_sparsity_t get_lagrangian_hessian_sparsity() {
        // TODO: support sparse matrices
    }
#endif
    /// Hessian-vector product of the augmented Lagrangian.
    /// @see @ref alpaqa::TypeErasedProblem::eval_augmented_lagrangian_hessian_product()
    void eval_augmented_lagrangian_hessian_product(
        const real_t *x, const real_t *y, const real_t *Σ, real_t scale,
        [[maybe_unused]] const real_t *zl, [[maybe_unused]] const real_t *zu,
        const real_t *v, real_t *Hv) {
        call_func("eval_augmented_lagrangian_hessian_product", vecn(x), vecm(y),
                  vecm(Σ), scale, vecn(v), vecn(Hv));
    }
    /// Hessian of the augmented Lagrangian.
    /// @see @ref alpaqa::TypeErasedProblem::eval_augmented_lagrangian_hessian()
    void eval_augmented_lagrangian_hessian(const real_t *x, const real_t *y,
                                           const real_t *Σ, real_t scale,
                                           [[maybe_unused]] const real_t *zl,
                                           [[maybe_unused]] const real_t *zu,
                                           real_t *H_values) {
        call_func("eval_augmented_lagrangian_hessian", vecn(x), vecm(y),
                  vecm(Σ), scale, mmat{H_values, funcs.n, funcs.n});
    }
#if 0
    /// Get the sparsity pattern of the Hessian of the augmented Lagrangian.
    /// @see @ref alpaqa::TypeErasedProblem::get_augmented_lagrangian_hessian_sparsity()
    alpaqa_sparsity_t get_augmented_lagrangian_hessian_sparsity() {
        // TODO: support sparse matrices
    }
#endif

    /// Cost and its gradient.
    /// @see @ref alpaqa::TypeErasedProblem::eval_objective_and_gradient()
    real_t eval_objective_and_gradient(const real_t *x, real_t *grad_fx) {
        return py::cast<real_t>(
            call_func("eval_objective_and_gradient", vecn(x), vecn(grad_fx)));
    }
    /// Cost and constraints.
    /// @see @ref alpaqa::TypeErasedProblem::eval_objective_and_constraints()
    real_t eval_objective_and_constraints(const real_t *x, real_t *g) {
        return py::cast<real_t>(
            call_func("eval_objective_and_constraints", vecn(x), vecm(g)));
    }
    /// Gradient of the cost and gradient-vector product of the constraints.
    /// @see @ref alpaqa::TypeErasedProblem::eval_objective_gradient_and_constraints_gradient_product()
    void eval_objective_gradient_and_constraints_gradient_product(
        const real_t *x, const real_t *y, real_t *grad_f, real_t *grad_gxy) {
        call_func("eval_objective_gradient_and_constraints_gradient_product",
                  vecn(x), vecm(y), vecn(grad_f), vecn(grad_gxy));
    }
    /// Gradient of the Lagrangian.
    /// @see @ref alpaqa::TypeErasedProblem::eval_lagrangian_gradient()
    void eval_lagrangian_gradient(const real_t *x, const real_t *y,
                                  real_t *grad_L, real_t *work_n) {
        call_func("eval_lagrangian_gradient", vecn(x), vecm(y), vecn(grad_L),
                  vecn(work_n));
    }

    /// Augmented Lagrangian.
    /// @see @ref alpaqa::TypeErasedProblem::eval_augmented_lagrangian()
    real_t eval_augmented_lagrangian(const real_t *x, const real_t *y,
                                     const real_t *Σ,
                                     [[maybe_unused]] const real_t *zl,
                                     [[maybe_unused]] const real_t *zu,
                                     real_t *ŷ) {
        return py::cast<real_t>(call_func("eval_augmented_lagrangian", vecn(x),
                                          vecm(y), vecm(Σ), vecm(ŷ)));
    }
    /// Gradient of the augmented Lagrangian.
    /// @see @ref alpaqa::TypeErasedProblem::eval_augmented_lagrangian_gradient()
    void eval_augmented_lagrangian_gradient(const real_t *x, const real_t *y,
                                            const real_t *Σ,
                                            [[maybe_unused]] const real_t *zl,
                                            [[maybe_unused]] const real_t *zu,
                                            real_t *grad_ψ, real_t *work_n,
                                            real_t *work_m) {
        call_func("eval_augmented_lagrangian_gradient", vecn(x), vecm(y),
                  vecm(Σ), vecn(grad_ψ), vecn(work_n), vecn(work_m));
    }
    /// Augmented Lagrangian and its gradient.
    /// @see @ref alpaqa::TypeErasedProblem::eval_augmented_lagrangian_and_gradient()
    real_t eval_augmented_lagrangian_and_gradient(
        const real_t *x, const real_t *y, const real_t *Σ,
        [[maybe_unused]] const real_t *zl, [[maybe_unused]] const real_t *zu,
        real_t *grad_ψ, real_t *work_n, real_t *work_m) {
        return py::cast<real_t>(call_func(
            "eval_augmented_lagrangian_and_gradient", vecn(x), vecm(y), vecm(Σ),
            vecn(grad_ψ), vecn(work_n), vecn(work_m)));
    }

    /// Provide the initial values for the bounds of
    /// @ref alpaqa::BoxConstrProblem::C, i.e. the constraints on the decision
    /// variables.
    void initialize_box_C(real_t *lb, real_t *ub) {
        py::gil_scoped_acquire gil;
        try {
            auto C    = py::cast<py::tuple>(o.attr("C"));
            auto C_lb = py::cast<crvec>(C[0]);
            auto C_ub = py::cast<crvec>(C[1]);
            if (C_lb.size() != funcs.n || C_ub.size() != funcs.n)
                throw py::value_error("Invalid dimensions box C");
            vecn(lb) = C_lb;
            vecn(ub) = C_ub;
        } catch (py::error_already_set &e) {
            throw python_loader_error(e.what());
        }
    }
    /// Provide the initial values for the bounds of
    /// @ref alpaqa::BoxConstrProblem::D, i.e. the general constraints.
    void initialize_box_D(real_t *lb, real_t *ub) {
        py::gil_scoped_acquire gil;
        try {
            auto D    = py::cast<py::tuple>(o.attr("D"));
            auto D_lb = py::cast<crvec>(D[0]);
            auto D_ub = py::cast<crvec>(D[1]);
            if (D_lb.size() != funcs.n || D_ub.size() != funcs.n)
                throw py::value_error("Invalid dimensions box D");
            vecn(lb) = D_lb;
            vecn(ub) = D_ub;
        } catch (py::error_already_set &e) {
            throw python_loader_error(e.what());
        }
    }
    /// Provide the initial values for @ref alpaqa::BoxConstrProblem::l1_reg,
    /// the ℓ₁-regularization factor.
    /// This function is called twice:
    ///  1. with @p lambda set to `nullptr`, and the user should set the size.
    ///  2. with @p lambda pointing to an array of that size, and the user
    ///     should initialize this array.
    void initialize_l1_reg(real_t *lambda, alpaqa_length_t *size) {
        py::gil_scoped_acquire gil;
        try {
            if (lambda) {
                auto lambda_py = py::cast<crvec>(o.attr("l1_reg"));
                if (lambda_py.size() != *size)
                    throw py::value_error("Invalid dimensions l1_reg");
                mvec{lambda, *size} = lambda_py;
            } else {
                auto lambda_py = py::cast<crvec>(o.attr("l1_reg"));
                *size          = lambda_py.size();
            }
        } catch (py::error_already_set &e) {
            throw python_loader_error(e.what());
        }
    }
};

} // namespace

} // namespace alpaqa

/// Main entry point of this file, it is called by the
/// @ref alpaqa::dl::DLProblem class.
extern "C" PYTHON_LOADER_EXPORT alpaqa_problem_register_t
register_alpaqa_problem(alpaqa_register_arg_t user_data_v) noexcept try {
    // Check and convert user arguments
    if (!user_data_v.data)
        throw std::invalid_argument("Missing user data");
    if (user_data_v.type != alpaqa_register_arg_strings)
        throw std::invalid_argument("Invalid user data type");
    using param_t    = std::span<std::string_view>;
    const auto &opts = *reinterpret_cast<param_t *>(user_data_v.data);
    // Python module and reference to load
    std::string_view ref;
    alpaqa::params::set_params(ref, "ref", opts);
    if (ref.empty())
        throw std::invalid_argument(
            "Missing option problem.ref=package.module:Class");
    // Build and expose problem
    auto problem = std::make_unique<alpaqa::Problem>(ref, opts);
    alpaqa_problem_register_t result;
    result.functions = &problem->funcs;
    result.instance  = problem.release();
    result.cleanup   = [](void *instance) {
        delete static_cast<alpaqa::Problem *>(instance);
    };
    return result;
} catch (...) {
    return {.exception = new alpaqa_exception_ptr_t{std::current_exception()}};
}

/// Returns the alpaqa DL ABI version. This version is verified for
/// compatibility by the @ref alpaqa::dl::DLProblem constructor before
/// registering the problem.
extern "C" PYTHON_LOADER_EXPORT alpaqa_dl_abi_version_t
register_alpaqa_problem_version() {
    return ALPAQA_DL_ABI_VERSION;
}
