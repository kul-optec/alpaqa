#include <alpaqa/implementation/lbfgsb-adapter.tpp>

namespace alpaqa::lbfgspp {

// clang-format off
ALPAQA_LBFGSPP_EXPORT_TEMPLATE(struct, LBFGSBStats, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_LBFGSPP_EXPORT_TEMPLATE(struct, LBFGSBStats, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_LBFGSPP_EXPORT_TEMPLATE(struct, LBFGSBStats, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_LBFGSPP_EXPORT_TEMPLATE(struct, LBFGSBStats, EigenConfigq);)

ALPAQA_LBFGSPP_EXPORT_TEMPLATE(class, LBFGSBSolver, EigenConfigd);
ALPAQA_IF_FLOAT(ALPAQA_LBFGSPP_EXPORT_TEMPLATE(class, LBFGSBSolver, EigenConfigf);)
ALPAQA_IF_LONGD(ALPAQA_LBFGSPP_EXPORT_TEMPLATE(class, LBFGSBSolver, EigenConfigl);)
ALPAQA_IF_QUADF(ALPAQA_LBFGSPP_EXPORT_TEMPLATE(class, LBFGSBSolver, EigenConfigq);)
// clang-format on

} // namespace alpaqa::lbfgspp