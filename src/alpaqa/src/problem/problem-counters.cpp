#include <alpaqa/problem/problem-counters.hpp>

#include <iomanip>
#include <iostream>

namespace alpaqa {

namespace {
struct CountResult {
    unsigned count;
    std::chrono::nanoseconds time;
};
std::ostream &operator<<(std::ostream &os, const CountResult &t) {
    auto sec = [](auto t) { return std::chrono::duration<double>(t).count(); };
    os << std::setw(8);
    if (t.count > 0) {
        os << t.count << "  (";
        auto old  = os.flags();
        auto prec = os.precision(3);
        os << std::scientific << std::setw(9) << 1e6 * sec(t.time) << " µs, "
           << std::setw(9) << 1e6 * sec(t.time) / static_cast<double>(t.count)
           << " µs/call)\r\n";
        os.precision(prec);
        os.flags(old);
    } else {
        os << '-' << "\r\n";
    }
    return os;
}
} // namespace

#define ALPAQA_STRINGIFY(a) ALPAQA_STRINGIFY_IMPL(a)
#define ALPAQA_STRINGIFY_IMPL(a) #a
#define ALPAQA_PRINT_EVAL(x)                                                   \
    do {                                                                       \
        os << std::setw(53) << ALPAQA_STRINGIFY(x) ":"                         \
           << CountResult{c.x, c.time.x};                                      \
    } while (false)

std::ostream &operator<<(std::ostream &os, const EvalCounter &c) {
    ALPAQA_PRINT_EVAL(projecting_difference_constraints);
    ALPAQA_PRINT_EVAL(projection_multipliers);
    ALPAQA_PRINT_EVAL(proximal_gradient_step);
    ALPAQA_PRINT_EVAL(inactive_indices_res_lna);
    ALPAQA_PRINT_EVAL(objective);
    ALPAQA_PRINT_EVAL(objective_gradient);
    ALPAQA_PRINT_EVAL(objective_and_gradient);
    ALPAQA_PRINT_EVAL(objective_and_constraints);
    ALPAQA_PRINT_EVAL(objective_gradient_and_constraints_gradient_product);
    ALPAQA_PRINT_EVAL(constraints);
    ALPAQA_PRINT_EVAL(constraints_gradient_product);
    ALPAQA_PRINT_EVAL(grad_gi);
    ALPAQA_PRINT_EVAL(constraints_jacobian);
    ALPAQA_PRINT_EVAL(lagrangian_gradient);
    ALPAQA_PRINT_EVAL(lagrangian_hessian_product);
    ALPAQA_PRINT_EVAL(lagrangian_hessian);
    ALPAQA_PRINT_EVAL(augmented_lagrangian_hessian_product);
    ALPAQA_PRINT_EVAL(augmented_lagrangian_hessian);
    ALPAQA_PRINT_EVAL(augmented_lagrangian);
    ALPAQA_PRINT_EVAL(augmented_lagrangian_gradient);
    ALPAQA_PRINT_EVAL(augmented_lagrangian_and_gradient);
    os << "+ --------------------------------------------------:\n" //
       << std::setw(53) << "total:"                                 //
       << CountResult{
              c.projecting_difference_constraints + c.projection_multipliers +
                  c.proximal_gradient_step + c.inactive_indices_res_lna +
                  c.objective + c.objective_gradient +
                  c.objective_and_gradient + c.objective_and_constraints +
                  c.objective_gradient_and_constraints_gradient_product +
                  c.constraints + c.constraints_gradient_product + c.grad_gi +
                  c.constraints_jacobian + c.lagrangian_gradient +
                  c.lagrangian_hessian_product + c.lagrangian_hessian +
                  c.augmented_lagrangian_hessian_product +
                  c.augmented_lagrangian_hessian + c.augmented_lagrangian +
                  c.augmented_lagrangian_gradient +
                  c.augmented_lagrangian_and_gradient,
              c.time.projecting_difference_constraints +
                  c.time.projection_multipliers +
                  c.time.proximal_gradient_step +
                  c.time.inactive_indices_res_lna + c.time.objective +
                  c.time.objective_gradient + c.time.objective_and_gradient +
                  c.time.objective_and_constraints +
                  c.time.objective_gradient_and_constraints_gradient_product +
                  c.time.constraints + c.time.constraints_gradient_product +
                  c.time.grad_gi + c.time.constraints_jacobian +
                  c.time.lagrangian_gradient +
                  c.time.lagrangian_hessian_product +
                  c.time.lagrangian_hessian +
                  c.time.augmented_lagrangian_hessian_product +
                  c.time.augmented_lagrangian_hessian +
                  c.time.augmented_lagrangian +
                  c.time.augmented_lagrangian_gradient +
                  c.time.augmented_lagrangian_and_gradient,
          };
    return os;
}

} // namespace alpaqa