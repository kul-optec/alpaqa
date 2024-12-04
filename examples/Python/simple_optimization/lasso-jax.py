# %% alpaqa lasso example

import numpy as np
import alpaqa as pa
import jax
import jax.numpy as jnp
from jax import grad, jit
from pprint import pprint

jax.config.update("jax_enable_x64", True)

scale = 5000
n, m = scale, scale * 2
sparsity = 0.02

# %% Generate some data

rng = np.random.default_rng(seed=123)
# Random data matrix A
A = rng.uniform(-1, 1, (m, n))
# Sparse solution x_exact
x_exact = rng.uniform(-0.1, 1, n)
x_exact[rng.uniform(0, 1, n) > sparsity] = 0
# Noisy right-hand side b
b = A @ x_exact + rng.normal(0, 0.1, m)

# %% Build the problem

# Quadratic loss plus l1-regularization
# minimize  ½‖Ax - b‖² + λ‖x‖₁

λ = 0.0025 * m


def loss(x):
    err = A @ x - b
    return 0.5 * jnp.dot(err, err)


class LassoProblem(pa.BoxConstrProblem):
    def __init__(self):
        super().__init__(n, 0)
        self.variable_bounds.lower[:] = 0  # Positive lasso
        self.l1_reg = [λ]  # Regularization
        self.jit_loss = jit(loss)
        self.jit_grad_loss = jit(grad(loss))

    def eval_objective(self, x):  # Cost function
        return self.jit_loss(x)

    def eval_objective_gradient(self, x, grad_f):  # Gradient of the cost
        grad_f[:] = self.jit_grad_loss(x)


prob = LassoProblem()

# %% Solve the problem using alpaqa's PANOC solver

opts = {
    "max_iter": 100,
    "stop_crit": pa.FPRNorm,
    # Use a laxer tolerance because large problems have more numerical errors:
    "quadratic_upperbound_tolerance_factor": 1e-12,
}
direction = pa.StructuredLBFGSDirection({"memory": 5}, {"hessian_vec_factor": 0})
# direction = pa.LBFGSDirection({"memory": 5})
solver = pa.PANOCSolver({"print_interval": 5} | opts, direction)
# Add evaluation counters to the problem
cnt = pa.problem_with_counters(prob)
# Solve the problem
sol, stats = solver(cnt.problem, {"tolerance": 1e-10})

# %% Print the results

final_f = prob.eval_objective(sol)
print()
pprint(stats)
print()
print("Evaluations:")
print(cnt.evaluations)
print(f"Cost:          {final_f + stats['final_h']}")
print(f"Loss:          {final_f}")
print(f"Regularizer:   {stats['final_h']}")
print(f"FP Residual:   {stats['ε']}")
print(f"Run time:      {stats['elapsed_time']}")
print(stats["status"])

# %% Plot the results

import matplotlib.pyplot as plt

plt.figure(figsize=(8, 5))
plt.plot(x_exact, ".-", label="True solution")
plt.plot(sol, ".-", label="Estimated solution")
plt.legend()
plt.title("PANOC lasso example: solution")
plt.tight_layout()
plt.figure(figsize=(8, 5))
plt.plot(A @ x_exact, ".-", label="True solution")
plt.plot(A @ sol, ".-", label="Estimated solution")
plt.legend()
plt.title("PANOC lasso example: right-hand side")
plt.tight_layout()
plt.show()
