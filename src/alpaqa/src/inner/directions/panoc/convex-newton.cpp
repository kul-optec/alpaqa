#include <alpaqa/inner/directions/panoc/convex-newton.hpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_TEMPLATE(struct, ConvexNewtonDirection, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, ConvexNewtonDirection, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, ConvexNewtonDirection, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, ConvexNewtonDirection, EigenConfigq);)
// clang-format on

} // namespace alpaqa
