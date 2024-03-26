#include <alpaqa/casadi/casadi-namespace.hpp>
#include <alpaqa/config/config.hpp>
#include <alpaqa/implementation/casadi/CasADiProblem.tpp>

namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE {
CasADiFunctions::~CasADiFunctions() = default;
CASADI_LOADER_EXPORT_TEMPLATE(class, CasADiProblem, EigenConfigd);
} // namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE
