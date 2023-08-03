#pragma once

#include <alpaqa/inner/directions/pantr/newton-tr.hpp>
#include <alpaqa/inner/pantr.hpp>
#include <alpaqa/outer/alm.hpp>

namespace alpaqa {

// clang-format off
ALPAQA_EXPORT_EXTERN_TEMPLATE(class, PANTRSolver, NewtonTRDirection<EigenConfigd>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, PANTRSolver, NewtonTRDirection<EigenConfigf>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, PANTRSolver, NewtonTRDirection<EigenConfigl>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, PANTRSolver, NewtonTRDirection<EigenConfigq>);)

ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ALMSolver, PANTRSolver<NewtonTRDirection<EigenConfigd>>);
ALPAQA_IF_FLOAT(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ALMSolver, PANTRSolver<NewtonTRDirection<EigenConfigf>>);)
ALPAQA_IF_LONGD(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ALMSolver, PANTRSolver<NewtonTRDirection<EigenConfigl>>);)
ALPAQA_IF_QUADF(ALPAQA_EXPORT_EXTERN_TEMPLATE(class, ALMSolver, PANTRSolver<NewtonTRDirection<EigenConfigq>>);)
// clang-format on

} // namespace alpaqa
