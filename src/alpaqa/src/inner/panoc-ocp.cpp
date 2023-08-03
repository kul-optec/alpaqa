#include <alpaqa/implementation/inner/panoc-ocp.tpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_TEMPLATE(struct, PANOCOCPProgressInfo, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, PANOCOCPProgressInfo, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, PANOCOCPProgressInfo, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, PANOCOCPProgressInfo, EigenConfigq);)

ALPAQA_EXPORT_TEMPLATE(class, PANOCOCPSolver, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(class, PANOCOCPSolver, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(class, PANOCOCPSolver, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(class, PANOCOCPSolver, EigenConfigq);)
// clang-format on

} // namespace alpaqa