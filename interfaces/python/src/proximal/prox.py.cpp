#include <alpaqa/util/quadmath/quadmath.hpp>

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace py::literals;

#include <alpaqa/proximal/indicator-box.hpp>
#include <alpaqa/proximal/norm-l1.hpp>
#include <alpaqa/proximal/nuclear-norm.hpp>
#include <alpaqa/proximal/prox.hpp>

template <alpaqa::Config Conf, class T>
void register_prox_func(py::module_ &m) {
    USING_ALPAQA_CONFIG(Conf);
    m.def(
        "prox",
        [](T &self, crmat in, rmat out, real_t γ) {
            return alpaqa::prox(self, std::move(in), out, γ);
        },
        "self"_a, "input"_a, "output"_a, "γ"_a = 1);
    m.def(
        "prox",
        [](T &self, crmat in, real_t γ) {
            mat out(in.rows(), in.cols());
            auto h_out = alpaqa::prox(self, std::move(in), out, γ);
            return std::make_tuple(h_out, std::move(out));
        },
        "self"_a, "input"_a, "γ"_a);
}

template <alpaqa::Config Conf>
void register_prox(py::module_ &m) {
    USING_ALPAQA_CONFIG(Conf);

    using NuclearNorm = alpaqa::proximal::NuclearNorm<config_t>;
    py::class_<NuclearNorm>(m, "NuclearNorm",
                            "C++ documentation :cpp:class:`alpaqa::proximal::NuclearNorm`")
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

    using L1Norm = alpaqa::proximal::L1Norm<config_t>;
    py::class_<L1Norm>(m, "L1Norm", "C++ documentation :cpp:class:`alpaqa::proximal::L1Norm`")
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