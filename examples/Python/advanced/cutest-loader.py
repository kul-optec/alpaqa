import os
import numpy as np
import alpaqa as pa
import contextlib
import gc
from pathlib import Path

# Path containing the compiled CUTEst problems
cutest_dir = Path(os.getenv("HOME")) / "opt" / "CUTEst" / "QP"
problem_name = "CBS"

# alpaqa currently only supports one instance of a CUTEst problem at a time 🙃
with contextlib.suppress(NameError):
    del prob
gc.collect()
# Load problem
prob = pa.CUTEstProblem(str(cutest_dir / problem_name), sparse=True)

# Extract the problem data
n = prob.num_variables
m = prob.num_constraints
Q, Q_sym = prob.eval_lagrangian_hessian(np.zeros(n), np.zeros(m))
c, q = prob.eval_objective_and_gradient(np.zeros(n))
x_lb = prob.variable_bounds.lower
x_ub = prob.variable_bounds.upper
A, A_sym = prob.eval_constraints_jacobian(np.zeros(n))
g = prob.eval_constraints(np.zeros(n))
g_lb = prob.general_bounds.lower - g
g_ub = prob.general_bounds.upper - g

# You could now pass these matrices to a QP solver, for example.

# Do note the symmetry of the sparse matrices:
# - Q will usually only contain the elements in the upper triangular
#   (as indicated by Q_sym).
# - A will usually be unsymmetric, and store all its elements explicitly
#   (as indicated by A_sym).
print(n, m)
print(A_sym)
print(A.nnz)
print(Q_sym)
print(Q.nnz)
