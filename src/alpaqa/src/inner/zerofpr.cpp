#include <alpaqa/implementation/inner/zerofpr.tpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_TEMPLATE(struct, ZeroFPRParams, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, ZeroFPRParams, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, ZeroFPRParams, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, ZeroFPRParams, EigenConfigq);)

ALPAQA_EXPORT_TEMPLATE(struct, ZeroFPRStats, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, ZeroFPRStats, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, ZeroFPRStats, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, ZeroFPRStats, EigenConfigq);)

ALPAQA_EXPORT_TEMPLATE(struct, ZeroFPRProgressInfo, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, ZeroFPRProgressInfo, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, ZeroFPRProgressInfo, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, ZeroFPRProgressInfo, EigenConfigq);)
// clang-format on

} // namespace alpaqa