#pragma once

#include <alpaqa/inner/directions/panoc/anderson.hpp>
#include <alpaqa/inner/panoc.hpp>
#include <alpaqa/outer/alm.hpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_EXTERN_TEMPLATE(class, PANOCSolver, AndersonDirection<EigenConfigd>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, PANOCSolver, AndersonDirection<EigenConfigf>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, PANOCSolver, AndersonDirection<EigenConfigl>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, PANOCSolver, AndersonDirection<EigenConfigq>);)

ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ALMSolver, PANOCSolver<AndersonDirection<EigenConfigd>>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ALMSolver, PANOCSolver<AndersonDirection<EigenConfigf>>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ALMSolver, PANOCSolver<AndersonDirection<EigenConfigl>>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ALMSolver, PANOCSolver<AndersonDirection<EigenConfigq>>);)
// clang-format on

} // namespace alpaqa
