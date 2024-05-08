/**
 * @file
 * This file defines mappings from Python dicts (kwargs) to simple parameter
 * structs.
 */

#pragma once

#include <alpaqa/params/structs.hpp>
#include <guanaqo/any-ptr.hpp>
#include <guanaqo/demangled-typename.hpp>
#include <guanaqo/string-util.hpp>
#include <functional>
#include <ranges>
#include <stdexcept>
#include <variant>

#include <pybind11/chrono.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;
using namespace py::literals;

struct PythonParam {
    std::string keys;
};

template <class T>
void dict_to_struct_helper(T &, const py::dict &, const PythonParam &s);

template <class T>
py::object struct_to_dict_helper(const T &t);

struct cast_error_with_types : py::cast_error {
    cast_error_with_types(const py::cast_error &e, std::string from, std::string to)
        : py::cast_error(e), from(std::move(from)), to(std::move(to)) {}
    std::string from;
    std::string to;
};

template <class T, class A>
auto set_attr(A T::*attr, T &t, py::handle h, const PythonParam &s) {
    try {
        if (py::isinstance<py::dict>(h))
            dict_to_struct_helper<A>(t.*attr, py::cast<py::dict>(h), s);
        else
            t.*attr = h.cast<A>();
    } catch (const py::cast_error &e) {
        throw cast_error_with_types(e, py::str(py::type::handle_of(h)), py::type_id<A>());
    }
}

namespace alpaqa::params {

/// Function wrapper to set attributes of a struct, type-erasing the type of the
/// attribute.
template <>
struct attribute_accessor<PythonParam> {
    template <class T, class T_actual, class A>
    static attribute_accessor make(A T_actual::*attr, const char *descr = "");
    std::function<void(const guanaqo::any_ptr &, const char *)> def_readwrite;
    std::function<py::object(const guanaqo::any_ptr &)> to_py;
    std::function<void(py::handle, const guanaqo::any_ptr &, const PythonParam &)> from_py;
};

namespace detail {
template <class S>
auto find_param_python(const attribute_table_t<S> &m, std::string_view key, std::string &error_msg)
    -> std::optional<typename attribute_table_t<S>::const_iterator> {
    auto it = m.find(key);
    if (it == m.end()) {
        py::list keys;
        for (const auto &k : std::views::keys(m))
            keys.append(py::str(k));
        auto dl     = py::module::import("difflib");
        auto sorted = dl.attr("get_close_matches")(key, keys, keys.size(), 0.);
        error_msg   = py::cast<std::string>(py::str(", ").attr("join")(sorted));
        return std::nullopt;
    }
    return std::make_optional(it);
}
} // namespace detail
} // namespace alpaqa::params

/// Use @p s to index into the struct type @p T and overwrite the attribute
/// given by @p s.key.
template <class T>
    requires requires { alpaqa::params::attribute_table<T, PythonParam>::table; }
void dict_to_struct_helper(T &t, const py::dict &dict, const PythonParam &s) {
    using namespace alpaqa::params;
    const auto &members = attribute_table<T, PythonParam>::table;
    std::string error_msg;
    for (const auto &[k, v] : dict) {
        if (!py::isinstance<py::str>(k))
            throw std::invalid_argument("Invalid key type in " + s.keys + ", should be str");
        auto ks    = py::cast<std::string>(k);
        auto param = detail::find_param_python(members, ks, error_msg);
        if (!param)
            throw std::invalid_argument("Invalid key '" + ks + "' for type '" +
                                        guanaqo::demangled_typename(typeid(T)) + "' in '" + s.keys +
                                        "',\n  did you mean: " + error_msg);
        PythonParam s_sub{s.keys.empty() ? ks : s.keys + '.' + ks};
        try {
            (*param)->second.from_py(v, &t, s_sub);
        } catch (const cast_error_with_types &e) {
            throw std::runtime_error("Error converting parameter '" + s_sub.keys + "' from " +
                                     e.from + " to '" + e.to + "': " + e.what());
        }
    }
}

template <class T>
    requires(!requires { alpaqa::params::attribute_table<T, PythonParam>::table; })
void dict_to_struct_helper(T &, const py::dict &, const PythonParam &s) {
    throw std::runtime_error("No known conversion from Python dict to C++ type '" +
                             guanaqo::demangled_typename(typeid(T)) + "' in '" + s.keys + '\'');
}

template <class T>
    requires requires { alpaqa::params::attribute_table<T, PythonParam>::table; }
