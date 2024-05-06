#include <alpaqa/problem/ocproblem-counters.hpp>
#include <alpaqa/problem/problem-counters.hpp>
#include <pybind11/chrono.h>
#include <pybind11/pybind11.h>
#include <memory>
#include <sstream>

namespace py = pybind11;

void register_counters(py::module_ &m) {
    // ----------------------------------------------------------------------------------------- //
    py::class_<alpaqa::EvalCounter, std::shared_ptr<alpaqa::EvalCounter>> evalcounter(
        m, "EvalCounter",
        "C++ documentation: "
        ":cpp:class:`alpaqa::EvalCounter`\n\n");
    py::class_<alpaqa::EvalCounter::EvalTimer>(evalcounter, "EvalTimer",
                                               "C++ documentation: "
                                               ":cpp:class:`alpaqa::EvalCounter::EvalTimer`\n\n")
        .def(py::pickle(
            [](const alpaqa::EvalCounter::EvalTimer &p) { // __getstate__
                return py::make_tuple(
                    // clang-format off
                    p.projecting_difference_constraints,
                    p.projection_multipliers,
                    p.proximal_gradient_step,
                    p.inactive_indices_res_lna,
                    p.objective,
                    p.objective_gradient,
                    p.objective_and_gradient,
                    p.objective_and_constraints,
                    p.objective_gradient_and_constraints_gradient_product,
                    p.constraints,
                    p.constraints_gradient_product,
                    p.grad_gi,
                    p.constraints_jacobian,
                    p.lagrangian_gradient,
                    p.lagrangian_hessian_product,
                    p.lagrangian_hessian,
                    p.augmented_lagrangian_hessian_product,
                    p.augmented_lagrangian_hessian,
                    p.augmented_lagrangian,
                    p.augmented_lagrangian_gradient,
                    p.augmented_lagrangian_and_gradient)
                    // clang-format on
                    ;
            },
            [](py::tuple t) { // __setstate__
                if (t.size() != 21)
                    throw std::runtime_error("Invalid state!");
                using T = alpaqa::EvalCounter::EvalTimer;
                return T{
                    // clang-format off
                    .projecting_difference_constraints = py::cast<decltype(T::projecting_difference_constraints)>(t[0]),
                    .projection_multipliers = py::cast<decltype(T::projection_multipliers)>(t[1]),
                    .proximal_gradient_step = py::cast<decltype(T::proximal_gradient_step)>(t[2]),
                    .inactive_indices_res_lna = py::cast<decltype(T::inactive_indices_res_lna)>(t[3]),
                    .objective = py::cast<decltype(T::objective)>(t[4]),
                    .objective_gradient = py::cast<decltype(T::objective_gradient)>(t[5]),
                    .objective_and_gradient = py::cast<decltype(T::objective_and_gradient)>(t[6]),
                    .objective_and_constraints = py::cast<decltype(T::objective_and_constraints)>(t[7]),
                    .objective_gradient_and_constraints_gradient_product = py::cast<decltype(T::objective_gradient_and_constraints_gradient_product)>(t[8]),
                    .constraints = py::cast<decltype(T::constraints)>(t[9]),
                    .constraints_gradient_product = py::cast<decltype(T::constraints_gradient_product)>(t[10]),
                    .grad_gi = py::cast<decltype(T::grad_gi)>(t[11]),
                    .constraints_jacobian = py::cast<decltype(T::constraints_jacobian)>(t[12]),
                    .lagrangian_gradient = py::cast<decltype(T::lagrangian_gradient)>(t[13]),
                    .lagrangian_hessian_product = py::cast<decltype(T::lagrangian_hessian_product)>(t[14]),
                    .lagrangian_hessian = py::cast<decltype(T::lagrangian_hessian)>(t[15]),
                    .augmented_lagrangian_hessian_product = py::cast<decltype(T::augmented_lagrangian_hessian_product)>(t[16]),
                    .augmented_lagrangian_hessian = py::cast<decltype(T::augmented_lagrangian_hessian)>(t[17]),
                    .augmented_lagrangian = py::cast<decltype(T::augmented_lagrangian)>(t[18]),
                    .augmented_lagrangian_gradient = py::cast<decltype(T::augmented_lagrangian_gradient)>(t[19]),
                    .augmented_lagrangian_and_gradient = py::cast<decltype(T::augmented_lagrangian_and_gradient)>(t[20]),
                    // clang-format on
                };
            }))
        // clang-format off
        .def_readwrite("projecting_difference_constraints", &alpaqa::EvalCounter::EvalTimer::projecting_difference_constraints)
        .def_readwrite("projection_multipliers", &alpaqa::EvalCounter::EvalTimer::projection_multipliers)
        .def_readwrite("proximal_gradient_step", &alpaqa::EvalCounter::EvalTimer::proximal_gradient_step)
        .def_readwrite("inactive_indices_res_lna", &alpaqa::EvalCounter::EvalTimer::inactive_indices_res_lna)
        .def_readwrite("objective", &alpaqa::EvalCounter::EvalTimer::objective)
        .def_readwrite("objective_gradient", &alpaqa::EvalCounter::EvalTimer::objective_gradient)
        .def_readwrite("objective_and_gradient", &alpaqa::EvalCounter::EvalTimer::objective_and_gradient)
        .def_readwrite("objective_and_constraints", &alpaqa::EvalCounter::EvalTimer::objective_and_constraints)
        .def_readwrite("objective_gradient_and_constraints_gradient_product", &alpaqa::EvalCounter::EvalTimer::objective_gradient_and_constraints_gradient_product)
        .def_readwrite("constraints", &alpaqa::EvalCounter::EvalTimer::constraints)
        .def_readwrite("constraints_gradient_product", &alpaqa::EvalCounter::EvalTimer::constraints_gradient_product)
        .def_readwrite("grad_gi", &alpaqa::EvalCounter::EvalTimer::grad_gi)
        .def_readwrite("constraints_jacobian", &alpaqa::EvalCounter::EvalTimer::constraints_jacobian)
        .def_readwrite("lagrangian_gradient", &alpaqa::EvalCounter::EvalTimer::lagrangian_gradient)
        .def_readwrite("lagrangian_hessian_product", &alpaqa::EvalCounter::EvalTimer::lagrangian_hessian_product)
        .def_readwrite("lagrangian_hessian", &alpaqa::EvalCounter::EvalTimer::lagrangian_hessian)
        .def_readwrite("augmented_lagrangian_hessian_product", &alpaqa::EvalCounter::EvalTimer::augmented_lagrangian_hessian_product)
        .def_readwrite("augmented_lagrangian_hessian", &alpaqa::EvalCounter::EvalTimer::augmented_lagrangian_hessian)
        .def_readwrite("augmented_lagrangian", &alpaqa::EvalCounter::EvalTimer::augmented_lagrangian)
        .def_readwrite("augmented_lagrangian_gradient", &alpaqa::EvalCounter::EvalTimer::augmented_lagrangian_gradient)
        .def_readwrite("augmented_lagrangian_and_gradient", &alpaqa::EvalCounter::EvalTimer::augmented_lagrangian_and_gradient);
    // clang-format on

    evalcounter
        .def(py::pickle(
            [](const alpaqa::EvalCounter &p) { // __getstate__
                return py::make_tuple(
                    // clang-format off
                    p.projecting_difference_constraints,
                    p.projection_multipliers,
                    p.proximal_gradient_step,
                    p.inactive_indices_res_lna,
                    p.objective,
                    p.objective_gradient,
                    p.objective_and_gradient,
                    p.objective_and_constraints,
                    p.objective_gradient_and_constraints_gradient_product,
                    p.constraints,
                    p.constraints_gradient_product,
                    p.grad_gi,
                    p.constraints_jacobian,
                    p.lagrangian_gradient,
                    p.lagrangian_hessian_product,
                    p.lagrangian_hessian,
                    p.augmented_lagrangian_hessian_product,
                    p.augmented_lagrangian_hessian,
                    p.augmented_lagrangian,
                    p.augmented_lagrangian_gradient,
                    p.augmented_lagrangian_and_gradient,
                    // clang-format on
                    p.time);
            },
            [](py::tuple t) { // __setstate__
                if (t.size() != 22)
                    throw std::runtime_error("Invalid state!");
                using T = alpaqa::EvalCounter;
                return T{
                    // clang-format off
                    .projecting_difference_constraints= py::cast<decltype(T::projecting_difference_constraints)>(t[0]),
                    .projection_multipliers= py::cast<decltype(T::projection_multipliers)>(t[1]),
                    .proximal_gradient_step= py::cast<decltype(T::proximal_gradient_step)>(t[2]),
                    .inactive_indices_res_lna= py::cast<decltype(T::inactive_indices_res_lna)>(t[3]),
                    .objective= py::cast<decltype(T::objective)>(t[4]),
                    .objective_gradient= py::cast<decltype(T::objective_gradient)>(t[5]),
                    .objective_and_gradient= py::cast<decltype(T::objective_and_gradient)>(t[6]),
                    .objective_and_constraints= py::cast<decltype(T::objective_and_constraints)>(t[7]),
                    .objective_gradient_and_constraints_gradient_product= py::cast<decltype(T::objective_gradient_and_constraints_gradient_product)>(t[8]),
                    .constraints= py::cast<decltype(T::constraints)>(t[9]),
                    .constraints_gradient_product= py::cast<decltype(T::constraints_gradient_product)>(t[10]),
                    .grad_gi= py::cast<decltype(T::grad_gi)>(t[11]),
                    .constraints_jacobian= py::cast<decltype(T::constraints_jacobian)>(t[12]),
                    .lagrangian_gradient= py::cast<decltype(T::lagrangian_gradient)>(t[13]),
                    .lagrangian_hessian_product= py::cast<decltype(T::lagrangian_hessian_product)>(t[14]),
                    .lagrangian_hessian= py::cast<decltype(T::lagrangian_hessian)>(t[15]),
                    .augmented_lagrangian_hessian_product= py::cast<decltype(T::augmented_lagrangian_hessian_product)>(t[16]),
                    .augmented_lagrangian_hessian= py::cast<decltype(T::augmented_lagrangian_hessian)>(t[17]),
                    .augmented_lagrangian= py::cast<decltype(T::augmented_lagrangian)>(t[18]),
                    .augmented_lagrangian_gradient= py::cast<decltype(T::augmented_lagrangian_gradient)>(t[19]),
                    .augmented_lagrangian_and_gradient= py::cast<decltype(T::augmented_lagrangian_and_gradient)>(t[20]),
                    // clang-format on
                    .time = py::cast<decltype(T::time)>(t[21]),
                };
            }))
        // clang-format off
        .def_readwrite("projecting_difference_constraints", &alpaqa::EvalCounter::projecting_difference_constraints)
        .def_readwrite("projection_multipliers", &alpaqa::EvalCounter::projection_multipliers)
        .def_readwrite("proximal_gradient_step", &alpaqa::EvalCounter::proximal_gradient_step)
        .def_readwrite("inactive_indices_res_lna", &alpaqa::EvalCounter::inactive_indices_res_lna)
        .def_readwrite("objective", &alpaqa::EvalCounter::objective)
        .def_readwrite("objective_gradient", &alpaqa::EvalCounter::objective_gradient)
        .def_readwrite("objective_and_gradient", &alpaqa::EvalCounter::objective_and_gradient)
        .def_readwrite("objective_and_constraints", &alpaqa::EvalCounter::objective_and_constraints)
        .def_readwrite("objective_gradient_and_constraints_gradient_product", &alpaqa::EvalCounter::objective_gradient_and_constraints_gradient_product)
        .def_readwrite("constraints", &alpaqa::EvalCounter::constraints)
        .def_readwrite("constraints_gradient_product", &alpaqa::EvalCounter::constraints_gradient_product)
        .def_readwrite("grad_gi", &alpaqa::EvalCounter::grad_gi)
        .def_readwrite("constraints_jacobian", &alpaqa::EvalCounter::constraints_jacobian)
        .def_readwrite("lagrangian_gradient", &alpaqa::EvalCounter::lagrangian_gradient)
        .def_readwrite("lagrangian_hessian_product", &alpaqa::EvalCounter::lagrangian_hessian_product)
        .def_readwrite("lagrangian_hessian", &alpaqa::EvalCounter::lagrangian_hessian)
        .def_readwrite("augmented_lagrangian_hessian_product", &alpaqa::EvalCounter::augmented_lagrangian_hessian_product)
        .def_readwrite("augmented_lagrangian_hessian", &alpaqa::EvalCounter::augmented_lagrangian_hessian)
        .def_readwrite("augmented_lagrangian", &alpaqa::EvalCounter::augmented_lagrangian)
        .def_readwrite("augmented_lagrangian_gradient", &alpaqa::EvalCounter::augmented_lagrangian_gradient)
        .def_readwrite("augmented_lagrangian_and_gradient", &alpaqa::EvalCounter::augmented_lagrangian_and_gradient)
        // clang-format on
        .def_readwrite("time", &alpaqa::EvalCounter::time)
        .def("__str__", [](const alpaqa::EvalCounter &c) {
            std::ostringstream os;
            os << c;
            return os.str();
        });

    py::class_<alpaqa::OCPEvalCounter, std::shared_ptr<alpaqa::OCPEvalCounter>> ocpevalcounter(
        m, "OCPEvalCounter",
        "C++ documentation: "
        ":cpp:class:`alpaqa::OCPEvalCounter`\n\n");
    py::class_<alpaqa::OCPEvalCounter::OCPEvalTimer>(
        ocpevalcounter, "OCPEvalTimer",
        "C++ documentation: "
        ":cpp:class:`alpaqa::OCPEvalCounter::OCPEvalTimer`\n\n")
        .def(py::pickle(
            [](const alpaqa::OCPEvalCounter::OCPEvalTimer &p) { // __getstate__
                return py::make_tuple(p.f, p.jac_f, p.grad_f_prod, p.h, p.h_N, p.l, p.l_N, p.qr,
                                      p.q_N, p.add_Q, p.add_Q_N, p.add_R_masked, p.add_S_masked,
                                      p.add_R_prod_masked, p.add_S_prod_masked, p.constr,
                                      p.constr_N, p.grad_constr_prod, p.grad_constr_prod_N,
                                      p.add_gn_hess_constr, p.add_gn_hess_constr_N);
            },
            [](py::tuple t) { // __setstate__
                if (t.size() != 21)
                    throw std::runtime_error("Invalid state!");
                using T = alpaqa::OCPEvalCounter::OCPEvalTimer;
                return T{
                    py::cast<decltype(T::f)>(t[0]),
                    py::cast<decltype(T::jac_f)>(t[1]),
                    py::cast<decltype(T::grad_f_prod)>(t[2]),
                    py::cast<decltype(T::h)>(t[3]),
                    py::cast<decltype(T::h_N)>(t[4]),
                    py::cast<decltype(T::l)>(t[5]),
                    py::cast<decltype(T::l_N)>(t[6]),
                    py::cast<decltype(T::qr)>(t[7]),
                    py::cast<decltype(T::q_N)>(t[8]),
                    py::cast<decltype(T::add_Q)>(t[9]),
                    py::cast<decltype(T::add_Q_N)>(t[10]),
                    py::cast<decltype(T::add_R_masked)>(t[11]),
                    py::cast<decltype(T::add_S_masked)>(t[12]),
                    py::cast<decltype(T::add_R_prod_masked)>(t[13]),
                    py::cast<decltype(T::add_S_prod_masked)>(t[14]),
                    py::cast<decltype(T::constr)>(t[15]),
                    py::cast<decltype(T::constr_N)>(t[16]),
                    py::cast<decltype(T::grad_constr_prod)>(t[17]),
                    py::cast<decltype(T::grad_constr_prod_N)>(t[18]),
                    py::cast<decltype(T::add_gn_hess_constr)>(t[19]),
                    py::cast<decltype(T::add_gn_hess_constr_N)>(t[20]),
                };
            }))
        // clang-format off
        .def_readwrite("f", &alpaqa::OCPEvalCounter::OCPEvalTimer::f)
        .def_readwrite("jac_f", &alpaqa::OCPEvalCounter::OCPEvalTimer::jac_f)
        .def_readwrite("grad_f_prod", &alpaqa::OCPEvalCounter::OCPEvalTimer::grad_f_prod)
        .def_readwrite("h", &alpaqa::OCPEvalCounter::OCPEvalTimer::h)
        .def_readwrite("h_N", &alpaqa::OCPEvalCounter::OCPEvalTimer::h_N)
        .def_readwrite("l", &alpaqa::OCPEvalCounter::OCPEvalTimer::l)
        .def_readwrite("l_N", &alpaqa::OCPEvalCounter::OCPEvalTimer::l_N)
        .def_readwrite("qr", &alpaqa::OCPEvalCounter::OCPEvalTimer::qr)
        .def_readwrite("q_N", &alpaqa::OCPEvalCounter::OCPEvalTimer::q_N)
        .def_readwrite("add_Q", &alpaqa::OCPEvalCounter::OCPEvalTimer::add_Q)
        .def_readwrite("add_Q_N", &alpaqa::OCPEvalCounter::OCPEvalTimer::add_Q_N)
        .def_readwrite("add_R_masked", &alpaqa::OCPEvalCounter::OCPEvalTimer::add_R_masked)
        .def_readwrite("add_S_masked", &alpaqa::OCPEvalCounter::OCPEvalTimer::add_S_masked)
        .def_readwrite("add_R_prod_masked", &alpaqa::OCPEvalCounter::OCPEvalTimer::add_R_prod_masked)
        .def_readwrite("add_S_prod_masked", &alpaqa::OCPEvalCounter::OCPEvalTimer::add_S_prod_masked)
        .def_readwrite("constr", &alpaqa::OCPEvalCounter::OCPEvalTimer::constr)
        .def_readwrite("constr_N", &alpaqa::OCPEvalCounter::OCPEvalTimer::constr_N)
        .def_readwrite("grad_constr_prod", &alpaqa::OCPEvalCounter::OCPEvalTimer::grad_constr_prod)
        .def_readwrite("grad_constr_prod_N", &alpaqa::OCPEvalCounter::OCPEvalTimer::grad_constr_prod_N)
        .def_readwrite("add_gn_hess_constr", &alpaqa::OCPEvalCounter::OCPEvalTimer::add_gn_hess_constr)
        .def_readwrite("add_gn_hess_constr_N", &alpaqa::OCPEvalCounter::OCPEvalTimer::add_gn_hess_constr_N)
        // clang-format on
        ;

    ocpevalcounter
        .def(py::pickle(
            [](const alpaqa::OCPEvalCounter &p) { // __getstate__
                return py::make_tuple(p.f, p.jac_f, p.grad_f_prod, p.h, p.h_N, p.l, p.l_N, p.qr,
                                      p.q_N, p.add_Q, p.add_Q_N, p.add_R_masked, p.add_S_masked,
                                      p.add_R_prod_masked, p.add_S_prod_masked, p.constr,
                                      p.constr_N, p.grad_constr_prod, p.grad_constr_prod_N,
                                      p.add_gn_hess_constr, p.add_gn_hess_constr_N, p.time);
            },
            [](py::tuple t) { // __setstate__
                if (t.size() != 22)
                    throw std::runtime_error("Invalid state!");
                using T = alpaqa::OCPEvalCounter;
                return T{
                    py::cast<decltype(T::f)>(t[0]),
                    py::cast<decltype(T::jac_f)>(t[1]),
                    py::cast<decltype(T::grad_f_prod)>(t[2]),
                    py::cast<decltype(T::h)>(t[3]),
                    py::cast<decltype(T::h_N)>(t[4]),
                    py::cast<decltype(T::l)>(t[5]),
                    py::cast<decltype(T::l_N)>(t[6]),
                    py::cast<decltype(T::qr)>(t[7]),
                    py::cast<decltype(T::q_N)>(t[8]),
                    py::cast<decltype(T::add_Q)>(t[9]),
                    py::cast<decltype(T::add_Q_N)>(t[10]),
                    py::cast<decltype(T::add_R_masked)>(t[11]),
                    py::cast<decltype(T::add_S_masked)>(t[12]),
                    py::cast<decltype(T::add_R_prod_masked)>(t[13]),
                    py::cast<decltype(T::add_S_prod_masked)>(t[14]),
                    py::cast<decltype(T::constr)>(t[15]),
                    py::cast<decltype(T::constr_N)>(t[16]),
                    py::cast<decltype(T::grad_constr_prod)>(t[17]),
                    py::cast<decltype(T::grad_constr_prod_N)>(t[18]),
                    py::cast<decltype(T::add_gn_hess_constr)>(t[19]),
                    py::cast<decltype(T::add_gn_hess_constr_N)>(t[20]),
                    py::cast<decltype(T::time)>(t[21]),
                };
            }))
        // clang-format off
        .def_readwrite("f", &alpaqa::OCPEvalCounter::f)
        .def_readwrite("jac_f", &alpaqa::OCPEvalCounter::jac_f)
        .def_readwrite("grad_f_prod", &alpaqa::OCPEvalCounter::grad_f_prod)
        .def_readwrite("h", &alpaqa::OCPEvalCounter::h)
        .def_readwrite("h_N", &alpaqa::OCPEvalCounter::h_N)
        .def_readwrite("l", &alpaqa::OCPEvalCounter::l)
        .def_readwrite("l_N", &alpaqa::OCPEvalCounter::l_N)
        .def_readwrite("qr", &alpaqa::OCPEvalCounter::qr)
        .def_readwrite("q_N", &alpaqa::OCPEvalCounter::q_N)
        .def_readwrite("add_Q", &alpaqa::OCPEvalCounter::add_Q)
        .def_readwrite("add_Q_N", &alpaqa::OCPEvalCounter::add_Q_N)
        .def_readwrite("add_R_masked", &alpaqa::OCPEvalCounter::add_R_masked)
        .def_readwrite("add_S_masked", &alpaqa::OCPEvalCounter::add_S_masked)
        .def_readwrite("add_R_prod_masked", &alpaqa::OCPEvalCounter::add_R_prod_masked)
        .def_readwrite("add_S_prod_masked", &alpaqa::OCPEvalCounter::add_S_prod_masked)
        .def_readwrite("constr", &alpaqa::OCPEvalCounter::constr)
        .def_readwrite("constr_N", &alpaqa::OCPEvalCounter::constr_N)
        .def_readwrite("grad_constr_prod", &alpaqa::OCPEvalCounter::grad_constr_prod)
        .def_readwrite("grad_constr_prod_N", &alpaqa::OCPEvalCounter::grad_constr_prod_N)
        .def_readwrite("add_gn_hess_constr", &alpaqa::OCPEvalCounter::add_gn_hess_constr)
        .def_readwrite("add_gn_hess_constr_N", &alpaqa::OCPEvalCounter::add_gn_hess_constr_N)
        .def_readwrite("time", &alpaqa::OCPEvalCounter::time)
        // clang-format on
        .def("__str__", [](const alpaqa::OCPEvalCounter &c) {
            std::ostringstream os;
            os << c;
            return os.str();
        });
}
