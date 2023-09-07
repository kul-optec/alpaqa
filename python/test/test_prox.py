import alpaqa as pa
import numpy as np
import numpy.linalg as la


def test_nuclear_norm():
    θ, φ = 0.3456, 0.8765
    U = np.array([[np.cos(θ), -np.sin(θ)], [np.sin(θ), np.cos(θ)]])
    V = np.array([[np.cos(φ), -np.sin(φ)], [np.sin(φ), np.cos(φ)]])
    Σ = np.diag([1, 0.05])
    x = U @ Σ @ V.T
    Σ_expected = np.diag([0.875, 0])
    y = np.empty((2, 2), order="F")
    h = pa.functions.NuclearNorm(0.25)
    hy = pa.prox(h, x, y, 0.5)
    assert abs(hy - 0.25 * 0.875) < 1e-12
    assert np.allclose(y, U @ Σ_expected @ V.T, rtol=1e-12, atol=1e-12)
    hy, y = pa.prox(h, x, 0.5)
    assert abs(hy - 0.25 * 0.875) < 1e-12
    assert np.allclose(y, U @ Σ_expected @ V.T, rtol=1e-12, atol=1e-12)


def test_nuclear_norm_reshape():
    θ, φ = 0.3456, 0.8765
    U = np.array([[np.cos(θ), -np.sin(θ)], [np.sin(θ), np.cos(θ)]])
    V = np.array([[np.cos(φ), -np.sin(φ)], [np.sin(φ), np.cos(φ)]])
    Σ = np.diag([1, 0.05])
    x = U @ Σ @ V.T
    Σ_expected = np.diag([0.875, 0])
    y = np.empty(2 * 2, order="F")
    h = pa.functions.NuclearNorm(0.25, 2, 2)
    hy = pa.prox(h, x.ravel("F"), y, 0.5)
    y = np.reshape(y, (2, 2), "F")
    assert abs(hy - 0.25 * 0.875) < 1e-12
    assert np.allclose(y, U @ Σ_expected @ V.T, rtol=1e-12, atol=1e-12)
    hy, y = pa.prox(h, x, 0.5)
    y = np.reshape(y, (2, 2), "F")
    assert abs(hy - 0.25 * 0.875) < 1e-12
    assert np.allclose(y, U @ Σ_expected @ V.T, rtol=1e-12, atol=1e-12)


def test_l1_norm():
    x = [-1, -0.125, -0.1, 0, 0.05, 0.12, 0.13, 1, 100]
    y_expected = np.array([-0.875, 0, 0, 0, 0, 0, 0.005, 0.875, 99.875])
    y = np.empty(len(x))
    λ = 0.25
    h = pa.functions.L1Norm(λ)
    hy = pa.prox(h, x, y, 0.5)
    assert np.allclose(y, y_expected, rtol=1e-12, atol=1e-12)
    assert abs(0.25 * la.norm(y_expected, 1) - hy) < 1e-12
    hy, y = pa.prox(h, x, 0.5)
    assert np.allclose(y.ravel(), y_expected, rtol=1e-12, atol=1e-12)
    assert abs(0.25 * la.norm(y_expected, 1) - hy) < 1e-12


def test_l1_norm_vec():
    x = [-1, -0.125, -0.1, 0, 0.05, 0.12, 0.13, 1, 100]
    y_expected = np.array([-0.875, 0, 0, 0, 0, 0, 0.005, 0.875, 99.875])
    y = np.empty(len(x))
    λ = 0.25 * np.ones(len(x))
    h = pa.functions.L1NormElementwise(λ)
    hy = pa.prox(h, x, y, 0.5)
    assert np.allclose(y, y_expected, rtol=1e-12, atol=1e-12)
    assert abs(0.25 * la.norm(y_expected, 1) - hy) < 1e-12
    hy, y = pa.prox(h, x, 0.5)
    assert np.allclose(y.ravel(), y_expected, rtol=1e-12, atol=1e-12)
    assert abs(0.25 * la.norm(y_expected, 1) - hy) < 1e-12


