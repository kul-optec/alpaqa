#include <alpaqa/config/config.hpp>
#include <alpaqa/problem/kkt-error.hpp>
#include <alpaqa-python/export.h>
#include <pybind11/eigen.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <memory>
#include <span>
#include <sstream>
#include <string_view>
#include <variant>

namespace py = pybind11;
using namespace py::literals;

#include <alpaqa/problem/box-constr-problem.hpp>
#include <alpaqa/problem/problem-with-counters.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>
#include <alpaqa/problem/unconstr-problem.hpp>
#include <alpaqa/util/check-dim.hpp>
#if ALPAQA_WITH_CASADI
#include <alpaqa/casadi/CasADiProblem.hpp>
#endif
#if ALPAQA_WITH_CUTEST
#include <alpaqa/cutest/cutest-loader.hpp>
#endif
#if ALPAQA_WITH_DL
#include <alpaqa/dl/dl-problem.hpp>
#endif

// Export RTTI for types passed by std::any
#if ALPAQA_WITH_DL
template class ALPAQA_PYTHON_EXPORT std::span<std::string_view>;
template class ALPAQA_PYTHON_EXPORT std::tuple<py::args, py::kwargs>;
template struct ALPAQA_PYTHON_EXPORT
    alpaqa::detail::function_wrapper_t<py::object(void *, py::args, py::kwargs)>;
#endif

#include <util/copy.hpp>
#include <util/member.hpp>