py::object struct_to_dict_helper(const T &t) {
    using namespace alpaqa::params;
    const auto &members = attribute_table<T, PythonParam>::table;
    py::dict d;
    for (auto &&[key, val] : members)
        d[py::str(key)] = val.to_py(&t);
    return d;
}

template <class T>
    requires(!requires { alpaqa::params::attribute_table<T, PythonParam>::table; })
py::object struct_to_dict_helper(const T &t) {
    return py::cast(t);
}

template <class T>
    requires requires { alpaqa::params::attribute_table<T, PythonParam>::table; }
py::dict struct_to_dict(const T &t) {
    return py::cast<py::dict>(struct_to_dict_helper<T>(t));
}

template <class T>
T dict_to_struct(const py::dict &dict) {
    T t;
    dict_to_struct_helper<T>(t, dict, PythonParam{});
    return t;
}

template <class T>
T kwargs_to_struct(const py::kwargs &kwargs) {
    return dict_to_struct<T>(static_cast<const py::dict &>(kwargs));
}

template <class Params>
using params_or_dict = std::variant<Params, py::dict>;

template <class T>
T var_kwargs_to_struct(const params_or_dict<T> &p) {
    return std::holds_alternative<T>(p) ? std::get<T>(p)
                                        : kwargs_to_struct<T>(std::get<py::dict>(p));
}

// TODO: move to separate file
#include <alpaqa/inner/directions/panoc/anderson.hpp>
#include <alpaqa/inner/directions/panoc/convex-newton.hpp>
#include <alpaqa/inner/directions/panoc/lbfgs.hpp>
#include <alpaqa/inner/directions/panoc/structured-lbfgs.hpp>
#include <alpaqa/inner/directions/panoc/structured-newton.hpp>
#include <alpaqa/inner/directions/pantr/newton-tr.hpp>
#include <alpaqa/inner/fista.hpp>
#include <alpaqa/inner/internal/lipschitz.hpp>
#include <alpaqa/inner/internal/panoc-stop-crit.hpp>
#include <alpaqa/inner/panoc.hpp>
#include <alpaqa/inner/pantr.hpp>
#include <alpaqa/inner/zerofpr.hpp>
#include <alpaqa/outer/alm.hpp>
#include <guanaqo/dl-flags.hpp>
#if ALPAQA_WITH_OCP
#include <alpaqa/inner/panoc-ocp.hpp>
#endif
#if ALPAQA_WITH_LBFGSB
#include <alpaqa/lbfgsb/lbfgsb-adapter.hpp>
#endif

namespace alpaqa::params {
#include <alpaqa/params/structs.ipp>
#if ALPAQA_WITH_LBFGSB
#include <alpaqa/lbfgsb/lbfgsb-structs.ipp>
#endif
PARAMS_TABLE_CONF(alpaqa::InnerSolveOptions,                   //
                  PARAMS_MEMBER(always_overwrite_results, ""), //
                  PARAMS_MEMBER(max_time, ""),                 //
                  PARAMS_MEMBER(tolerance, ""),                //
);
} // namespace alpaqa::params

template <class T, class T_actual, class A>
auto alpaqa::params::attribute_accessor<PythonParam>::make(A T_actual::*attr, const char *descr)
    -> attribute_accessor {
    return {
        .def_readwrite =
            [attr, descr](const guanaqo::any_ptr &o, const char *name) {
                auto *obj = o.cast<py::class_<T>>();
                using namespace std::string_view_literals;
                if (name == "global"sv)
                    name = "global_";
                return obj->def_readwrite(name, attr, descr);
            },
        .to_py =
            [attr](const guanaqo::any_ptr &o) {
                auto *obj = o.cast<const T>();
                return struct_to_dict_helper(obj->*attr);
            },
        .from_py =
            [attr](py::handle v, const guanaqo::any_ptr &o, const PythonParam &s) {
                auto *obj = o.cast<T>();
                set_attr(attr, *obj, v, s);
            },
    };
}

template <class T>
void make_dataclass(py::class_<T> &cls) {
    using namespace alpaqa::params;
    cls //
        .def(py::init(&dict_to_struct<T>), "params"_a)
        .def(py::init(&kwargs_to_struct<T>))
        .def("to_dict", &struct_to_dict<T>);
    const auto &members = attribute_table<T, PythonParam>::table;
    for (auto &&[k, v] : members)
        v.def_readwrite(&cls, std::string(k).c_str());
}

template <class T, class... Extra>
auto register_dataclass(py::handle scope, const char *name, const Extra &...extra) {
    py::class_<T> cls(scope, name, extra...);
    make_dataclass(cls);
    return cls;
}
