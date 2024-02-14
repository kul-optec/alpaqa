#include <alpaqa/implementation/inner/zerofpr.tpp>
#include <alpaqa/implementation/outer/alm.tpp>
#include <alpaqa/zerofpr-convex-newton-alm.hpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_TEMPLATE(class, ZeroFPRSolver, ConvexNewtonDirection<EigenConfigd>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(class, ZeroFPRSolver, ConvexNewtonDirection<EigenConfigf>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(class, ZeroFPRSolver, ConvexNewtonDirection<EigenConfigl>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(class, ZeroFPRSolver, ConvexNewtonDirection<EigenConfigq>);)

ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, ZeroFPRSolver<ConvexNewtonDirection<EigenConfigd>>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, ZeroFPRSolver<ConvexNewtonDirection<EigenConfigf>>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, ZeroFPRSolver<ConvexNewtonDirection<EigenConfigl>>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_TEMPLATE(class, ALMSolver, ZeroFPRSolver<ConvexNewtonDirection<EigenConfigq>>);)
// clang-format on

} // namespace alpaqa