namespace {

template <class FuncProb, auto py_f, auto f, class Ret, class... Args>
void functional_setter_ret(FuncProb &p, std::optional<py::object> o) {
    if (o) {
        p.*py_f = *std::move(o);
        p.*f    = [&pf{p.*py_f}](Args... x) -> Ret { return py::cast<Ret>(pf(x...)); };
    } else {
        p.*py_f = py::none();
        p.*f    = [](Args...) -> Ret {
            throw std::runtime_error("FunctionalProblem function is None");
        };
    }
}

template <class FuncProb, auto py_f, auto f, class Out, class Ret, class... Args>
void functional_setter_out(FuncProb &p, std::optional<py::object> o) {
    if (o) {
        p.*py_f = *std::move(o);
        p.*f    = [&pf{p.*py_f}](Args... x, Out r) -> void { r = py::cast<Ret>(pf(x...)); };
    } else {
        p.*py_f = py::none();
        p.*f    = [](Args..., Out) -> void {
            throw std::runtime_error("FunctionalProblem function is None");
        };
    }
}

template <class T, class... Args>
void problem_constr_proj_methods(py::class_<T, Args...> &cls) {
    USING_ALPAQA_CONFIG_TEMPLATE(T::config_t);
    cls //
        .def(
            "eval_projecting_difference_constraints",
            [](const T &prob, crvec z) {
                vec e(z.size());
                prob.eval_projecting_difference_constraints(z, e);
                return e;
            },
            "z"_a)
        .def(
            "eval_proximal_gradient_step",
            [](const T &prob, real_t γ, crvec x, crvec grad_ψ) {
                vec x̂(x.size());
                vec p(x.size());
                real_t hx̂ = prob.eval_proximal_gradient_step(γ, x, grad_ψ, x̂, p);
                return std::make_tuple(std::move(x̂), std::move(p), hx̂);
            },
            "γ"_a, "x"_a, "grad_ψ"_a)
        .def(
            "eval_inactive_indices_res_lna",
            [](const T &prob, real_t γ, crvec x, crvec grad_ψ) {
                indexvec J_sto(x.size());
                index_t nJ = prob.eval_inactive_indices_res_lna(γ, x, grad_ψ, J_sto);
                return indexvec{J_sto.topRows(nJ)};
            },
            "γ"_a, "x"_a, "grad_ψ"_a);
}

namespace sp = alpaqa::sparsity;

template <alpaqa::Config Conf, class F>
struct cvt_matrix_visitor_t {
    USING_ALPAQA_CONFIG(Conf);
    using result_t = std::tuple<py::object, sp::Symmetry>;
    F func;
    auto operator()(const sp::Dense<config_t> &d) const -> result_t {
        mat vals(d.rows, d.cols);
        func(vals.reshaped());
        return {
            py::cast(std::move(vals)),
            d.symmetry,
        };
    }
    template <class I>
    auto operator()(const sp::SparseCSC<config_t, I> &csc) const -> result_t {
        vec vals(csc.nnz());
        func(vals);
        auto csc_array = py::module_::import("scipy.sparse").attr("csc_array");
        auto matrix    = py::make_tuple(std::move(vals), csc.inner_idx, csc.outer_ptr);
        auto shape     = ("shape"_a = py::make_tuple(csc.rows, csc.cols));
        return {
            csc_array(std::move(matrix), std::move(shape)),
            csc.symmetry,
        };
    }
    template <class I>
    auto operator()(const sp::SparseCOO<config_t, I> &coo) const -> result_t {
        vec vals(coo.nnz());
        func(vals);
        auto coo_array = py::module_::import("scipy.sparse").attr("coo_array");
        auto Δ         = Eigen::VectorX<I>::Constant(coo.nnz(), coo.first_index);
        auto indices   = py::make_tuple(coo.row_indices - Δ, coo.col_indices - Δ);
        auto matrix    = py::make_tuple(std::move(vals), std::move(indices));
        auto shape     = ("shape"_a = py::make_tuple(coo.rows, coo.cols));
        return {
            coo_array(std::move(matrix), std::move(shape)),
            coo.symmetry,
        };
    }
};

template <alpaqa::Config Conf>
auto cvt_matrix(const alpaqa::Sparsity<Conf> &sparsity, const auto &func) {
    cvt_matrix_visitor_t<Conf, decltype(func)> visitor{func};
    return std::visit(visitor, sparsity.value);
}

template <class T, class... Args>
void problem_methods(py::class_<T, Args...> &cls) {
    USING_ALPAQA_CONFIG_TEMPLATE(T::config_t);
    cls.def_property_readonly("num_variables", &T::get_num_variables,
                              "Number of decision variables, dimension of :math:`x`");
    cls.def_property_readonly("num_constraints", &T::get_num_constraints,
                              "Number of general constraints, dimension of :math:`g(x)`");
    // clang-format off
    cls.def("__str__", &T::get_name);
    cls.def("eval_projecting_difference_constraints", &T::eval_projecting_difference_constraints, "z"_a, "e"_a);
    cls.def("eval_projection_multipliers", &T::eval_projection_multipliers, "y"_a, "M"_a);
    cls.def("eval_proximal_gradient_step", &T::eval_proximal_gradient_step, "γ"_a, "x"_a, "grad_ψ"_a, "x_hat"_a, "p"_a);
    cls.def("eval_inactive_indices_res_lna", &T::eval_inactive_indices_res_lna, "γ"_a, "x"_a, "grad_ψ"_a, "J"_a);
    cls.def("eval_objective", &T::eval_objective, "x"_a);
    cls.def("eval_objective_gradient", &T::eval_objective_gradient, "x"_a, "grad_fx"_a);
    cls.def("eval_constraints", &T::eval_constraints, "x"_a, "gx"_a);
    cls.def("eval_constraints_gradient_product", &T::eval_constraints_gradient_product, "x"_a, "y"_a, "grad_gxy"_a);
    cls.def("eval_grad_gi", &T::eval_grad_gi, "x"_a, "i"_a, "grad_gi"_a);
    // cls.def("eval_constraints_jacobian", &T::eval_constraints_jacobian, "x"_a, "J"_a); // TODO
    cls.def("eval_lagrangian_hessian_product", &T::eval_lagrangian_hessian_product, "x"_a, "y"_a, "scale"_a, "v"_a, "Hv"_a);
    // cls.def("eval_lagrangian_hessian", &T::eval_lagrangian_hessian, "x"_a, "y"_a, "H"_a); // TODO
    cls.def("eval_augmented_lagrangian_hessian_product", &T::eval_augmented_lagrangian_hessian_product, "x"_a, "y"_a, "Σ"_a, "scale"_a, "v"_a, "Hv"_a);
    // cls.def("eval_augmented_lagrangian_hessian", &T::eval_augmented_lagrangian_hessian, "x"_a, "y"_a, "Σ"_a, "H"_a); // TODO
    cls.def("eval_objective_and_gradient", &T::eval_objective_and_gradient, "x"_a, "grad_fx"_a);
    if constexpr (requires { &T::eval_objective_and_constraints; })
        cls.def("eval_objective_and_constraints", &T::eval_objective_and_constraints, "x"_a, "g"_a);
    if constexpr (requires { &T::eval_objective_gradient_and_constraints_gradient_product; })
        cls.def("eval_objective_gradient_and_constraints_gradient_product", &T::eval_objective_gradient_and_constraints_gradient_product, "x"_a, "y"_a, "grad_f"_a, "grad_gxy"_a);
    if constexpr (requires { &T::eval_lagrangian_gradient; })
        cls.def("eval_lagrangian_gradient", &T::eval_lagrangian_gradient, "x"_a, "y"_a, "grad_L"_a, "work_n"_a);
    if constexpr (requires { &T:: eval_augmented_lagrangian; })
        cls.def("eval_augmented_lagrangian", &T::eval_augmented_lagrangian, "x"_a, "y"_a, "Σ"_a, "ŷ"_a);
    if constexpr (requires { &T::eval_augmented_lagrangian_gradient; })
        cls.def("eval_augmented_lagrangian_gradient", &T::eval_augmented_lagrangian_gradient, "x"_a, "y"_a, "Σ"_a, "grad_ψ"_a, "work_n"_a, "work_m"_a);
    if constexpr (requires { &T::eval_augmented_lagrangian_and_gradient; })
        cls.def("eval_augmented_lagrangian_and_gradient", &T::eval_augmented_lagrangian_and_gradient, "x"_a, "y"_a, "Σ"_a, "grad_ψ"_a, "work_n"_a, "work_m"_a);
    if constexpr (requires { &T::check; })
        cls.def("check", &T::check);
    if constexpr (requires { &T::get_box_variables; })
        cls.def("get_box_variables", &T::get_box_variables);
    if constexpr (requires { &T::get_box_general_constraints; })
        cls.def("get_box_general_constraints", &T::get_box_general_constraints);

    if constexpr (requires { &T::provides_eval_inactive_indices_res_lna; })
        cls.def("provides_eval_inactive_indices_res_lna", &T::provides_eval_inactive_indices_res_lna);
    if constexpr (requires { &T::provides_eval_constraints_jacobian; })
        cls.def("provides_eval_constraints_jacobian", &T::provides_eval_constraints_jacobian);
    if constexpr (requires { &T::provides_get_constraints_jacobian_sparsity; })
        cls.def("provides_get_constraints_jacobian_sparsity", &T::provides_get_constraints_jacobian_sparsity);
    if constexpr (requires { &T::provides_eval_grad_gi; })
        cls.def("provides_eval_grad_gi", &T::provides_eval_grad_gi);
    if constexpr (requires { &T::provides_eval_lagrangian_hessian_product; })
        cls.def("provides_eval_lagrangian_hessian_product", &T::provides_eval_lagrangian_hessian_product);
    if constexpr (requires { &T::provides_eval_lagrangian_hessian; })
        cls.def("provides_eval_lagrangian_hessian", &T::provides_eval_lagrangian_hessian);
    if constexpr (requires { &T::provides_get_lagrangian_hessian_sparsity; })
        cls.def("provides_get_lagrangian_hessian_sparsity", &T::provides_get_lagrangian_hessian_sparsity);
    if constexpr (requires { &T::provides_eval_augmented_lagrangian_hessian_product; })
        cls.def("provides_eval_augmented_lagrangian_hessian_product", &T::provides_eval_augmented_lagrangian_hessian_product);
    if constexpr (requires { &T::provides_eval_augmented_lagrangian_hessian; })
        cls.def("provides_eval_augmented_lagrangian_hessian", &T::provides_eval_augmented_lagrangian_hessian);
    if constexpr (requires { &T::provides_get_augmented_lagrangian_hessian_sparsity; })
        cls.def("provides_get_augmented_lagrangian_hessian_sparsity", &T::provides_get_augmented_lagrangian_hessian_sparsity);
    if constexpr (requires { &T::provides_eval_objective_and_gradient; })
        cls.def("provides_eval_objective_and_gradient", &T::provides_eval_objective_and_gradient);
    if constexpr (requires { &T::provides_eval_objective_and_constraints; })
        cls.def("provides_eval_objective_and_constraints", &T::provides_eval_objective_and_constraints);
    if constexpr (requires { &T::provides_eval_objective_gradient_and_constraints_gradient_product; })
        cls.def("provides_eval_objective_gradient_and_constraints_gradient_product", &T::provides_eval_objective_gradient_and_constraints_gradient_product);
    if constexpr (requires { &T::provides_eval_lagrangian_gradient; })
        cls.def("provides_eval_lagrangian_gradient", &T::provides_eval_lagrangian_gradient);
    if constexpr (requires { &T::provides_eval_augmented_lagrangian; })
        cls.def("provides_eval_augmented_lagrangian", &T::provides_eval_augmented_lagrangian);
    if constexpr (requires { &T::provides_eval_augmented_lagrangian_gradient; })
        cls.def("provides_eval_augmented_lagrangian_gradient", &T::provides_eval_augmented_lagrangian_gradient);
    if constexpr (requires { &T::provides_eval_augmented_lagrangian_and_gradient; })
        cls.def("provides_eval_augmented_lagrangian_and_gradient", &T::provides_eval_augmented_lagrangian_and_gradient);
    if constexpr (requires { &T::provides_check; })
        cls.def("provides_check", &T::provides_check);
    if constexpr (requires { &T::provides_get_box_variables; })
        cls.def("provides_get_box_variables", &T::provides_get_box_variables);
    if constexpr (requires { &T::provides_get_box_general_constraints; })
        cls.def("provides_get_box_general_constraints", &T::provides_get_box_general_constraints);
    // clang-format on
    cls.def(
           "eval_projecting_difference_constraints",
           [](const T &prob, crvec z) {
               vec e(prob.get_num_constraints());
               prob.eval_projecting_difference_constraints(z, e);
               return e;
           },
           "z"_a)
        .def(
            "eval_proximal_gradient_step",
            [](const T &prob, real_t γ, crvec x, crvec grad_ψ) {
                vec x̂(prob.get_num_variables());
                vec p(prob.get_num_variables());
                real_t hx̂ = prob.eval_proximal_gradient_step(γ, x, grad_ψ, x̂, p);
                return std::make_tuple(std::move(x̂), std::move(p), hx̂);
            },
            "γ"_a, "x"_a, "grad_ψ"_a)
        .def(
            "eval_inactive_indices_res_lna",
            [](const T &prob, real_t γ, crvec x, crvec grad_ψ) {
                indexvec J_sto(prob.get_num_variables());
                index_t nJ = prob.eval_inactive_indices_res_lna(γ, x, grad_ψ, J_sto);
                return indexvec{J_sto.topRows(nJ)};
            },
            "γ"_a, "x"_a, "grad_ψ"_a)
        .def(
            "eval_objective_gradient",
            [](const T &p, crvec x) {
                vec g(p.get_num_variables());
                p.eval_objective_gradient(x, g);
                return g;
            },
            "x"_a)
        .def(
            "eval_constraints",
            [](const T &p, crvec x) {
                vec g(p.get_num_constraints());
                p.eval_constraints(x, g);
                return g;
            },
            "x"_a)
        .def(
            "eval_constraints_gradient_product",
            [](const T &p, crvec x, crvec y) {
                vec g(p.get_num_variables());
                p.eval_constraints_gradient_product(x, y, g);
                return g;
            },
            "x"_a, "y"_a);
    if constexpr (requires { &T::eval_objective_and_gradient; })
        cls.def(
            "eval_objective_and_gradient",
            [](const T &p, crvec x) {
                vec g(p.get_num_variables());
                real_t f = p.eval_objective_and_gradient(x, g);
                return py::make_tuple(f, std::move(g));
            },
            "x"_a);
    if constexpr (requires { &T::eval_augmented_lagrangian; })
        cls.def(
            "eval_augmented_lagrangian",
            [](const T &p, crvec x, crvec y, crvec Σ) {
                vec ŷ(p.get_num_constraints());
                auto ψ = p.eval_augmented_lagrangian(x, y, Σ, ŷ);
                return std::make_tuple(std::move(ψ), std::move(ŷ));
            },
            "x"_a, "y"_a, "Σ"_a);
    if constexpr (requires { &T::eval_augmented_lagrangian_gradient; })
        cls.def(
            "eval_augmented_lagrangian_gradient",
            [](const T &p, crvec x, crvec y, crvec Σ) {
                vec grad_ψ(p.get_num_variables()), work_n(p.get_num_variables()), work_m(p.get_num_constraints());
                p.eval_augmented_lagrangian_gradient(x, y, Σ, grad_ψ, work_n, work_m);
                return grad_ψ;
            },
            "x"_a, "y"_a, "Σ"_a);
    if constexpr (requires { &T::eval_augmented_lagrangian_and_gradient; })
        cls.def(
            "eval_augmented_lagrangian_and_gradient",
            [](const T &p, crvec x, crvec y, crvec Σ) {
                vec grad_ψ(p.get_num_variables()), work_n(p.get_num_variables()), work_m(p.get_num_constraints());
                auto ψ = p.eval_augmented_lagrangian_and_gradient(x, y, Σ, grad_ψ, work_n, work_m);
                return std::make_tuple(std::move(ψ), std::move(grad_ψ));
            },
            "x"_a, "y"_a, "Σ"_a);
    if constexpr (requires { &T::eval_constraints_jacobian; })
        cls.def(
            "eval_constraints_jacobian",
            [&](const T &p, crvec x) {
                return cvt_matrix(p.get_constraints_jacobian_sparsity(),
                                  [&](rvec values) { return p.eval_constraints_jacobian(x, values); });
            },
            "x"_a, "Returns the Jacobian of the constraints and its symmetry.");
    if constexpr (requires { &T::eval_lagrangian_hessian; })
        cls.def(
            "eval_lagrangian_hessian",
            [&](const T &p, crvec x, crvec y, real_t scale) {
                return cvt_matrix(p.get_lagrangian_hessian_sparsity(),
                                  [&](rvec values) { return p.eval_lagrangian_hessian(x, y, scale, values); });
            },
            "x"_a, "y"_a, "scale"_a = 1.,
            "Returns the Hessian of the Lagrangian and its symmetry.");
    if constexpr (requires { &T::eval_augmented_lagrangian_hessian; })
        cls.def(
            "eval_augmented_lagrangian_hessian",
            [&](const T &p, crvec x, crvec y, crvec Σ, real_t scale) {
                return cvt_matrix(p.get_augmented_lagrangian_hessian_sparsity(), [&](rvec values) {
                    return p.eval_augmented_lagrangian_hessian(x, y, Σ, scale, values);
                });
            },
            "x"_a, "y"_a, "Σ"_a, "scale"_a = 1.,
            "Returns the Hessian of the augmented Lagrangian and its symmetry.");
}

} // namespace

