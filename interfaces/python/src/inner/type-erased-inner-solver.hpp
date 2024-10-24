#pragma once

#include <alpaqa/inner/inner-solve-options.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>
#include <optional>
#include <type_traits>

#include <pybind11/pybind11.h>
namespace py = pybind11;
using namespace py::literals;

#include <inner/type-erased-solver-stats.hpp>

namespace alpaqa {

using guanaqo::required_function_t;

template <Config Conf, class ProblemT>
struct InnerSolverVTable : guanaqo::BasicVTable {
    USING_ALPAQA_CONFIG(Conf);
    using Stats        = TypeErasedInnerSolverStats<Conf>;
    using Problem      = ProblemT;
    using SolveOptions = InnerSolveOptions<config_t>;

    // clang-format off
    required_function_t<Stats(const Problem &, const SolveOptions &, rvec, rvec, crvec, rvec)>
        call = nullptr;
    required_function_t<void()>
        stop = nullptr;
    required_function_t<std::string() const>
        get_name = nullptr;
    required_function_t<py::object() const>
        get_params = nullptr;
    // clang-format on

    template <class T>
    InnerSolverVTable(std::in_place_t, T &t) : guanaqo::BasicVTable{std::in_place, t} {
        stop       = guanaqo::type_erased_wrapped<T, &T::stop>();
        get_name   = guanaqo::type_erased_wrapped<T, &T::get_name>();
        get_params = [](const void *self_) -> py::object {
            auto &self    = *std::launder(reinterpret_cast<const T *>(self_));
            auto &&params = self.get_params();
            if constexpr (std::is_convertible_v<decltype(params), py::object>)
                return std::move(params);
            else
                return py::cast(std::move(params));
        };
        call = []<class... Args>(void *self_, const Problem &p, Args... args) {
            auto &self = *std::launder(reinterpret_cast<T *>(self_));
            return Stats{self(p, std::forward<Args>(args)...)};
        };
    }
    InnerSolverVTable() = default;
};

template <Config Conf, class ProblemT, class Allocator = std::allocator<std::byte>>
class TypeErasedInnerSolver
    : public guanaqo::TypeErased<InnerSolverVTable<Conf, ProblemT>, Allocator> {
  public:
    USING_ALPAQA_CONFIG(Conf);
    using VTable         = InnerSolverVTable<Conf, ProblemT>;
    using allocator_type = Allocator;
    using TypeErased     = guanaqo::TypeErased<VTable, allocator_type>;
    using Stats          = typename VTable::Stats;
    using Problem        = typename VTable::Problem;
    using TypeErased::TypeErased;

  protected:
    using TypeErased::call;
    using TypeErased::self;
    using TypeErased::vtable;

  public:
    template <class T, class... Args>
    static TypeErasedInnerSolver make(Args &&...args) {
        return TypeErased::template make<TypeErasedInnerSolver, T>(std::forward<Args>(args)...);
    }

    template <class... Args>
    decltype(auto) operator()(Args &&...args) {
        return call(vtable.call, std::forward<Args>(args)...);
    }
    decltype(auto) stop() { return call(vtable.stop); }
    decltype(auto) get_name() const { return call(vtable.get_name); }
    decltype(auto) get_params() const { return call(vtable.get_params); }
};

} // namespace alpaqa

template <class InnerSolverT>
struct InnerSolverConversion {
    using InnerSolver = InnerSolverT;
    std::optional<py::class_<InnerSolver>> cls;
    void initialize(py::class_<InnerSolver> &&cls) {
        assert(!this->cls);
        this->cls.emplace(std::move(cls));
    }
    template <class T>
    void implicitly_convertible_to() {
        assert(this->cls);
        cls->def(py::init([](const T &t) { return std::make_unique<InnerSolver>(t); }),
                 "inner_solver"_a, "Explicit conversion.");
        py::implicitly_convertible<T, InnerSolver>();
    }
};

/// Global instance of the py::class_<InnerSolverT> binding, for registering
/// converting constructors from concrete inner solvers later.
template <class InnerSolverT>
inline InnerSolverConversion<InnerSolverT> inner_solver_class;
