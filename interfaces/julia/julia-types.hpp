#pragma once

#include <alpaqa/config/config.hpp>

#include <cstddef>
#include <limits>

namespace alpaqa {
struct LoadedProblem;

/// Structs in this namespace should have matching layouts with the
/// corresponding structs on the Julia side.
namespace jl ::julia {

/// This struct is passed to the Julia inner solver implementation, and enables
/// calling of the C++ problem functions from Julia. Its layout matches that of
/// the corresponding struct in Julia.
struct SubProblem {
    const void *data = nullptr;
    double (*value_and_gradient)(const void *data, const double *x, size_t nx,
                                 double *grad_f){nullptr};
    double (*prox)(const void *data, double gamma, const double *x, size_t nx,
                   double *prox_h){nullptr};
    void (*hvp)(const void *data, const double *x, size_t nx, const double *v,
                double *Hv){nullptr};
    void (*jprox)(const void *data, double gamma, const double *x, size_t nx,
                  double *J_diag){nullptr};
    void (*hess)(const void *data, const double *x, size_t nx,
                 double *H){nullptr};
    double (*value)(const void *data, const double *x, size_t nx){nullptr};
    void (*gradient)(const void *data, const double *x, size_t nx,
                     double *grad_f){nullptr};
};

struct JuliaInnerSolverResult {
    bool success        = false;
    int num_iter        = 0;
    double achieved_tol = std::numeric_limits<double>::infinity();
};

struct InnerSolverFunction {
    void *context = nullptr;
    JuliaInnerSolverResult (*function)(
        void *context,             ///< [inout] Julia closure data
        const SubProblem &problem, ///< [in] Problem data and functions
        double *x, ///< [inout] Initial guess on input, solution on return
        size_t nx, ///< [in] Number of elements of x
        double tol ///< [in] Tolerance for the inner solver
    ){nullptr};
};

struct EvalCounter {
    unsigned projecting_difference_constraints;
    unsigned projection_multipliers;
    unsigned proximal_gradient_step;
    unsigned inactive_indices_res_lna;
    unsigned prox_jacobian_diag;
    unsigned nonsmooth_objective;
    unsigned objective;
    unsigned objective_gradient;
    unsigned objective_and_gradient;
    unsigned objective_and_constraints;
    unsigned objective_gradient_and_constraints_gradient_product;
    unsigned constraints;
    unsigned constraints_gradient_product;
    unsigned grad_gi;
    unsigned constraints_jacobian;
    unsigned lagrangian_gradient;
    unsigned lagrangian_hessian_product;
    unsigned lagrangian_hessian;
    unsigned augmented_lagrangian_hessian_product;
    unsigned augmented_lagrangian_hessian;
    unsigned augmented_lagrangian;
    unsigned augmented_lagrangian_gradient;
    unsigned augmented_lagrangian_and_gradient;
};

struct SolverReturn {
    bool success;
    EvalCounter evaluations;
    unsigned num_inner_iter;
    unsigned num_outer_iter;
    unsigned num_outer_fail;
};

struct DriverReturn {
    bool success;
    EvalCounter evaluations;
    unsigned num_inner_iter;
    unsigned num_outer_iter;
    char *output_file_id;
    char *error;
};

} // namespace jl::julia
} // namespace alpaqa
