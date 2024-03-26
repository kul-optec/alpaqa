#include <alpaqa/casadi-loader-export.h>
#include <alpaqa/casadi/CasADiProblem.hpp>
#include <alpaqa/casadi/casadi-namespace.hpp>

namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE::casadi_loader {

/// Convert the map of strings to a map of CasADi functions.
CasADiFunctions CASADI_LOADER_EXPORT
deserialize_problem(const SerializedCasADiFunctions &functions);

/// Complete the given problem that contains only functions f and g, adding the
/// necessary functions and gradients so that it can be used with
/// @ref CasADiProblem.
void CASADI_LOADER_EXPORT complete_problem(CasADiFunctions &functions);

} // namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE::casadi_loader
