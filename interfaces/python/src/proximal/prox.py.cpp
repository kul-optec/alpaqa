#include <alpaqa/util/quadmath/quadmath.hpp>

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace py::literals;

#include <alpaqa/functions/indicator-box.hpp>
#include <alpaqa/functions/l1-norm.hpp>
#include <alpaqa/functions/nuclear-norm.hpp>
#include <alpaqa/functions/prox.hpp>

template <alpaqa::Config Conf, class T>
void register_prox_func(py::module_ &m) {
    USING_ALPAQA_CONFIG(Conf);
    m.def(
        "prox",
        [](T &self, crmat in, rmat out, real_t γ) {
            return alpaqa::prox(self, std::move(in), out, γ);
        },
        "self"_a, "input"_a, "output"_a, "γ"_a = 1,
        "C++ documentation: :cpp:var:`alpaqa::prox`\n"
        "Compute the proximal mapping of ``self`` at ``in`` with step size ``γ``.");
    m.def(
        "prox",
        [](T &self, crmat in, real_t γ) {
            mat out(in.rows(), in.cols());
            auto h_out = alpaqa::prox(self, std::move(in), out, γ);
            return std::make_tuple(h_out, std::move(out));
        },
        "self"_a, "input"_a, "γ"_a = 1,
        "C++ documentation: :cpp:var:`alpaqa::prox`\n"
        "Compute the proximal mapping of ``self`` at ``in`` with step size ``γ``.");
    m.def(
        "prox_step",
        [](T &self, crmat in, crmat in_step, rmat out, rmat out_step, real_t γ, real_t γ_step) {
            return alpaqa::prox_step(self, std::move(in), std::move(in_step), out, out_step, γ,
                                     γ_step);
        },
        "self"_a, "input"_a, "input_step"_a, "output"_a, "output_step"_a, "γ"_a = 1,
        "γ_step"_a = -1,
        "C++ documentation: :cpp:var:`alpaqa::prox_step`\n"
        "Compute a generalized forward-backward step.");
    m.def(
        "prox_step",
        [](T &self, crmat in, crmat in_step, real_t γ, real_t γ_step) {
            mat out(in.rows(), in.cols()), out_step(in.rows(), in.cols());
            auto h_out = alpaqa::prox_step(self, std::move(in), std::move(in_step), out, out_step,
                                           γ, γ_step);
            return std::make_tuple(h_out, std::move(out), std::move(out_step));
        },
        "self"_a, "input"_a, "input_step"_a, "γ"_a = 1, "γ_step"_a = -1,
        "C++ documentation: :cpp:var:`alpaqa::prox_step`\n"
        "Compute a generalized forward-backward step.");
}

template <alpaqa::Config Conf>
void register_prox(py::module_ &m) {
    USING_ALPAQA_CONFIG(Conf);

    using NuclearNorm = alpaqa::functions::NuclearNorm<config_t>;
    py::class_<NuclearNorm>(m, "NuclearNorm",
                            "C++ documentation :cpp:class:`alpaqa::functions::NuclearNorm`")
        .def(py::init<real_t>(), "λ"_a)
        .def(py::init<real_t, length_t, length_t>(), "λ"_a, "rows"_a, "cols"_a)
        .def_readonly("λ", &NuclearNorm::λ)
        .def_readonly("singular_values", &NuclearNorm::singular_values)
        .def_property_readonly("U",
                               [](const NuclearNorm &self) -> mat { return self.svd.matrixU(); })
        .def_property_readonly("V",
                               [](const NuclearNorm &self) -> mat { return self.svd.matrixV(); })
        .def_property_readonly(
            "singular_values_input",
            [](const NuclearNorm &self) -> vec { return self.svd.singularValues(); })
        .def("prox", &NuclearNorm::prox);
    register_prox_func<config_t, NuclearNorm>(m);

    using L1Norm = alpaqa::functions::L1Norm<config_t>;
    py::class_<L1Norm>(m, "L1Norm", "C++ documentation :cpp:class:`alpaqa::functions::L1Norm`")
        .def(py::init<real_t>(), "λ"_a)
        .def_readonly("λ", &L1Norm::λ)
        .def("prox", &L1Norm::prox);
    register_prox_func<config_t, L1Norm>(m);

    using Box = alpaqa::Box<config_t>;
    register_prox_func<config_t, Box>(m);
}

template void register_prox<alpaqa::EigenConfigd>(py::module_ &);
ALPAQA_IF_FLOAT(template void register_prox<alpaqa::EigenConfigf>(py::module_ &);)
ALPAQA_IF_LONGD(template void register_prox<alpaqa::EigenConfigl>(py::module_ &);)
ALPAQA_IF_QUADF(template void register_prox<alpaqa::EigenConfigq>(py::module_ &);)