def test_l1_norm_step():
    rng = np.random.default_rng(4321)
    x_fw = np.array([-1, -0.125, -0.1, 0, 0.05, 0.12, 0.13, 1, 100])
    γ_fwd = 0.1234
    x = rng.uniform(-1, 1, len(x_fw))
    g = (x - x_fw) / γ_fwd
    assert np.allclose(x_fw, x - γ_fwd * g)
    y_expected = np.array([-0.875, 0, 0, 0, 0, 0, 0.005, 0.875, 99.875])
    y = np.empty(len(x))
    p = np.empty(len(x))
    h = pa.functions.L1Norm(0.25)
    hy = pa.prox_step(h, x, g, y, p, 0.5, -γ_fwd)
    assert np.allclose(y, y_expected, rtol=1e-12, atol=1e-12)
    assert abs(0.25 * la.norm(y_expected, 1) - hy) < 1e-12
    hy, y, p = pa.prox_step(h, x, g, 0.5, -γ_fwd)
    assert np.allclose(y.ravel(), y_expected, rtol=1e-12, atol=1e-12)
    assert abs(0.25 * la.norm(y_expected, 1) - hy) < 1e-12


def test_box():
    rng = np.random.default_rng(1234)
    n = 128
    x = rng.uniform(-1, 1, n)
    lb = rng.uniform(-1.5, 1, n)
    ub = lb + rng.uniform(0, 1, n)
    y_expected = np.fmax(lb, np.fmin(x, ub))
    y = np.empty(len(x))
    h = pa.Box(lower=lb, upper=ub)
    hy = pa.prox(h, x, y, 100)
    assert np.allclose(y, y_expected, rtol=1e-12, atol=1e-12)
    assert hy == 0
    hy, y = pa.prox(h, x, 100)
    assert np.allclose(y.ravel(), y_expected, rtol=1e-12, atol=1e-12)
    assert hy == 0


def test_box_matrix():
    rng = np.random.default_rng(1234)
    n = 128
    x = rng.uniform(-1, 1, (n, n))
    lb = rng.uniform(-1.5, 1, n * n)
    ub = lb + rng.uniform(0, 1, n * n)
    y = np.empty(x.shape, order="F")
    x_vec = x.ravel(order="F")
    y_expected = np.fmax(lb, np.fmin(x_vec, ub)).reshape((n, n), order="F")
    h = pa.Box(lower=lb, upper=ub)
    hy = pa.prox(h, x, y, 100)
    assert np.allclose(y, y_expected, rtol=1e-12, atol=1e-12)
    assert hy == 0
    hy, y = pa.prox(h, x, 100)
    assert np.allclose(y, y_expected, rtol=1e-12, atol=1e-12)
    assert hy == 0


def test_box_step():
    rng = np.random.default_rng(1234)
    n = 128
    x = rng.uniform(-1, 1, n)
    g = rng.uniform(-1, 1, n)
    lb = rng.uniform(-1.5, 1, n)
    ub = lb + rng.uniform(0, 1, n)
    y_expected = np.fmax(lb, np.fmin(x - 0.1 * g, ub))
    y = np.empty(len(x))
    p = np.empty(len(x))
    h = pa.Box(lower=lb, upper=ub)
    hy = pa.prox_step(h, x, g, y, p, 100, -0.1)
    p_expected = y_expected - x
    assert np.allclose(y, y_expected, rtol=1e-12, atol=1e-12)
    assert np.allclose(p, p_expected, rtol=1e-12, atol=1e-12)
    assert hy == 0
    hy, y, p = pa.prox_step(h, x, g, 100, -0.1)
    assert np.allclose(y.ravel(), y_expected, rtol=1e-12, atol=1e-12)
    assert np.allclose(p.ravel(), p_expected, rtol=1e-12, atol=1e-12)
    assert hy == 0


if __name__ == "__main__":
    test_nuclear_norm()
    test_nuclear_norm_reshape()
    test_l1_norm()
    test_l1_norm_vec()
    test_l1_norm_step()
    test_box()
    test_box_matrix()
    test_box_step()
