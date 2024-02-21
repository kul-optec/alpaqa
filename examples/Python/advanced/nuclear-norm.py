# %% alpaqa nuclear norm example

import alpaqa as pa
from jax.config import config
import jax.numpy as jnp
import jax.numpy.linalg as jla
from jax import grad, jit
from jax import random
from pprint import pprint

config.update("jax_enable_x64", True)

# %% Define the problem functions

# Quadratic loss plus nuclear norm regularization
#
# minimize  ½‖vec(AX - B)‖² + λ nucl(X)


# Returns the loss function with constant A and B
def loss(A, B):
    def _loss(X):
        err = (A @ X - B).ravel()
        return 0.5 * jnp.dot(err, err)

    return _loss


class MyProblem(pa.UnconstrProblem):
    def __init__(self, A, B, λ):
        self.rows, self.cols = A.shape[1], B.shape[1]
        super().__init__(self.rows * self.cols)
        f = loss(A, B)
        self.jit_loss = jit(f)
        self.jit_grad_loss = jit(grad(f))
        self.reg = pa.functions.NuclearNorm(λ, self.rows, self.cols)

    def eval_f(self, x):  # Cost function
        # Important: use consistent order when reshaping or raveling!
        X = jnp.reshape(x, (self.rows, self.cols), order="F")
        return self.jit_loss(X)

    def eval_grad_f(self, x, grad_f):  # Gradient of the cost
        X = jnp.reshape(x, (self.rows, self.cols), order="F")
        grad_f[:] = self.jit_grad_loss(X).ravel(order="F")

    def eval_prox_grad_step(self, γ, x, grad, x_hat, p):
        # use the prox_step helper function to carry out a generalized
        # forward-backward step. This assumes Fortran order (column major),
        # so we have to use order="F" for all reshape/ravel calls
        return pa.prox_step(self.reg, x, grad, x_hat, p, γ, -γ)


# %% Generate some data

m, n = 30, 30
r = m // 5

key = random.PRNGKey(0)
key, *subkeys = random.split(key, 6)
# Random rank-r data matrix X = UV, then add some noise
U_true = random.uniform(subkeys[0], (m, r), minval=-1, maxval=1)
V_true = random.uniform(subkeys[1], (r, n), minval=-1, maxval=1)
B_rand = 0.01 * random.normal(subkeys[2], (m, n))
A = random.uniform(subkeys[3], (m, m), minval=-1, maxval=1)
X_true = U_true @ V_true
B = A @ X_true + B_rand  # Add noise

print("cond(A) =", jla.cond(A))
print("inf(B) =", jla.norm(B_rand.ravel(), jnp.inf))

prob = MyProblem(A, B, λ=1 * m)

# %% Solve the problem using alpaqa's PANOC solver

opts = {
    "max_iter": 2000,
    "stop_crit": pa.FPRNorm,
    "quadratic_upperbound_tolerance_factor": 1e-14,
}
direction = pa.LBFGSDirection({"memory": 100})
# direction = pa.AndersonDirection({"memory": 5})
solver = pa.PANOCSolver({"print_interval": 10} | opts, direction)

# Add callback to the solver
residuals = []


def callback(it: pa.PANOCProgressInfo):
    residuals.append(it.ε)


solver.set_progress_callback(callback)

# Add evaluation counters to the problem
cnt = pa.problem_with_counters(prob)
# Solve the problem
sol, stats = solver(cnt.problem, {"tolerance": 1e-10})
X_sol = jnp.reshape(sol, (m, n), order="F")

# %% Print the results

final_f = prob.eval_f(sol)
print()
pprint(stats)
print()
print("Evaluations:")
print(cnt.evaluations)
print(f"Cost:          {final_f + stats['final_h']}")
print(f"Loss:          {final_f}")
print(f"Inf norm loss: {jla.norm((X_sol - X_true).ravel(), jnp.inf)}")
print(f"Regularizer:   {stats['final_h']}")
print(f"Rank:          {jla.matrix_rank(X_sol)}")
print(f"True rank:     {r}")
print(f"FP Residual:   {stats['ε']}")
print(f"Run time:      {stats['elapsed_time']}")
print(stats["status"])

# %% Plot the results

import matplotlib.pyplot as plt

plt.figure()
plt.semilogy(residuals, ".-")
plt.title("PANOC nuclear norm example: residuals")
plt.xlabel("Iteration")
plt.tight_layout()

if m * n <= 5000:
    plt.figure(figsize=(8, 5))
    plt.plot((A @ X_sol).ravel(), ".-", label="Estimated solution $ A\\tilde X $")
    plt.plot((A @ X_true).ravel(), "x:", label="True solution $ AX $")
    plt.plot(B.ravel(), "*-", label="Constant $ B $")
    plt.legend()
    plt.title("PANOC nuclear norm example: solution")
    plt.tight_layout()

plt.show()
