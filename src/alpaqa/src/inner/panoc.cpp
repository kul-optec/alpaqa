#include <alpaqa/implementation/inner/panoc.tpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_TEMPLATE(struct, PANOCParams, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, PANOCParams, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, PANOCParams, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, PANOCParams, EigenConfigq);)

ALPAQA_EXPORT_TEMPLATE(struct, PANOCStats, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, PANOCStats, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, PANOCStats, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, PANOCStats, EigenConfigq);)

ALPAQA_EXPORT_TEMPLATE(struct, PANOCProgressInfo, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, PANOCProgressInfo, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, PANOCProgressInfo, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, PANOCProgressInfo, EigenConfigq);)
// clang-format on

} // namespace alpaqa