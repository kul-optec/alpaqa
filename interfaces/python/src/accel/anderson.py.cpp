#include <alpaqa/util/quadmath/quadmath.hpp>

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace py::literals;

#include <alpaqa/accelerators/anderson.hpp>
#include <alpaqa/util/check-dim.hpp>

#include <dict/stats-to-dict.hpp>
#include <params/params.hpp>

template <alpaqa::Config Conf>
void register_anderson(py::module_ &m) {
    USING_ALPAQA_CONFIG(Conf);

    using Anderson = alpaqa::AndersonAccel<config_t>;
    py::class_<Anderson> anderson(m, "AndersonAccel",
                                  "C++ documentation :cpp:class:`alpaqa::AndersonAccel`");
    using AndersonParams = typename Anderson::Params;
    register_dataclass<AndersonParams>(
        anderson, "Params", "C++ documentation :cpp:class:`alpaqa::AndersonAccelParams`");

    anderson //
        .def(py::init([](params_or_dict<AndersonParams> params) {
                 return Anderson{var_kwargs_to_struct(params)};
             }),
             "params"_a)
        .def(py::init([](params_or_dict<AndersonParams> params, length_t n) {
                 return Anderson{var_kwargs_to_struct(params), n};
             }),
             "params"_a, "n"_a)
        .def_property_readonly("params", &Anderson::get_params)
        .def_property_readonly("n", &Anderson::n)
        .def("__str__", &Anderson::get_name)
        .def("resize", &Anderson::resize, "n"_a)
        .def("initialize", &Anderson::initialize, "g_0"_a, "r_0"_a)
        .def(
            "compute",
            [](Anderson &self, crvec gₖ, vec rₖ, rvec xₖ) { self.compute(gₖ, std::move(rₖ), xₖ); },
            "g_k"_a, "r_k"_a, "x_k_aa"_a)
        .def(
            "compute",
            [](Anderson &self, crvec gₖ, vec rₖ) {
                vec x(self.n());
                self.compute(gₖ, std::move(rₖ), x);
                return x;
            },
            "g_k"_a, "r_k"_a)
        .def("reset", &Anderson::reset)
        .def_property_readonly("history", &Anderson::history)
        .def_property_readonly("current_history", &Anderson::current_history)
        .def_property_readonly("Q", [](const Anderson &self) { return self.get_QR().get_Q(); })
        .def_property_readonly("R", [](const Anderson &self) { return self.get_QR().get_R(); });
}

template void register_anderson<alpaqa::EigenConfigd>(py::module_ &);
ALPAQA_IF_FLOAT(template void register_anderson<alpaqa::EigenConfigf>(py::module_ &);)
ALPAQA_IF_LONGD(template void register_anderson<alpaqa::EigenConfigl>(py::module_ &);)
ALPAQA_IF_QUADF(template void register_anderson<alpaqa::EigenConfigq>(py::module_ &);)
