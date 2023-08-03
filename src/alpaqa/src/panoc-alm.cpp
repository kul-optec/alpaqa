#include <alpaqa/implementation/inner/panoc.tpp>
#include <alpaqa/implementation/outer/alm.tpp>
#include <alpaqa/panoc-alm.hpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_TEMPLATE(class, PANOCSolver, LBFGSDirection<EigenConfigd>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(class, PANOCSolver, LBFGSDirection<EigenConfigf>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(class, PANOCSolver, LBFGSDirection<EigenConfigl>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(class, PANOCSolver, LBFGSDirection<EigenConfigq>);)

ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, PANOCSolver<LBFGSDirection<EigenConfigd>>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, PANOCSolver<LBFGSDirection<EigenConfigf>>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, PANOCSolver<LBFGSDirection<EigenConfigl>>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, PANOCSolver<LBFGSDirection<EigenConfigq>>);)
// clang-format on

} // namespace alpaqa