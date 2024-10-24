#include <guanaqo/quadmath/quadmath.hpp>

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace py::literals;

#include <inner/inner-solve.hpp>
#include <inner/type-erased-inner-solver.hpp>

template <alpaqa::Config Conf>
void register_python_inner_solver(py::module_ &) {
    USING_ALPAQA_CONFIG(Conf);

    using TEProblem   = alpaqa::TypeErasedProblem<config_t>;
    using InnerSolver = alpaqa::TypeErasedInnerSolver<config_t, TEProblem>;

    struct PythonInnerSolver {
        py::object solver;

        using Problem      = TEProblem;
        using SolveOptions = alpaqa::InnerSolveOptions<config_t>;
        py::object operator()(const Problem &problem, const SolveOptions &opt, rvec x, rvec y,
                              crvec Σ, rvec e) {
            py::gil_scoped_acquire gil;
            return solver(problem, opt, x, y, Σ, e);
        }
        void stop() {
            py::gil_scoped_acquire gil;
            solver.attr("stop")();
        }
        std::string get_name() const {
            py::gil_scoped_acquire gil;
            return py::cast<std::string>(py::str(solver));
        }
        py::object get_params() const {
            py::gil_scoped_acquire gil;
            return py::none();
        }
    };

    assert(inner_solver_class<InnerSolver>.cls);
    inner_solver_class<InnerSolver>.cls->def(py::init([](py::object solver) {
        return InnerSolver::template make<PythonInnerSolver>(std::move(solver));
    }));
}

template void register_python_inner_solver<alpaqa::EigenConfigd>(py::module_ &);
ALPAQA_IF_FLOAT(template void register_python_inner_solver<alpaqa::EigenConfigf>(py::module_ &);)
ALPAQA_IF_LONGD(template void register_python_inner_solver<alpaqa::EigenConfigl>(py::module_ &);)
ALPAQA_IF_QUADF(template void register_python_inner_solver<alpaqa::EigenConfigq>(py::module_ &);)
