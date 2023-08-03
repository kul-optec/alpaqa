#include <alpaqa/implementation/outer/internal/alm-helpers.tpp>

namespace alpaqa::detail {

ALPAQA_EXPORT_TEMPLATE(struct, ALMHelpers, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, ALMHelpers, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, ALMHelpers, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, ALMHelpers, EigenConfigq);)

} // namespace alpaqa::detail