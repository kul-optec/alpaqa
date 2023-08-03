#include <alpaqa/implementation/inner/panoc-helpers.tpp>

namespace alpaqa::detail {

ALPAQA_EXPORT_TEMPLATE(struct, PANOCHelpers, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, PANOCHelpers, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, PANOCHelpers, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, PANOCHelpers, EigenConfigq);)

} // namespace alpaqa::detail