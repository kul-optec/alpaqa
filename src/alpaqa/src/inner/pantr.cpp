#include <alpaqa/implementation/inner/pantr.tpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_TEMPLATE(struct, PANTRParams, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, PANTRParams, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, PANTRParams, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, PANTRParams, EigenConfigq);)

ALPAQA_EXPORT_TEMPLATE(struct, PANTRStats, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, PANTRStats, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, PANTRStats, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, PANTRStats, EigenConfigq);)

ALPAQA_EXPORT_TEMPLATE(struct, PANTRProgressInfo, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, PANTRProgressInfo, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, PANTRProgressInfo, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, PANTRProgressInfo, EigenConfigq);)
// clang-format on

} // namespace alpaqa