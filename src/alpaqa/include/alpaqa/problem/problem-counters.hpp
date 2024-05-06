#pragma once

#include <alpaqa/export.h>

#include <chrono>
#include <iosfwd>

namespace alpaqa {

struct EvalCounter {
    unsigned projecting_difference_constraints{};
    unsigned projection_multipliers{};
    unsigned proximal_gradient_step{};
    unsigned inactive_indices_res_lna{};
    unsigned objective{};
    unsigned objective_gradient{};
    unsigned objective_and_gradient{};
    unsigned objective_and_constraints{};
    unsigned objective_gradient_and_constraints_gradient_product{};
    unsigned constraints{};
    unsigned constraints_gradient_product{};
    unsigned grad_gi{};
    unsigned constraints_jacobian{};
    unsigned lagrangian_gradient{};
    unsigned lagrangian_hessian_product{};
    unsigned lagrangian_hessian{};
    unsigned augmented_lagrangian_hessian_product{};
    unsigned augmented_lagrangian_hessian{};
    unsigned augmented_lagrangian{};
    unsigned augmented_lagrangian_gradient{};
    unsigned augmented_lagrangian_and_gradient{};

    struct EvalTimer {
        std::chrono::nanoseconds projecting_difference_constraints{};
        std::chrono::nanoseconds projection_multipliers{};
        std::chrono::nanoseconds proximal_gradient_step{};
        std::chrono::nanoseconds inactive_indices_res_lna{};
        std::chrono::nanoseconds objective{};
        std::chrono::nanoseconds objective_gradient{};
        std::chrono::nanoseconds objective_and_gradient{};
        std::chrono::nanoseconds objective_and_constraints{};
        std::chrono::nanoseconds objective_gradient_and_constraints_gradient_product{};
        std::chrono::nanoseconds constraints{};
        std::chrono::nanoseconds constraints_gradient_product{};
        std::chrono::nanoseconds grad_gi{};
        std::chrono::nanoseconds constraints_jacobian{};
        std::chrono::nanoseconds lagrangian_gradient{};
        std::chrono::nanoseconds lagrangian_hessian_product{};
        std::chrono::nanoseconds lagrangian_hessian{};
        std::chrono::nanoseconds augmented_lagrangian_hessian_product{};
        std::chrono::nanoseconds augmented_lagrangian_hessian{};
        std::chrono::nanoseconds augmented_lagrangian{};
        std::chrono::nanoseconds augmented_lagrangian_gradient{};
        std::chrono::nanoseconds augmented_lagrangian_and_gradient{};
    } time;

    void reset() { *this = {}; }
};

ALPAQA_EXPORT std::ostream &operator<<(std::ostream &, const EvalCounter &);

inline EvalCounter::EvalTimer &operator+=(EvalCounter::EvalTimer &a,
                                          const EvalCounter::EvalTimer &b) {
    a.projecting_difference_constraints += b.projecting_difference_constraints;
    a.projection_multipliers += b.projection_multipliers;
    a.proximal_gradient_step += b.proximal_gradient_step;
    a.inactive_indices_res_lna += b.inactive_indices_res_lna;
    a.objective += b.objective;
    a.objective_gradient += b.objective_gradient;
    a.objective_and_gradient += b.objective_and_gradient;
    a.objective_and_constraints += b.objective_and_constraints;
    a.objective_gradient_and_constraints_gradient_product +=
        b.objective_gradient_and_constraints_gradient_product;
    a.constraints += b.constraints;
    a.constraints_gradient_product += b.constraints_gradient_product;
    a.grad_gi += b.grad_gi;
    a.constraints_jacobian += b.constraints_jacobian;
    a.lagrangian_gradient += b.lagrangian_gradient;
    a.lagrangian_hessian_product += b.lagrangian_hessian_product;
    a.lagrangian_hessian += b.lagrangian_hessian;
    a.augmented_lagrangian_hessian_product += b.augmented_lagrangian_hessian_product;
    a.augmented_lagrangian_hessian += b.augmented_lagrangian_hessian;
    a.augmented_lagrangian += b.augmented_lagrangian;
    a.augmented_lagrangian_gradient += b.augmented_lagrangian_gradient;
    a.augmented_lagrangian_and_gradient += b.augmented_lagrangian_and_gradient;
    return a;
}

inline EvalCounter &operator+=(EvalCounter &a, const EvalCounter &b) {
    a.projecting_difference_constraints += b.projecting_difference_constraints;
    a.projection_multipliers += b.projection_multipliers;
    a.proximal_gradient_step += b.proximal_gradient_step;
    a.inactive_indices_res_lna += b.inactive_indices_res_lna;
    a.objective += b.objective;
    a.objective_gradient += b.objective_gradient;
    a.objective_and_gradient += b.objective_and_gradient;
    a.objective_and_constraints += b.objective_and_constraints;
    a.objective_gradient_and_constraints_gradient_product +=
        b.objective_gradient_and_constraints_gradient_product;
    a.constraints += b.constraints;
    a.constraints_gradient_product += b.constraints_gradient_product;
    a.grad_gi += b.grad_gi;
    a.constraints_jacobian += b.constraints_jacobian;
    a.lagrangian_gradient += b.lagrangian_gradient;
    a.lagrangian_hessian_product += b.lagrangian_hessian_product;
    a.lagrangian_hessian += b.lagrangian_hessian;
    a.augmented_lagrangian_hessian_product += b.augmented_lagrangian_hessian_product;
    a.augmented_lagrangian_hessian += b.augmented_lagrangian_hessian;
    a.augmented_lagrangian += b.augmented_lagrangian;
    a.augmented_lagrangian_gradient += b.augmented_lagrangian_gradient;
    a.augmented_lagrangian_and_gradient += b.augmented_lagrangian_and_gradient;
    a.time += b.time;
    return a;
}

inline EvalCounter operator+(EvalCounter a, const EvalCounter &b) { return a += b; }

} // namespace alpaqa
