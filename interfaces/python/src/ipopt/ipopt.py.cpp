
#include <alpaqa/ipopt/ipopt-adapter.hpp>
#include <alpaqa/ipopt/ipopt-enums.hpp>
#include <IpIpoptApplication.hpp>
#include <IpSolveStatistics.hpp>

#include <stdexcept>
#include <string>

#include <pybind11/eigen/matrix.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;
using namespace py::literals;

namespace {

std::string possible_keys(const auto &tbl) {
    if (tbl.empty())
        return std::string{};
    auto penult       = std::prev(tbl.end());
    auto quote_concat = [](std::string &&a, auto b) { return a + "'" + b.first + "', "; };
    return std::accumulate(tbl.begin(), penult, std::string{}, quote_concat) + "'" +
           std::string(penult->first) + "'";
}

void set_params(Ipopt::IpoptApplication &app, const py::dict &opts) {
    // Search the option name in the list of Ipopt options
    const auto &ipopt_opts = app.RegOptions()->RegisteredOptionsList();

    for (auto &&[py_key, py_value] : opts) {
        auto opt_name        = py::cast<std::string>(py_key);
        const auto regops_it = ipopt_opts.find(opt_name);
        if (regops_it == ipopt_opts.end())
            throw std::invalid_argument("Invalid key '" + opt_name + "' for type '" +
                                        "IpoptApplication"
                                        "',\n  possible keys are: " +
                                        possible_keys(ipopt_opts));
        // Depending on the type, set the value of the option
        bool success    = false;
        const auto type = regops_it->second->Type();
        switch (type) {
            case Ipopt::OT_Number: {
                try {
                    auto value = py::cast<Ipopt::Number>(py_value);
                    success    = app.Options()->SetNumericValue(opt_name, value, false);
                } catch (const py::cast_error &) {
                    throw std::invalid_argument("Cannot cast value '" + opt_name + "' to double");
                }
            } break;
            case Ipopt::OT_Integer: {
                try {
                    auto value = py::cast<Ipopt::Index>(py_value);
                    success    = app.Options()->SetIntegerValue(opt_name, value, false);
                } catch (const py::cast_error &) {
                    throw std::invalid_argument("Cannot cast value '" + opt_name + "' to integer");
                }
            } break;
            case Ipopt::OT_String: {
                try {
                    auto value = py::cast<std::string>(py_value);
                    success    = app.Options()->SetStringValue(opt_name, value, false);
                } catch (const py::cast_error &) {
                    throw std::invalid_argument("Cannot cast value '" + opt_name + "' to string");
                }
            } break;
            case Ipopt::OT_Unknown:
            default: {
                throw std::invalid_argument("Unknown type for '" + opt_name + "'");
            }
        }
        if (!success)
            throw std::invalid_argument("Invalid option '" + opt_name + "'");
    }
}

auto make_ipopt_solver(const py::dict &opts) {
    using namespace Ipopt;

    // We are using the factory, since this allows us to compile this
    // example with an Ipopt Windows DLL
    SmartPtr<IpoptApplication> app = IpoptApplicationFactory();
    app->RethrowNonIpoptException(true);

    app->Options()->SetNumericValue("tol", 1e-8);
    app->Options()->SetNumericValue("constr_viol_tol", 1e-8);
    app->Options()->SetStringValue("linear_solver", "mumps");
    // app->Options()->SetStringValue("print_timing_statistics", "yes");
    // app->Options()->SetStringValue("timing_statistics", "yes");
    app->Options()->SetStringValue("hessian_approximation", "exact");

    set_params(*app, opts);

    // Initialize the IpoptApplication and process the options
    ApplicationReturnStatus status = app->Initialize();
    if (status != Solve_Succeeded)
        throw std::runtime_error("Error during Ipopt initialization: " +
                                 std::string(enum_name(status)));

    return app;
}

py::dict convert_ipopt_statistics_to_dict(const Ipopt::SolveStatistics *stats) {
    py::dict statistics_dict;

    statistics_dict["iteration_count"]      = stats->IterationCount();
    statistics_dict["total_cpu_time"]       = stats->TotalCpuTime();
    statistics_dict["total_sys_time"]       = stats->TotalSysTime();
    statistics_dict["total_wallclock_time"] = stats->TotalWallclockTime();

    Ipopt::Index num_obj_evals, num_constr_evals, num_obj_grad_evals, num_constr_jac_evals,
        num_hess_evals;
    stats->NumberOfEvaluations(num_obj_evals, num_constr_evals, num_obj_grad_evals,
                               num_constr_jac_evals, num_hess_evals);

    statistics_dict["num_obj_evals"]        = num_obj_evals;
    statistics_dict["num_constr_evals"]     = num_constr_evals;
    statistics_dict["num_obj_grad_evals"]   = num_obj_grad_evals;
    statistics_dict["num_constr_jac_evals"] = num_constr_jac_evals;
    statistics_dict["num_hess_evals"]       = num_hess_evals;

    Ipopt::Number dual_inf, constr_viol, varbounds_viol, complementarity, kkt_error;
    stats->Infeasibilities(dual_inf, constr_viol, varbounds_viol, complementarity, kkt_error);

    statistics_dict["dual_infeasibility"]        = dual_inf;
    statistics_dict["constraint_violation"]      = constr_viol;
    statistics_dict["variable_bounds_violation"] = varbounds_viol;
    statistics_dict["complementarity"]           = complementarity;
    statistics_dict["kkt_error"]                 = kkt_error;

    statistics_dict["final_objective"]        = stats->FinalObjective();
    statistics_dict["final_scaled_objective"] = stats->FinalScaledObjective();

    return statistics_dict;
}

USING_ALPAQA_CONFIG(alpaqa::EigenConfigd);

struct IpoptSolverPython {
    using Problem = alpaqa::TypeErasedProblem<config_t>;
    py::dict options;
    py::dict solve(const Problem &problem, rvec x, rvec y, rvec w) const {
        auto solver = make_ipopt_solver(options);

        // Ipopt problem adapter
        using Problem                    = alpaqa::IpoptAdapter;
        Ipopt::SmartPtr<Ipopt::TNLP> nlp = new Problem(problem);
        auto *my_nlp                     = dynamic_cast<Problem *>(GetRawPtr(nlp));

        // Dimensions
        length_t n = problem.get_num_variables(), m = problem.get_num_constraints();

        // Initial guess
        my_nlp->initial_guess                    = x;
        my_nlp->initial_guess_multipliers        = y;
        my_nlp->initial_guess_bounds_multipliers = w;

        // Solve the problem
        auto app_status = solver->OptimizeTNLP(nlp);

        // Results
        auto &nlp_res = my_nlp->results;
        if (nlp_res.status == Ipopt::SolverReturn::UNASSIGNED) {
            nlp_res.solution_x   = vec::Constant(n, alpaqa::NaN<config_t>);
            nlp_res.solution_y   = vec::Constant(m, alpaqa::NaN<config_t>);
            nlp_res.solution_z_L = vec::Constant(n, alpaqa::NaN<config_t>);
            nlp_res.solution_z_U = vec::Constant(n, alpaqa::NaN<config_t>);
        } else {
            x = nlp_res.solution_x;
            y = nlp_res.solution_y;
            if (w.size() > 0)
                w = nlp_res.combine_bounds_multipliers();
        }
        py::dict stats{"status"_a     = py::cast(nlp_res.status),
                       "app_status"_a = py::cast(app_status)};
        if (auto ipstats = solver->Statistics(); IsValid(ipstats))
            stats.attr("update")(convert_ipopt_statistics_to_dict(GetRawPtr(ipstats)));
        return stats;
    }
    [[nodiscard]] auto call(const IpoptSolverPython::Problem &p, std::optional<vec> x,
                            std::optional<vec> y, std::optional<vec> w) const {
        alpaqa::util::check_dim_msg<vec>(
            x, p.get_num_variables(),
            "Length of x does not match problem size problem.num_variables");
        alpaqa::util::check_dim_msg<vec>(
            y, p.get_num_constraints(),
            "Length of y does not match problem size problem.num_constraints");
        if (w)
            alpaqa::util::check_dim_msg<vec>(
                *w, p.get_num_variables(),
                "Length of w does not match problem size problem.num_variables");
        auto stats = solve(p, *x, *y, w ? rvec{*w} : alpaqa::null_vec<config_t>);
        return w ? py::make_tuple(std::move(*x), std::move(*y), std::move(*w), std::move(stats))
                 : py::make_tuple(std::move(*x), std::move(*y), std::move(stats));
    }
};

} // namespace

