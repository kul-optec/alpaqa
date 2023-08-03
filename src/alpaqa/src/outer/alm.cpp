#include <alpaqa/outer/alm.hpp>

namespace alpaqa {

ALPAQA_EXPORT_TEMPLATE(struct, ALMParams, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, ALMParams, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, ALMParams, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, ALMParams, EigenConfigq);)

} // namespace alpaqa