template <alpaqa::Config Conf>
void register_problems(py::module_ &m) {
    USING_ALPAQA_CONFIG(Conf);
    using alpaqa::util::check_dim;

    using Box = alpaqa::Box<config_t>;
    py::class_<Box> box(m, "Box", "C++ documentation: :cpp:class:`alpaqa::Box`");
    default_copy_methods(box);
    box //
        .def(py::pickle(
            [](const Box &b) { // __getstate__
                return py::make_tuple(b.upperbound, b.lowerbound);
            },
            [](py::tuple t) { // __setstate__
                if (t.size() != 2)
                    throw std::runtime_error("Invalid state!");
                return Box::from_lower_upper(py::cast<decltype(Box::lowerbound)>(t[1]),
                                             py::cast<decltype(Box::upperbound)>(t[0]));
            }))
        .def(py::init<length_t>(), "n"_a,
             "Create an :math:`n`-dimensional box at with bounds at "
             ":math:`\\pm\\infty` (no constraints).")
        .def(py::init([](vec lower, vec upper) {
                 if (lower.size() != upper.size())
                     throw std::invalid_argument("Upper and lower bound dimensions do not match");
                 return Box::from_lower_upper(std::move(lower), std::move(upper));
             }),
             py::kw_only(), "lower"_a, "upper"_a, "Create a box with the given bounds.")
        .def_property("lowerbound", vector_getter<&Box::lowerbound>(),
                      vector_setter<&Box::lowerbound>("lowerbound"))
        .def_property("upperbound", vector_getter<&Box::upperbound>(),
                      vector_setter<&Box::upperbound>("upperbound"));

    using BoxConstrProblem = alpaqa::BoxConstrProblem<config_t>;
    py::class_<BoxConstrProblem> box_constr_problem(
        m, "BoxConstrProblem", "C++ documentation: :cpp:class:`alpaqa::BoxConstrProblem`");
    default_copy_methods(box_constr_problem);
    box_constr_problem //
        .def(py::init<length_t, length_t>(), "num_variables"_a, "num_constraints"_a,
             ":param n: Number of unknowns\n"
             ":param m: Number of constraints")
        .def(py::pickle(
            [](const BoxConstrProblem &self) { // __getstate__
                self.check();
                return py::make_tuple(self.C, self.D, self.l1_reg, self.penalty_alm_split);
            },
            [](py::tuple t) { // __setstate__
                if (t.size() != 4)
                    throw std::runtime_error("Invalid state!");
                return BoxConstrProblem{
                    py::cast<Box>(t[0]),
                    py::cast<Box>(t[1]),
                    py::cast<vec>(t[2]),
                    py::cast<index_t>(t[3]),
                };
            }))
        .def_property_readonly("num_variables", &BoxConstrProblem::get_num_variables,
                               "Number of decision variables, dimension of :math:`x`")
        .def_property_readonly("num_constraints", &BoxConstrProblem::get_num_constraints,
                               "Number of general constraints, dimension of :math:`g(x)`")
        .def("resize", &BoxConstrProblem::resize, "num_variables"_a, "num_constraints"_a)
        .def_readwrite("C", &BoxConstrProblem::C, "Box constraints on :math:`x`")
        .def_readwrite("D", &BoxConstrProblem::D, "Box constraints on :math:`g(x)`")
        .def_readwrite("l1_reg", &BoxConstrProblem::l1_reg,
                       py::return_value_policy::reference_internal,
                       ":math:`\\ell_1` regularization on :math:`x`")
        .def_readwrite("penalty_alm_split", &BoxConstrProblem::penalty_alm_split,
                       py::return_value_policy::reference_internal,
                       "Index between quadratic penalty and augmented Lagrangian constraints")
        .def("eval_projecting_difference_constraints", &BoxConstrProblem::eval_projecting_difference_constraints, "z"_a, "e"_a)
        .def("eval_projection_multipliers", &BoxConstrProblem::eval_projection_multipliers, "y"_a, "M"_a)
        .def("eval_proximal_gradient_step", &BoxConstrProblem::eval_proximal_gradient_step, "γ"_a, "x"_a,
             "grad_ψ"_a, "x_hat"_a, "p"_a)
        .def("eval_inactive_indices_res_lna", &BoxConstrProblem::eval_inactive_indices_res_lna,
             "γ"_a, "x"_a, "grad_ψ"_a, "J"_a)
        .def("get_box_variables", &BoxConstrProblem::get_box_variables)
        .def("get_box_general_constraints", &BoxConstrProblem::get_box_general_constraints);
    problem_constr_proj_methods(box_constr_problem);

    using UnconstrProblem = alpaqa::UnconstrProblem<config_t>;
    py::class_<UnconstrProblem> unconstr_problem(
        m, "UnconstrProblem", "C++ documentation: :cpp:class:`alpaqa::UnconstrProblem`");
    default_copy_methods(unconstr_problem);
    unconstr_problem //
        .def(py::init<length_t>(), "num_variables"_a,
             ":param n: Number of unknowns")
        .def(py::pickle(
            [](const UnconstrProblem &self) { // __getstate__
                return py::make_tuple(self.n);
            },
            [](py::tuple t) { // __setstate__
                if (t.size() != 1)
                    throw std::runtime_error("Invalid state!");
                return UnconstrProblem{py::cast<length_t>(t[0])};
            }))
        .def_property_readonly("num_variables", &UnconstrProblem::get_num_variables,
                               "Number of decision variables, dimension of :math:`x`")
        .def_property_readonly("num_constraints", &UnconstrProblem::get_num_constraints,
                               "Number of general constraints, dimension of :math:`g(x)`")
        .def("resize", &UnconstrProblem::resize, "num_variables"_a)
        .def("eval_constraints", &UnconstrProblem::eval_constraints, "x"_a, "g"_a)
        .def("eval_constraints_gradient_product", &UnconstrProblem::eval_constraints_gradient_product, "x"_a, "y"_a, "grad_gxy"_a)
        .def("eval_constraints_jacobian", &UnconstrProblem::eval_constraints_jacobian, "x"_a, "J_values"_a)
        .def("eval_grad_gi", &UnconstrProblem::eval_grad_gi, "x"_a, "i"_a, "grad_gi"_a)
        .def("eval_projecting_difference_constraints", &UnconstrProblem::eval_projecting_difference_constraints, "z"_a, "e"_a)
        .def("eval_projection_multipliers", &UnconstrProblem::eval_projection_multipliers, "y"_a, "M"_a)
        .def("eval_proximal_gradient_step", &UnconstrProblem::eval_proximal_gradient_step, "γ"_a, "x"_a, "grad_ψ"_a,
             "x_hat"_a, "p"_a)
        .def("eval_inactive_indices_res_lna", &UnconstrProblem::eval_inactive_indices_res_lna,
             "γ"_a, "x"_a, "grad_ψ"_a, "J"_a);
    problem_constr_proj_methods(unconstr_problem);

    struct PyProblem {
        USING_ALPAQA_CONFIG(Conf);
        py::object o;

        PyProblem(py::object o) : o{std::move(o)} {}

        // clang-format off
        void eval_projecting_difference_constraints(crvec z, rvec e) const { py::gil_scoped_acquire gil; o.attr("eval_projecting_difference_constraints")(z, e); }
        void eval_projection_multipliers(rvec y, real_t M) const { py::gil_scoped_acquire gil; o.attr("eval_projection_multipliers")(y, M); }
        real_t eval_proximal_gradient_step(real_t γ, crvec x, crvec grad_ψ, rvec x̂, rvec p) const { py::gil_scoped_acquire gil; return py::cast<real_t>(o.attr("eval_proximal_gradient_step")(γ, x, grad_ψ, x̂, p)); }
        index_t eval_inactive_indices_res_lna(real_t γ, crvec x, crvec grad_ψ, rindexvec J) const { py::gil_scoped_acquire gil; return py::cast<index_t>(o.attr("eval_inactive_indices_res_lna")(γ, x, grad_ψ, J)); }
        real_t eval_objective(crvec x) const { py::gil_scoped_acquire gil; return py::cast<real_t>(o.attr("eval_objective")(x)); }
        void eval_objective_gradient(crvec x, rvec grad_fx) const { py::gil_scoped_acquire gil; o.attr("eval_objective_gradient")(x, grad_fx); }
        void eval_constraints(crvec x, rvec gx) const { py::gil_scoped_acquire gil; o.attr("eval_constraints")(x, gx); }
        void eval_constraints_gradient_product(crvec x, crvec y, rvec grad_gxy) const { py::gil_scoped_acquire gil; o.attr("eval_constraints_gradient_product")(x, y, grad_gxy); }
        void eval_grad_gi(crvec x, index_t i, rvec grad_gi) const { py::gil_scoped_acquire gil; o.attr("eval_grad_gi")(x, i, grad_gi); }
        void eval_lagrangian_hessian_product(crvec x, crvec y, real_t scale, crvec v, rvec Hv) const { py::gil_scoped_acquire gil; o.attr("eval_lagrangian_hessian_product")(x, y, scale, v, Hv); }
        // void eval_lagrangian_hessian(crvec x, crvec y, rmat H) const { py::gil_scoped_acquire gil; o.attr("eval_lagrangian_hessian")(x, y, H); } // TODO
        void eval_augmented_lagrangian_hessian_product(crvec x, crvec y, crvec Σ, real_t scale, crvec v, rvec Hv) const { py::gil_scoped_acquire gil; o.attr("eval_augmented_lagrangian_hessian_product")(x, y, Σ, scale, v, Hv); }
        // void eval_augmented_lagrangian_hessian(crvec x, crvec y, crvec Σ, rmat H) const { py::gil_scoped_acquire gil; o.attr("eval_augmented_lagrangian_hessian")(x, y, Σ, H); } // TODO
        real_t eval_objective_and_gradient(crvec x, rvec grad_fx) const { py::gil_scoped_acquire gil; return py::cast<real_t>(o.attr("eval_objective_and_gradient")(x, grad_fx)); }
        real_t eval_objective_and_constraints(crvec x, rvec g) const { py::gil_scoped_acquire gil; return py::cast<real_t>(o.attr("eval_objective_and_constraints")(x, g)); }
        void eval_objective_gradient_and_constraints_gradient_product(crvec x, crvec y, rvec grad_f, rvec grad_gxy) const { py::gil_scoped_acquire gil; o.attr("eval_objective_gradient_and_constraints_gradient_product")(x, y, grad_f, grad_gxy); }
        void eval_lagrangian_gradient(crvec x, crvec y, rvec grad_L, rvec work_n) const { py::gil_scoped_acquire gil; o.attr("eval_lagrangian_gradient")(x, y, grad_L, work_n); }
        real_t eval_augmented_lagrangian(crvec x, crvec y, crvec Σ, rvec ŷ) const { py::gil_scoped_acquire gil; return py::cast<real_t>(o.attr("eval_augmented_lagrangian")(x, y, Σ, ŷ)); }
        void eval_augmented_lagrangian_gradient(crvec x, crvec y, crvec Σ, rvec grad_ψ, rvec work_n, rvec work_m) const { py::gil_scoped_acquire gil; o.attr("eval_augmented_lagrangian_gradient")(x, y, Σ, grad_ψ, work_n, work_m); }
        real_t eval_augmented_lagrangian_and_gradient(crvec x, crvec y, crvec Σ, rvec grad_ψ, rvec work_n, rvec work_m) const { py::gil_scoped_acquire gil; return py::cast<real_t>(o.attr("eval_augmented_lagrangian_and_gradient")(x, y, Σ, grad_ψ, work_n, work_m)); }
        void check() const { py::gil_scoped_acquire gil; if (auto ch = py::getattr(o, "check", py::none()); !ch.is_none()) ch(); }
        std::string get_name() const { py::gil_scoped_acquire gil; return py::str(o); }
        const Box &get_box_variables() const { py::gil_scoped_acquire gil; alpaqa::ScopedMallocAllower ma; C = py::cast<Box>(o.attr("get_box_variables")()); return C; }
        const Box &get_box_general_constraints() const { py::gil_scoped_acquire gil; alpaqa::ScopedMallocAllower ma; D = py::cast<Box>(o.attr("get_box_general_constraints")()); return D; }

        [[nodiscard]] bool provides_eval_inactive_indices_res_lna() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_inactive_indices_res_lna") && (!py::hasattr(o, "provides_eval_inactive_indices_res_lna") || py::cast<bool>(o.attr("provides_eval_inactive_indices_res_lna")())); }
        [[nodiscard]] bool provides_eval_grad_gi() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_grad_gi") && (!py::hasattr(o, "provides_eval_grad_gi") || py::cast<bool>(o.attr("provides_eval_grad_gi")())); }
        [[nodiscard]] bool provides_eval_lagrangian_hessian_product() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_lagrangian_hessian_product") && (!py::hasattr(o, "provides_eval_lagrangian_hessian_product") || py::cast<bool>(o.attr("provides_eval_lagrangian_hessian_product")())); }
        // [[nodiscard]] bool provides_eval_lagrangian_hessian() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_lagrangian_hessian") && (!py::hasattr(o, "provides_eval_lagrangian_hessian") || py::cast<bool>(o.attr("provides_eval_lagrangian_hessian")())); }
        [[nodiscard]] bool provides_eval_augmented_lagrangian_hessian_product() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_augmented_lagrangian_hessian_product") && (!py::hasattr(o, "provides_eval_augmented_lagrangian_hessian_product") || py::cast<bool>(o.attr("provides_eval_augmented_lagrangian_hessian_product")())); }
        // [[nodiscard]] bool provides_eval_augmented_lagrangian_hessian() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_augmented_lagrangian_hessian") && (!py::hasattr(o, "provides_eval_augmented_lagrangian_hessian") || py::cast<bool>(o.attr("provides_eval_augmented_lagrangian_hessian")())); }
        [[nodiscard]] bool provides_eval_objective_and_gradient() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_objective_and_gradient") && (!py::hasattr(o, "provides_eval_objective_and_gradient") || py::cast<bool>(o.attr("provides_eval_objective_and_gradient")())); }
        [[nodiscard]] bool provides_eval_objective_and_constraints() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_objective_and_constraints") && (!py::hasattr(o, "provides_eval_objective_and_constraints") || py::cast<bool>(o.attr("provides_eval_objective_and_constraints")())); }
        [[nodiscard]] bool provides_eval_objective_gradient_and_constraints_gradient_product() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_objective_gradient_and_constraints_gradient_product") && (!py::hasattr(o, "provides_eval_objective_gradient_and_constraints_gradient_product") || py::cast<bool>(o.attr("provides_eval_objective_gradient_and_constraints_gradient_product")())); }
        [[nodiscard]] bool provides_eval_lagrangian_gradient() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_lagrangian_gradient") && (!py::hasattr(o, "provides_eval_lagrangian_gradient") || py::cast<bool>(o.attr("provides_eval_lagrangian_gradient")())); }
        [[nodiscard]] bool provides_eval_augmented_lagrangian() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_augmented_lagrangian") && (!py::hasattr(o, "provides_eval_augmented_lagrangian") || py::cast<bool>(o.attr("provides_eval_augmented_lagrangian")())); }
        [[nodiscard]] bool provides_eval_augmented_lagrangian_gradient() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_augmented_lagrangian_gradient") && (!py::hasattr(o, "provides_eval_augmented_lagrangian_gradient") || py::cast<bool>(o.attr("provides_eval_augmented_lagrangian_gradient")())); }
        [[nodiscard]] bool provides_eval_augmented_lagrangian_and_gradient() const { py::gil_scoped_acquire gil; return py::hasattr(o, "eval_augmented_lagrangian_and_gradient") && (!py::hasattr(o, "provides_eval_augmented_lagrangian_and_gradient") || py::cast<bool>(o.attr("provides_eval_augmented_lagrangian_and_gradient")())); }
        [[nodiscard]] bool provides_check() const { py::gil_scoped_acquire gil; return py::hasattr(o, "check") && (!py::hasattr(o, "provides_check") || py::cast<bool>(o.attr("provides_check")())); }
        [[nodiscard]] bool provides_get_box_variables() const { py::gil_scoped_acquire gil; return py::hasattr(o, "get_box_variables") && (!py::hasattr(o, "provides_get_box_variables") || py::cast<bool>(o.attr("provides_get_box_variables")())); }
        [[nodiscard]] bool provides_get_box_general_constraints() const { py::gil_scoped_acquire gil; return py::hasattr(o, "get_box_general_constraints") && (!py::hasattr(o, "provides_get_box_general_constraints") || py::cast<bool>(o.attr("provides_get_box_general_constraints")())); }

        length_t get_num_variables() const { py::gil_scoped_acquire gil; return py::cast<length_t>(o.attr("num_variables")); }
        length_t get_num_constraints() const { py::gil_scoped_acquire gil; return py::cast<length_t>(o.attr("num_constraints")); }
        // clang-format on

        // To keep the references to the boxes alive
        mutable Box C;
        mutable Box D;
    };

    using TEProblem = alpaqa::TypeErasedProblem<config_t>;
    py::class_<TEProblem> te_problem(m, "Problem",
                                     "C++ documentation: :cpp:class:`alpaqa::TypeErasedProblem`");
    default_copy_methods(te_problem);
    problem_methods(te_problem);

    // ProblemWithCounters
    struct ProblemWithCounters {
        TEProblem problem;
        std::shared_ptr<alpaqa::EvalCounter> evaluations;
    };
    py::class_<ProblemWithCounters>(m, "ProblemWithCounters")
        .def_readonly("problem", &ProblemWithCounters::problem)
        .def_readonly("evaluations", &ProblemWithCounters::evaluations);
    static constexpr auto te_pwc = []<class P>(P &&p) -> ProblemWithCounters {
        using PwC = alpaqa::ProblemWithCounters<P>;
        auto te_p = TEProblem::template make<PwC>(std::forward<P>(p));
        auto eval = te_p.template as<PwC>().evaluations;
        return {std::move(te_p), std::move(eval)};
    };

    if constexpr (std::is_same_v<typename Conf::real_t, double>) {
#if ALPAQA_WITH_CASADI
        using CasADiProblem      = alpaqa::CasADiProblem<config_t>;
        auto load_CasADi_problem = [](const char *so_name) {
            return std::make_unique<CasADiProblem>(so_name);
        };
        auto deserialize_CasADi_problem = [](std::map<std::string, std::string> functions) {
            return std::make_unique<CasADiProblem>(
                alpaqa::SerializedCasADiFunctions{std::move(functions)});
        };
#else
        struct CasADiProblem : BoxConstrProblem {};
        auto load_CasADi_problem = [](const char *) -> std::unique_ptr<CasADiProblem> {
            throw std::runtime_error("This version of alpaqa was compiled without CasADi support");
        };
        auto deserialize_CasADi_problem =
            [](std::map<std::string, std::string>) -> std::unique_ptr<CasADiProblem> {
            throw std::runtime_error("This version of alpaqa was compiled without CasADi support");
        };
#endif

        py::class_<CasADiProblem, BoxConstrProblem> casadi_problem(
            m, "CasADiProblem",
            "C++ documentation: :cpp:class:`alpaqa::CasADiProblem`\n\n"
            "See :py:class:`alpaqa.Problem` for the full documentation.");
        default_copy_methods(casadi_problem);
#if ALPAQA_WITH_CASADI
        problem_methods(casadi_problem);
        casadi_problem.def_property(
            "param", [](CasADiProblem &p) -> rvec { return p.param; },
            [](CasADiProblem &p, crvec param) {
                alpaqa::util::check_dim_msg(param, p.param.size(), "Invalid parameter size");
                p.param = param;
            },
            "Parameter vector :math:`p` of the problem");
        te_problem.def(py::init<const CasADiProblem &>(), "problem"_a, "Explicit conversion.");
        py::implicitly_convertible<CasADiProblem, TEProblem>();
#endif

        m.def("load_casadi_problem", load_CasADi_problem, "so_name"_a,
              "Load a compiled CasADi problem.\n\n");

        m.def("deserialize_casadi_problem", deserialize_CasADi_problem, "functions"_a,
              "Deserialize a CasADi problem from the given serialized functions.\n\n");

#if ALPAQA_WITH_CASADI
        m.def(
            "problem_with_counters", [](CasADiProblem &p) { return te_pwc(p); },
            py::keep_alive<0, 1>(), "problem"_a,
            "Wrap the problem to count all function evaluations.\n\n"
            ":param problem: The original problem to wrap. Copied.\n"
            ":return: * Wrapped problem.\n"
            "         * Counters for wrapped problem.\n\n");
#endif

#if ALPAQA_WITH_CUTEST
        using alpaqa::CUTEstProblem;
        py::class_<CUTEstProblem, BoxConstrProblem> cutest_problem(
            m, "CUTEstProblem",
            "C++ documentation: :cpp:class:`alpaqa::CUTEstProblem`\n\n"
            "See :py:class:`alpaqa.Problem` for the full documentation.");
        cutest_problem.def(
            py::init<const char *, const char *, bool, alpaqa::DynamicLoadFlags>(), "so_filename"_a,
            "outsdiff_filename"_a = nullptr, "sparse"_a = false,
            py::arg_v("dl_flags", alpaqa::DynamicLoadFlags{}, "..."),
            "Load a CUTEst problem from the given shared library and OUTSDIF.d file");
        default_copy_methods(cutest_problem);
        problem_methods(cutest_problem);
        py::class_<CUTEstProblem::Report> report(cutest_problem, "Report");
        py::class_<CUTEstProblem::Report::Calls> calls(report, "Calls");
        calls.def_readwrite("objective", &CUTEstProblem::Report::Calls::objective)
            .def_readwrite("objective_grad", &CUTEstProblem::Report::Calls::objective_grad)
            .def_readwrite("objective_hess", &CUTEstProblem::Report::Calls::objective_hess)
            .def_readwrite("hessian_times_vector",
                           &CUTEstProblem::Report::Calls::hessian_times_vector)
            .def_readwrite("constraints", &CUTEstProblem::Report::Calls::constraints)
            .def_readwrite("constraints_grad", &CUTEstProblem::Report::Calls::constraints_grad)
            .def_readwrite("constraints_hess", &CUTEstProblem::Report::Calls::constraints_hess);
        report.def_readwrite("calls", &CUTEstProblem::Report::calls)
            .def_readwrite("time_setup", &CUTEstProblem::Report::time_setup)
            .def_readwrite("time", &CUTEstProblem::Report::time);
        cutest_problem
            .def("get_report", &CUTEstProblem::get_report,
                 "Get the report generated by cutest_creport.")
            .def(
                "format_report",
                [](const CUTEstProblem &self, std::optional<CUTEstProblem::Report> r) {
                    std::ostringstream oss;
                    if (r)
                        self.format_report(oss, *r);
                    else
                        self.format_report(oss);
                    return std::move(oss).str();
                },
                "report"_a = std::nullopt, "Convert the given report to a string.")
            .def_readwrite("x0", &CUTEstProblem::x0, "Initial guess for decision variables.")
            .def_readwrite("y0", &CUTEstProblem::y0, "Initial guess for multipliers.")
            .def_readonly("name", &CUTEstProblem::name, "CUTEst problem name.");
        te_problem.def(py::init<const CUTEstProblem &>(), "problem"_a, "Explicit conversion.");
        py::implicitly_convertible<CUTEstProblem, TEProblem>();
        m.def(
            "problem_with_counters", [](CUTEstProblem &p) { return te_pwc(p); },
            py::keep_alive<0, 1>(), "problem"_a,
            "Wrap the problem to count all function evaluations.\n\n"
            ":param problem: The original problem to wrap. Copied.\n"
            ":return: * Wrapped problem.\n"
            "         * Counters for wrapped problem.\n\n");
#endif
#if ALPAQA_WITH_DL
        using alpaqa::dl::DLProblem;
        py::class_<DLProblem, BoxConstrProblem> dl_problem(
            m, "DLProblem",
            "C++ documentation: :cpp:class:`alpaqa::dl::DLProblem`\n\n"
            "See :py:class:`alpaqa.Problem` for the full documentation.");
        dl_problem.def(
            py::init([](const std::string &so_filename, py::args args, std::string function_name,
                        bool user_param_str, py::kwargs kwargs) {
                if (user_param_str) {
                    std::vector<std::string_view> str_opts;
                    str_opts.resize(args.size());
                    std::transform(args.begin(), args.end(), str_opts.begin(),
                                   [](const auto &e) { return py::cast<std::string_view>(e); });
                    std::span<std::string_view> user_param{str_opts};
                    return DLProblem{
                        so_filename,
                        std::move(function_name),
                        user_param,
                    };
                } else {
                    std::tuple<py::args, py::kwargs> user_param{
                        std::move(args),
                        std::move(kwargs),
                    };
                    return DLProblem{
                        so_filename,
                        std::move(function_name),
                        {&user_param, alpaqa_register_arg_py_args},
                    };
                }
            }),
            "so_filename"_a, py::kw_only{}, "function_name"_a = "register_alpaqa_problem",
            "user_param_str"_a = false,
            "Load a problem from the given shared library file.\n"
            "By default, extra arguments are passed to the problem as a void pointer "
            "to a ``std::tuple<pybind11::args, pybind11::kwargs>``.\n"
            "If the keyword argument ``user_param_str=True`` is used, the ``args`` "
            "is converted to a list of strings, and passed as a void pointer to a "
            "``std::span<std::string_view>``.");
        default_copy_methods(dl_problem);
        problem_methods(dl_problem);
        dl_problem.def(
            "call_extra_func",
            [](DLProblem &self, const std::string &name, py::args args, py::kwargs kwargs) {
                using instance_t = alpaqa::dl::ExtraFuncs::instance_t;
                return self.call_extra_func<py::object(instance_t *, py::args, py::kwargs)>(
                    name, std::move(args), std::move(kwargs));
            },
            "name"_a,
            "Call the given extra member function registered by the problem, with the "
            "signature "
            "``pybind11::object(pybind11::args, pybind11::kwargs)``.");
        te_problem.def(py::init<const DLProblem &>(), "problem"_a, "Explicit conversion.");
        py::implicitly_convertible<DLProblem, TEProblem>();
        m.def(
            "problem_with_counters", [](DLProblem &p) { return te_pwc(p); }, py::keep_alive<0, 1>(),
            "problem"_a,
            "Wrap the problem to count all function evaluations.\n\n"
            ":param problem: The original problem to wrap. Copied.\n"
            ":return: * Wrapped problem.\n"
            "         * Counters for wrapped problem.\n\n");
#endif
    }
    m.def(
        "problem_with_counters", [](py::object p) { return te_pwc(PyProblem{std::move(p)}); },
        py::keep_alive<0, 1>(), "problem"_a);

    m.def(
        "provided_functions",
        [](const TEProblem &problem) {
            std::ostringstream os;
            alpaqa::print_provided_functions(os, problem);
            return os.str();
        },
        "problem"_a, "Returns a string representing the functions provided by the problem.");

    using KKTError = alpaqa::KKTError<config_t>;
    py::class_<KKTError>(m, "KKTError", "C++ documentation: :cpp:class:`alpaqa::KKTError`")
        .def_readwrite("stationarity", &KKTError::stationarity)
        .def_readwrite("constr_violation", &KKTError::constr_violation)
        .def_readwrite("complementarity", &KKTError::complementarity)
        .def_readwrite("bounds_violation", &KKTError::bounds_violation);
    m.def("kkt_error", [](const TEProblem &problem, crvec x, crvec y) {
        return alpaqa::compute_kkt_error(problem, x, y);
    });

    // Must be last
    te_problem.def(py::init([](py::object o) { return TEProblem::template make<PyProblem>(o); }),
                   "problem"_a, "Explicit conversion from a custom Python class.");
}

template void register_problems<alpaqa::EigenConfigd>(py::module_ &);
ALPAQA_IF_FLOAT(template void register_problems<alpaqa::EigenConfigf>(py::module_ &);)
ALPAQA_IF_LONGD(template void register_problems<alpaqa::EigenConfigl>(py::module_ &);)
ALPAQA_IF_QUADF(template void register_problems<alpaqa::EigenConfigq>(py::module_ &);)
