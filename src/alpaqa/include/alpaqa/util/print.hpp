#pragma once

#include <Eigen/Core>
#include <guanaqo/eigen/view.hpp>
#include <guanaqo/print.hpp>

namespace alpaqa {

using guanaqo::float_to_str;
using guanaqo::float_to_str_vw;

template <class Derived>
std::ostream &print_python(std::ostream &os,
                           const Eigen::DenseBase<Derived> &M) {
    if constexpr (requires { M.derived().data(); })
        return guanaqo::print_python(os, guanaqo::as_view(M));
    else
        return print_python(os, M.eval());
}

} // namespace alpaqa