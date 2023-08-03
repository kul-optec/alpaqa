#include <alpaqa/implementation/inner/zerofpr.tpp>
#include <alpaqa/implementation/outer/alm.tpp>
#include <alpaqa/zerofpr-anderson-alm.hpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_TEMPLATE(class, ZeroFPRSolver, AndersonDirection<EigenConfigd>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(class, ZeroFPRSolver, AndersonDirection<EigenConfigf>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(class, ZeroFPRSolver, AndersonDirection<EigenConfigl>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(class, ZeroFPRSolver, AndersonDirection<EigenConfigq>);)

ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, ZeroFPRSolver<AndersonDirection<EigenConfigd>>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, ZeroFPRSolver<AndersonDirection<EigenConfigf>>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, ZeroFPRSolver<AndersonDirection<EigenConfigl>>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, ZeroFPRSolver<AndersonDirection<EigenConfigq>>);)
// clang-format on

} // namespace alpaqa