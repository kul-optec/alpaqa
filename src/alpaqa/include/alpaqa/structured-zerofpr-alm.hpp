#pragma once

#include <alpaqa/inner/directions/panoc/structured-lbfgs.hpp>
#include <alpaqa/inner/zerofpr.hpp>
#include <alpaqa/outer/alm.hpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ZeroFPRSolver, StructuredLBFGSDirection<EigenConfigd>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ZeroFPRSolver, StructuredLBFGSDirection<EigenConfigf>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ZeroFPRSolver, StructuredLBFGSDirection<EigenConfigl>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ZeroFPRSolver, StructuredLBFGSDirection<EigenConfigq>);)

ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ALMSolver, ZeroFPRSolver<StructuredLBFGSDirection<EigenConfigd>>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ALMSolver, ZeroFPRSolver<StructuredLBFGSDirection<EigenConfigf>>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ALMSolver, ZeroFPRSolver<StructuredLBFGSDirection<EigenConfigl>>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ALMSolver, ZeroFPRSolver<StructuredLBFGSDirection<EigenConfigq>>);)
// clang-format on

} // namespace alpaqa
