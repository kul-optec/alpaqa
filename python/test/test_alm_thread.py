from copy import deepcopy
import alpaqa as pa
import numpy as np
import concurrent.futures
import pytest
import os


@pytest.mark.skipif(not pa.with_casadi, reason="requires CasADi")
def test_alm_threaded():
    valgrind = 'valgrind' in os.getenv('LD_PRELOAD', '')

    pp = pa.PANOCParams(max_no_progress=100, max_iter=100)
    lbfgs = pa.LBFGSDirection()
    panoc = pa.PANOCSolver(pp, lbfgs)
    alm_params = pa.ALMParams(tolerance=1e-200, dual_tolerance=1e-200, max_iter=200, print_interval=0)
    solver = pa.ALMSolver(alm_params, panoc)

    import casadi as cs

    n = 2
    m = 2
    x = cs.SX.sym("x", n)

    Q = np.array([[1.5, 0.5], [0.5, 1.5]])
    f = 0.5 * x.T @ Q @ x
    g = x
    D = [-np.inf, 0.5], [+np.inf, +np.inf]

    name = "testproblem"
    p = pa.minimize(f, x).subject_to(g, D).with_name(name).compile()
    p = pa.Problem(p)

    def good_experiment():
        _, _, stats = deepcopy(solver)(deepcopy(p), asynchronous=True)
        return stats

    def bad_experiment1():
        _, _, stats = solver(deepcopy(p), asynchronous=True)
        return stats

    def bad_experiment2():
        _, _, stats = deepcopy(solver)(p, asynchronous=True)
        return stats

    def run(experiment):
        N = 4 if valgrind else 200
        with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as pool:
            futures = (pool.submit(experiment) for _ in range(N))
            for future in concurrent.futures.as_completed(futures):
                stats = future.result()
                assert stats["status"] == pa.SolverStatus.MaxIter

    run(good_experiment)
    if not valgrind:
        with pytest.raises(
            RuntimeError, match=r"^Same instance of ALMSolver<PANOCSolver<LBFGS"
        ) as e:
            run(bad_experiment1)
        print(e.value)
        with pytest.raises(
            RuntimeError, match=r"^Same instance of testproblem used in multiple threads"
        ) as e:
            run(bad_experiment2)
        print(e.value)


if __name__ == "__main__":
    test_alm_threaded()
    print("done.")
