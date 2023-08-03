#include <alpaqa/config/config.hpp>

#include <alpaqa/implementation/accelerators/lbfgs.tpp>

namespace alpaqa {

ALPAQA_EXPORT_TEMPLATE(struct, CBFGSParams, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, CBFGSParams, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, CBFGSParams, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, CBFGSParams, EigenConfigq);)

ALPAQA_EXPORT_TEMPLATE(struct, LBFGSParams, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(struct, LBFGSParams, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(struct, LBFGSParams, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(struct, LBFGSParams, EigenConfigq);)

ALPAQA_EXPORT_TEMPLATE(class, LBFGS, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(class, LBFGS, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(class, LBFGS, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(class, LBFGS, EigenConfigq);)

} // namespace alpaqa