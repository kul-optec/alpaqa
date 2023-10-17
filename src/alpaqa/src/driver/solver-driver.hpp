#pragma once

#include "options.hpp"
#include "results.hpp"
#include <functional>
#include <memory>

using solver_free_func_t = SolverResults(LoadedProblem &, std::ostream &);
using solver_func_t      = std::function<solver_free_func_t>;

struct SolverWrapper {
    [[nodiscard]] virtual bool has_statistics() const { return false; }
    virtual void write_statistics_to_stream(std::ostream &) {}
    SolverWrapper(solver_func_t run) : run(std::move(run)) {}
    solver_func_t run;
};

using SharedSolverWrapper = std::shared_ptr<SolverWrapper>;

using solver_builder_func =
    std::function<SharedSolverWrapper(std::string_view, Options &)>;
