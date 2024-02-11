#include <alpaqa/example-util.hpp>
#include <alpaqa/functions/l1-norm.hpp>
#include <alpaqa/util/print.hpp>
USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <tuple>
using std::cout, std::ranges::generate;

const length_t n = 5; // problem dimension

// minimize  ½‖Ax - b‖² + λ ‖x‖₁
auto build_problem() {
    auto rng = std::mt19937(12345);
    auto uni = std::uniform_real_distribution<real_t>(-1, 1);
    mat A(n, n);
    vec x(n), b(n);
    generate(A.reshaped(), [&] { return uni(rng); });
    generate(x.reshaped(), [&] { return uni(rng); });
    generate(b.reshaped(), [&] { return 1e-2 * uni(rng); });
    x(1) = x(3) = 0;
    b += A * x;
    real_t λ = 1e-2;
    return std::make_tuple(std::move(A), std::move(x), std::move(b), λ);
}

int main() {
    alpaqa::init_stdout();

    // Least squares problem
    auto [A, x_exact, b, λ] = build_problem();
    // ℓ₁ regularizer
    auto h = alpaqa::functions::L1Norm<config_t>(λ);
    // Step size
    real_t γ = 0.38; // should be < ‖AᵀA‖₂⁻¹

    // Iterates and other workspace variables
    vec x = vec::Zero(n), x_next(n), grad(n), err(n), step(n);

    // Forward-backward splitting loop
    cout << "iteration\t      least squares loss\t    fixed-point residual\n";
    for (index_t i = 0; i < 1'000; ++i) {
        // Compute loss function value and gradient
        err.noalias()  = A * x - b;
        grad.noalias() = A.transpose() * err;
        real_t f       = 0.5 * err.squaredNorm();

        // Forward-backward step
        alpaqa::prox_step(h, x, grad, x_next, step, γ, -γ);

        // Advance
        x.swap(x_next);

        // Check stopping criterion
        real_t residual = step.norm() / γ;
        cout << std::setw(9) << i << '\t' << alpaqa::float_to_str(f) << '\t'
             << alpaqa::float_to_str(residual) << '\r';
        if (residual < 1e-10)
            break;
    }

    // Report solution
    cout << '\n';
    alpaqa::print_python(cout << "x_exact = ", x_exact);
    alpaqa::print_python(cout << "x_lasso = ", x);
}
