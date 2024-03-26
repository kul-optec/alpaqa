#include <alpaqa/casadi/CasADiFunctionWrapper.hpp>
#include <alpaqa/casadi/CompleteCasADiProblem.hpp>
#include <alpaqa/casadi/casadi-namespace.hpp>
#include <alpaqa/config/config.hpp>
#include <alpaqa/implementation/casadi/CasADiLoader-util.hpp>
#include <casadi/casadi.hpp>

namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE::casadi_loader {

template <class F>
auto wrap(const char *name, F f) {
    try {
        return f();
    } catch (const casadi_loader::invalid_argument_dimensions &e) {
        throw std::invalid_argument(
            "Unable to load function '" + std::string(name) +
            "': " + demangled_typename(typeid(e)) + ": " + e.what());
    }
}

CasADiFunctions
deserialize_problem(const SerializedCasADiFunctions &functions) {
    CasADiFunctions result;
    for (auto &&[k, v] : functions.functions)
        result.functions.emplace(k, casadi::Function::deserialize(v));
    return result;
}

void complete_problem(CasADiFunctions &functions) {
    USING_ALPAQA_CONFIG(CasADiProblem<>::config_t);

    // Load and validate the objective
    using f_eval  = CasADiFunctionEvaluator<config_t, 2, 1>;
    const auto &f = functions.functions.at("f");
    wrap("f", [&] { f_eval::validate_num_args(f); });
    auto n = f.size1_in(0);
    auto p = f.size1_in(1);
    wrap("f", [&] { f_eval::validate_dimensions(f, dims(n, p), dims(1)); });

    // Add the gradient of the objective
    auto x      = casadi::SX::sym("x", n);
    auto param  = casadi::SX::sym("param", p);
    auto fx     = f(std::vector{x, param})[0];
    auto grad_f = casadi::SX::gradient(fx, x);
    functions.functions["f_grad_f"] =
        casadi::Function("f_grad_f", {x, param}, {fx, grad_f});

    auto complete_constraints = [&](const casadi::Function &g) {
        // Validate the constraints
        using g_eval = CasADiFunctionEvaluator<config_t, 2, 1>;
        wrap("g", [&] { g_eval::validate_num_args(g); });
        auto m = g.size1_out(0);
        wrap("g", [&] { g_eval::validate_dimensions(g, dims(n, p), dims(m)); });
        if (m == 0) // unconstrained case
            return;

        // Add the gradient-vector product of the constraints
        auto y           = casadi::SX::sym("y", m);
        auto gx          = g(std::vector{x, param})[0];
        auto grad_g_prod = casadi::SX::jtimes(gx, x, y, true);
        functions.functions["grad_g_prod"] =
            casadi::Function("grad_g_prod", {x, param, y}, {grad_g_prod});

        // Add the gradient of the Lagrangian
        auto grad_L = grad_f + grad_g_prod;
        functions.functions["grad_L"] =
            casadi::Function("grad_L", {x, param, y}, {grad_L});
    };
    // Try loading the constrains
    auto g = functions.functions.find("g");
    if (g != functions.functions.end())
        complete_constraints(g->second);
}

} // namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE::casadi_loader
