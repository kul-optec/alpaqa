#include <solver-builder.ipp>

#include <alpaqa/casadi/CasADiProblem.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>

#include <sstream>

namespace alpaqa::mex {

SolverResults minimize(crvec x0, crvec y0, std::string_view method,
                       const Options &options) {
    // TODO
    alpaqa::CasADiProblem<config_t> problem{
        "/home/pieter/GitHub/alpaqa/.venv/cache/alpaqa/cache/"
        "bcd8a11e-7e4b-11ee-9892-0f8255cce6e0/lib/alpaqa_problem.so"};
    alpaqa::TypeErasedProblem<config_t> te_problem(&problem);

    std::ostringstream os;
    auto [builder, direction] = get_solver_builder(method, options);
    auto solver               = builder(direction, options);
    return solver(te_problem, x0, y0, os);
}

} // namespace alpaqa::mex