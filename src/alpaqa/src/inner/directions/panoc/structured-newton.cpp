#include <alpaqa/inner/directions/panoc/structured-newton.hpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_TEMPLATE(struct, StructuredNewtonDirection, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, StructuredNewtonDirection, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, StructuredNewtonDirection, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, StructuredNewtonDirection, EigenConfigq);)
// clang-format on

} // namespace alpaqa
