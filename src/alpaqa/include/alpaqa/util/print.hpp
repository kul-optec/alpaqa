#pragma once

#include <alpaqa/export.hpp>
#include <alpaqa/util/float.hpp>

#include <iosfwd>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

#include <Eigen/Core>

namespace alpaqa {

template <std::floating_point F>
ALPAQA_EXPORT std::string
float_to_str(F value, int precision = std::numeric_limits<F>::max_digits10);

namespace detail {

template <class T>
ALPAQA_EXPORT std::ostream &
print_csv_impl(std::ostream &os, const T &M, std::string_view sep = ",",
               std::string_view begin = "", std::string_view end = "\n");

template <class T>
ALPAQA_EXPORT std::ostream &print_matlab_impl(std::ostream &os, const T &M,
                                              std::string_view end = ";\n");

template <class T>
ALPAQA_EXPORT std::ostream &print_python_impl(std::ostream &os, const T &M,
                                              std::string_view end = "\n");

#define ALPAQA_PRINT_OVL_IMPL(name, type)                                      \
    template <class... Args>                                                   \
    std::ostream &print_##name##_helper(                                       \
        std::ostream &os, const Eigen::Ref<const Eigen::MatrixX<type>> &M,     \
        Args &&...args) {                                                      \
        return print_##name##_impl(os, M, std::forward<Args>(args)...);        \
    }
#define ALPAQA_PRINT_OVL(type)                                                 \
    ALPAQA_PRINT_OVL_IMPL(csv, type)                                           \
    ALPAQA_PRINT_OVL_IMPL(matlab, type)                                        \
    ALPAQA_PRINT_OVL_IMPL(python, type)
ALPAQA_PRINT_OVL(int)
ALPAQA_PRINT_OVL(long)
ALPAQA_PRINT_OVL(long long)
ALPAQA_PRINT_OVL(double)
ALPAQA_PRINT_OVL(long double)
#ifdef ALPAQA_WITH_QUAD_PRECISION
ALPAQA_PRINT_OVL(__float128)
#endif

} // namespace detail

#define ALPAQA_PRINT_CVT(name)                                                 \
    template <class Derived, class... Args>                                    \
    std::ostream &print_##name(std::ostream &os,                               \
                               const Eigen::DenseBase<Derived> &M,             \
                               Args &&...args) {                               \
        using R = Eigen::Ref<const Eigen::MatrixX<typename Derived::Scalar>>;  \
        return detail::print_##name##_helper(os, static_cast<const R &>(M),    \
                                             std::forward<Args>(args)...);     \
    }

ALPAQA_PRINT_CVT(csv)
ALPAQA_PRINT_CVT(matlab)
ALPAQA_PRINT_CVT(python)

#undef ALPAQA_PRINT_CVT
#undef ALPAQA_PRINT_OVL
#undef ALPAQA_PRINT_OVL_IMPL

} // namespace alpaqa