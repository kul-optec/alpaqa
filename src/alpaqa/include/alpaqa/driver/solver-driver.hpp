#pragma once

#include <alpaqa/params/options.hpp>

#include <alpaqa/driver/results.hpp>
#include <functional>
#include <memory>

namespace alpaqa::driver {

using solver_free_func_t = SolverResults(alpaqa::LoadedProblem &,
                                         std::ostream &);
using solver_func_t      = std::function<solver_free_func_t>;

struct SolverWrapper {
    virtual ~SolverWrapper() = default;
    [[nodiscard]] virtual bool has_statistics() const { return false; }
    virtual void write_statistics_to_stream(std::ostream &) const {}
    SolverWrapper(solver_func_t run) : run(std::move(run)) {}
    solver_func_t run;
};

using SharedSolverWrapper = std::shared_ptr<SolverWrapper>;

using solver_builder_func =
    std::function<SharedSolverWrapper(std::string_view, alpaqa::Options &)>;

} // namespace alpaqa::driver
