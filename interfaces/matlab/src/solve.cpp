// In separate file, because MATLAB operators clash with Eigen
// (could probably be resolved by removing the using namespace matlab::data).

#include <solver-builder.ipp>
#include <stream.hpp>

#include <alpaqa/casadi/CasADiProblem.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>

namespace alpaqa::mex {

SolverResults minimize(std::span<const double> x0, std::span<const double> y0,
                       std::string_view method, const Options &options,
                       std::function<void(std::string_view)> write_utf8) {
    // TODO
    alpaqa::CasADiProblem<config_t> problem{
        "/home/pieter/GitHub/alpaqa/.venv/cache/alpaqa/cache/"
        "bcd8a11e-7e4b-11ee-9892-0f8255cce6e0/lib/alpaqa_problem.so"};
    alpaqa::TypeErasedProblem<config_t> te_problem(&problem);

    // Map inputs to Eigen vectors
    cmvec x0m{x0.data(), static_cast<length_t>(x0.size())};
    cmvec y0m{y0.data(), static_cast<length_t>(y0.size())};

    streambuf buf{std::move(write_utf8)};
    std::ostream os{&buf};
    auto [builder, direction] = get_solver_builder(method, options);
    auto solver               = builder(direction, options);
    return solver(te_problem, x0m, y0m, os);
}

} // namespace alpaqa::mex