#pragma once

#include <Eigen/Core>
#include <guanaqo/print.hpp>

namespace alpaqa {

using guanaqo::float_to_str;
using guanaqo::float_to_str_vw;

template <class Derived>
std::ostream &print_python(std::ostream &os,
                           const Eigen::DenseBase<Derived> &M) {
    if constexpr (requires { M.derived().data(); })
        return guanaqo::detail::print_python_impl(
            os, guanaqo::MatrixView<const typename Derived::Scalar>{{
                    .data         = M.derived().data(),
                    .rows         = M.rows(),
                    .cols         = M.cols(),
                    .outer_stride = M.outerStride(),
                }});
    else
        return print_python(os, M.eval());
}

} // namespace alpaqa