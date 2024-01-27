#include <alpaqa/util/quadmath/quadmath.hpp>

#include <pybind11/chrono.h>
#include <pybind11/eigen.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace py::literals;

#include <alpaqa/lbfgsb/lbfgsb-adapter.hpp>
#include <alpaqa/util/check-dim.hpp>

#include <dict/stats-to-dict.hpp>
#include <inner/inner-solve.hpp>
#include <inner/type-erased-inner-solver.hpp>
#include <params/params.hpp>
#include <util/async.hpp>
#include <util/copy.hpp>
#include <util/member.hpp>

template <alpaqa::Config Conf>
void register_lbfgsb(py::module_ &m) {
    if constexpr (std::is_same_v<Conf, alpaqa::EigenConfigd>) {
        USING_ALPAQA_CONFIG(Conf);

        using TEProblem   = alpaqa::TypeErasedProblem<config_t>;
        using InnerSolver = alpaqa::TypeErasedInnerSolver<config_t, TEProblem>;

        // ----------------------------------------------------------------------------------------- //
        using LBFGSBParams = alpaqa::lbfgsb::LBFGSBParams;
        auto params        = register_dataclass<LBFGSBParams>(
            m, "LBFGSBParams", "C++ documentation: :cpp:class:`alpaqa::LBFGSBParams`");

        // ----------------------------------------------------------------------------------------- //
        using LBFGSBProgressInfo = alpaqa::lbfgsb::LBFGSBProgressInfo;
        py::class_<LBFGSBProgressInfo>(m, "LBFGSBProgressInfo",
                                       "Data passed to the LBFGSB progress callback.\n\n"
                                       "C++ documentation: :cpp:class:`alpaqa::LBFGSBProgressInfo`")
            // clang-format off
        .def_readonly("k", &LBFGSBProgressInfo::k, "Iteration")
        .def_readonly("status", &LBFGSBProgressInfo::status, "Current solver status")
        .def_readonly("x", &LBFGSBProgressInfo::x, "Decision variable :math:`x`")
        .def_readonly("ψ", &LBFGSBProgressInfo::ψ, "Objective value :math:`\\psi(x)`")
        .def_readonly("grad_ψ", &LBFGSBProgressInfo::grad_ψ, "Gradient of objective :math:`\\nabla\\psi(x)`")
        .def_readonly("τ_max", &LBFGSBProgressInfo::τ_max, "Maximum line search parameter :math:`\\tau_\\mathrm{max}`")
        .def_readonly("τ", &LBFGSBProgressInfo::τ, "Line search parameter :math:`\\tau` (or :math:`\\tau_\\mathrm{rel}`)")
        .def_readonly("ε", &LBFGSBProgressInfo::ε, "Tolerance reached :math:`\\varepsilon_k`")
        .def_readonly("Σ", &LBFGSBProgressInfo::Σ, "Penalty factor :math:`\\Sigma`")
        .def_readonly("y", &LBFGSBProgressInfo::y, "Lagrange multipliers :math:`y`")
        .def_property_readonly("problem", member_ptr<&LBFGSBProgressInfo::problem>(), "Problem being solved")
        .def_property_readonly("params", member_ptr<&LBFGSBProgressInfo::params>(), "Solver parameters");
        // clang-format on

        // Solve without ALM
        using LBFGSBSolver = alpaqa::lbfgsb::LBFGSBSolver;
        using Problem      = typename LBFGSBSolver::Problem;

        py::class_<LBFGSBSolver> lbfgsb_solver(
            m, "LBFGSBSolver", "C++ documentation: :cpp:class:`alpaqa::LBFGSBSolver`");
        default_copy_methods(lbfgsb_solver);
        lbfgsb_solver
            // Constructors
            .def(py::init([](params_or_dict<LBFGSBParams> params) {
                     return LBFGSBSolver{var_kwargs_to_struct(params)};
                 }),
                 "lbfgsb_params"_a = py::dict{},
                 "Create a LBFGSB solver using structured L-BFGS directions.")
            .def_property_readonly_static(
                "Params", [p{py::object{params}}](const py::object &) { return p; });
        register_inner_solver_methods<LBFGSBSolver, Problem, InnerSolver>(lbfgsb_solver);
    }
}

// clang-format off
template void register_lbfgsb<alpaqa::EigenConfigd>(py::module_ &);
ALPAQA_IF_FLOAT(template void register_lbfgsb<alpaqa::EigenConfigf>(py::module_ &);)
ALPAQA_IF_LONGD(template void register_lbfgsb<alpaqa::EigenConfigl>(py::module_ &);)
ALPAQA_IF_QUADF(template void register_lbfgsb<alpaqa::EigenConfigq>(py::module_ &);)
// clang-format on
