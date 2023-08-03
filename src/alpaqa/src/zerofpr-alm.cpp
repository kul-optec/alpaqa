#include <alpaqa/implementation/inner/zerofpr.tpp>
#include <alpaqa/implementation/outer/alm.tpp>
#include <alpaqa/zerofpr-alm.hpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_TEMPLATE(class, ZeroFPRSolver, LBFGSDirection<EigenConfigd>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(class, ZeroFPRSolver, LBFGSDirection<EigenConfigf>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(class, ZeroFPRSolver, LBFGSDirection<EigenConfigl>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(class, ZeroFPRSolver, LBFGSDirection<EigenConfigq>);)

ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, ZeroFPRSolver<LBFGSDirection<EigenConfigd>>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, ZeroFPRSolver<LBFGSDirection<EigenConfigf>>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, ZeroFPRSolver<LBFGSDirection<EigenConfigl>>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, ZeroFPRSolver<LBFGSDirection<EigenConfigq>>);)
// clang-format on

} // namespace alpaqa