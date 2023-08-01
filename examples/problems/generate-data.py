import numpy as np
from sklearn.datasets import load_breast_cancer

X, y = load_breast_cancer(return_X_y=True)
print("Number of instances:", X.shape[0])
print("Number of attributes:", X.shape[1])
assert y.shape[0] == X.shape[0]
name = 'breast_cancer'
with open(f"{name}.csv", "w") as f:
    np.savetxt(f, [X.shape], delimiter=" ", newline="\n", fmt="%d")
    np.savetxt(f, [y], delimiter=",", newline="\n")
    np.savetxt(f, X.T, delimiter=",", newline="\n")