void register_ipopt(py::module_ &m) {
    py::enum_<Ipopt::SolverReturn>(m, "IpoptSolverReturn", "Ipopt::SolverReturn enum")
        .value("SUCCESS", Ipopt::SolverReturn::SUCCESS)
        .value("MAXITER_EXCEEDED", Ipopt::SolverReturn::MAXITER_EXCEEDED)
        .value("CPUTIME_EXCEEDED", Ipopt::SolverReturn::CPUTIME_EXCEEDED)
        .value("WALLTIME_EXCEEDED", Ipopt::SolverReturn::WALLTIME_EXCEEDED)
        .value("STOP_AT_TINY_STEP", Ipopt::SolverReturn::STOP_AT_TINY_STEP)
        .value("STOP_AT_ACCEPTABLE_POINT", Ipopt::SolverReturn::STOP_AT_ACCEPTABLE_POINT)
        .value("LOCAL_INFEASIBILITY", Ipopt::SolverReturn::LOCAL_INFEASIBILITY)
        .value("USER_REQUESTED_STOP", Ipopt::SolverReturn::USER_REQUESTED_STOP)
        .value("FEASIBLE_POINT_FOUND", Ipopt::SolverReturn::FEASIBLE_POINT_FOUND)
        .value("DIVERGING_ITERATES", Ipopt::SolverReturn::DIVERGING_ITERATES)
        .value("RESTORATION_FAILURE", Ipopt::SolverReturn::RESTORATION_FAILURE)
        .value("ERROR_IN_STEP_COMPUTATION", Ipopt::SolverReturn::ERROR_IN_STEP_COMPUTATION)
        .value("INVALID_NUMBER_DETECTED", Ipopt::SolverReturn::INVALID_NUMBER_DETECTED)
        .value("TOO_FEW_DEGREES_OF_FREEDOM", Ipopt::SolverReturn::TOO_FEW_DEGREES_OF_FREEDOM)
        .value("INVALID_OPTION", Ipopt::SolverReturn::INVALID_OPTION)
        .value("OUT_OF_MEMORY", Ipopt::SolverReturn::OUT_OF_MEMORY)
        .value("INTERNAL_ERROR", Ipopt::SolverReturn::INTERNAL_ERROR)
        .value("UNASSIGNED", Ipopt::SolverReturn::UNASSIGNED);

    py::enum_<Ipopt::ApplicationReturnStatus>(m, "IpoptApplicationReturnStatus",
                                              "Ipopt::ApplicationReturnStatus enum")
        .value("Solve_Succeeded", Ipopt::ApplicationReturnStatus::Solve_Succeeded)
        .value("Solved_To_Acceptable_Level",
               Ipopt::ApplicationReturnStatus::Solved_To_Acceptable_Level)
        .value("Infeasible_Problem_Detected",
               Ipopt::ApplicationReturnStatus::Infeasible_Problem_Detected)
        .value("Search_Direction_Becomes_Too_Small",
               Ipopt::ApplicationReturnStatus::Search_Direction_Becomes_Too_Small)
        .value("Diverging_Iterates", Ipopt::ApplicationReturnStatus::Diverging_Iterates)
        .value("User_Requested_Stop", Ipopt::ApplicationReturnStatus::User_Requested_Stop)
        .value("Feasible_Point_Found", Ipopt::ApplicationReturnStatus::Feasible_Point_Found)
        .value("Maximum_Iterations_Exceeded",
               Ipopt::ApplicationReturnStatus::Maximum_Iterations_Exceeded)
        .value("Restoration_Failed", Ipopt::ApplicationReturnStatus::Restoration_Failed)
        .value("Error_In_Step_Computation",
               Ipopt::ApplicationReturnStatus::Error_In_Step_Computation)
        .value("Maximum_CpuTime_Exceeded", Ipopt::ApplicationReturnStatus::Maximum_CpuTime_Exceeded)
        .value("Maximum_WallTime_Exceeded",
               Ipopt::ApplicationReturnStatus::Maximum_WallTime_Exceeded)
        .value("Not_Enough_Degrees_Of_Freedom",
               Ipopt::ApplicationReturnStatus::Not_Enough_Degrees_Of_Freedom)
        .value("Invalid_Problem_Definition",
               Ipopt::ApplicationReturnStatus::Invalid_Problem_Definition)
        .value("Invalid_Option", Ipopt::ApplicationReturnStatus::Invalid_Option)
        .value("Invalid_Number_Detected", Ipopt::ApplicationReturnStatus::Invalid_Number_Detected)
        .value("Unrecoverable_Exception", Ipopt::ApplicationReturnStatus::Unrecoverable_Exception)
        .value("NonIpopt_Exception_Thrown",
               Ipopt::ApplicationReturnStatus::NonIpopt_Exception_Thrown)
        .value("Insufficient_Memory", Ipopt::ApplicationReturnStatus::Insufficient_Memory)
        .value("Internal_Error", Ipopt::ApplicationReturnStatus::Internal_Error);

    py::class_<IpoptSolverPython>(m, "IpoptSolver", "Ipopt binding for testing purposes")
        .def(py::init<py::dict>(), "options"_a = py::dict())
        .def("__call__", &IpoptSolverPython::call, "problem"_a, "x"_a = std::nullopt,
             "y"_a = std::nullopt, "w"_a = std::nullopt);
}
