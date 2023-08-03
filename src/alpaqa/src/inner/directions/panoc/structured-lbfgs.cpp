#include <alpaqa/implementation/inner/directions/panoc/structured-lbfgs.tpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_TEMPLATE(struct, StructuredLBFGSDirection, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, StructuredLBFGSDirection, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, StructuredLBFGSDirection, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, StructuredLBFGSDirection, EigenConfigq);)
// clang-format on

} // namespace alpaqa