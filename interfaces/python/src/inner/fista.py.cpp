#include <alpaqa/util/quadmath/quadmath.hpp>

#include <pybind11/chrono.h>
#include <pybind11/eigen.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace py::literals;

#include <alpaqa/inner/fista.hpp>
#include <alpaqa/util/check-dim.hpp>

#include <dict/stats-to-dict.hpp>
#include <inner/inner-solve.hpp>
#include <inner/type-erased-inner-solver.hpp>
#include <params/params.hpp>
#include <util/async.hpp>
#include <util/copy.hpp>
#include <util/member.hpp>

template <alpaqa::Config Conf>
void register_fista(py::module_ &m) {
    USING_ALPAQA_CONFIG(Conf);

    using TEProblem   = alpaqa::TypeErasedProblem<config_t>;
    using InnerSolver = alpaqa::TypeErasedInnerSolver<config_t, TEProblem>;

    // ----------------------------------------------------------------------------------------- //
    using FISTAParams = alpaqa::FISTAParams<config_t>;
    auto params       = register_dataclass<FISTAParams>(
        m, "FISTAParams", "C++ documentation: :cpp:class:`alpaqa::FISTAParams`");

    // ----------------------------------------------------------------------------------------- //
    using FISTAProgressInfo = alpaqa::FISTAProgressInfo<config_t>;
    py::class_<FISTAProgressInfo>(m, "FISTAProgressInfo",
                                  "Data passed to the FISTA progress callback.\n\n"
                                  "C++ documentation: :cpp:class:`alpaqa::FISTAProgressInfo`")
        // clang-format off
        .def_readonly("k", &FISTAProgressInfo::k, "Iteration")
        .def_readonly("status", &FISTAProgressInfo::status, "Current solver status")
        .def_readonly("x", &FISTAProgressInfo::x, "Decision variable :math:`x`")
        .def_readonly("p", &FISTAProgressInfo::p, "Projected gradient step :math:`p`")
        .def_readonly("norm_sq_p", &FISTAProgressInfo::norm_sq_p, ":math:`\\left\\|p\\right\\|^2`")
        .def_readonly("x_hat", &FISTAProgressInfo::x̂, "Decision variable after projected gradient step :math:`\\hat x`")
        .def_readonly("y_hat", &FISTAProgressInfo::ŷ, "Candidate updated multipliers at x̂ :math:`\\hat y(\\hat x)`")
        .def_readonly("φγ", &FISTAProgressInfo::φγ, "Forward-backward envelope :math:`\\varphi_\\gamma(x)`")
        .def_readonly("ψ", &FISTAProgressInfo::ψ, "Objective value :math:`\\psi(x)`")
        .def_readonly("grad_ψ", &FISTAProgressInfo::grad_ψ, "Gradient of objective :math:`\\nabla\\psi(x)`")
        .def_readonly("ψ_hat", &FISTAProgressInfo::ψ_hat, "Objective at x̂ :math:`\\psi(\\hat x)`")
        .def_readonly("grad_ψ_hat", &FISTAProgressInfo::grad_ψ_hat, "Gradient of objective at x̂ :math:`\\nabla\\psi(\\hat x)`")
        .def_readonly("L", &FISTAProgressInfo::L, "Estimate of Lipschitz constant of objective :math:`L`")
        .def_readonly("γ", &FISTAProgressInfo::γ, "Step size :math:`\\gamma`")
        .def_readonly("t", &FISTAProgressInfo::t, "Acceleration parameter :math:`t`")
        .def_readonly("ε", &FISTAProgressInfo::ε, "Tolerance reached :math:`\\varepsilon_k`")
        .def_readonly("Σ", &FISTAProgressInfo::Σ, "Penalty factor :math:`\\Sigma`")
        .def_readonly("y", &FISTAProgressInfo::y, "Lagrange multipliers :math:`y`")
        .def_property_readonly("problem", member_ptr<&FISTAProgressInfo::problem>(), "Problem being solved")
        .def_property_readonly("params", member_ptr<&FISTAProgressInfo::params>(), "Solver parameters")
        // clang-format on
        .def_property_readonly(
            "fpr", [](const FISTAProgressInfo &p) { return std::sqrt(p.norm_sq_p) / p.γ; },
            "Fixed-point residual :math:`\\left\\|p\\right\\| / \\gamma`");

    // Solve without ALM
    using FISTASolver = alpaqa::FISTASolver<config_t>;
    using Problem     = typename FISTASolver::Problem;

    py::class_<FISTASolver> fista_solver(m, "FISTASolver",
                                         "C++ documentation: :cpp:class:`alpaqa::FISTASolver`");
    default_copy_methods(fista_solver);
    fista_solver
        // Constructors
        .def(py::init([](params_or_dict<FISTAParams> params) {
                 return FISTASolver{var_kwargs_to_struct(params)};
             }),
             "fista_params"_a = py::dict{},
             "Create a FISTA solver using structured L-BFGS directions.")
        .def_property_readonly_static("Params",
                                      [p{py::object{params}}](const py::object &) { return p; });
    register_inner_solver_methods<FISTASolver, Problem, InnerSolver>(fista_solver);
}

// clang-format off
template void register_fista<alpaqa::EigenConfigd>(py::module_ &);
ALPAQA_IF_FLOAT(template void register_fista<alpaqa::EigenConfigf>(py::module_ &);)
ALPAQA_IF_LONGD(template void register_fista<alpaqa::EigenConfigl>(py::module_ &);)
ALPAQA_IF_QUADF(template void register_fista<alpaqa::EigenConfigq>(py::module_ &);)
// clang-format on
