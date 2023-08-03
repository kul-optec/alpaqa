#include <alpaqa/implementation/inner/panoc.tpp>
#include <alpaqa/implementation/outer/alm.tpp>
#include <alpaqa/panoc-anderson-alm.hpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_TEMPLATE(class, PANOCSolver, AndersonDirection<EigenConfigd>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(class, PANOCSolver, AndersonDirection<EigenConfigf>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(class, PANOCSolver, AndersonDirection<EigenConfigl>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(class, PANOCSolver, AndersonDirection<EigenConfigq>);)

ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, PANOCSolver<AndersonDirection<EigenConfigd>>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, PANOCSolver<AndersonDirection<EigenConfigf>>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, PANOCSolver<AndersonDirection<EigenConfigl>>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, PANOCSolver<AndersonDirection<EigenConfigq>>);)
// clang-format on

} // namespace alpaqa