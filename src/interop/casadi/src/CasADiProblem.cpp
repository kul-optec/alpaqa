#include <alpaqa/config/config.hpp>
#include <alpaqa/implementation/casadi/CasADiProblem.tpp>

namespace alpaqa {
CasADiFunctions::~CasADiFunctions() = default;
CASADI_LOADER_EXPORT_TEMPLATE(class, CasADiProblem, EigenConfigd);
} // namespace alpaqa