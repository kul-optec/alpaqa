#pragma once

#include <alpaqa/inner/directions/panoc/convex-newton.hpp>
#include <dict/kwargs-to-struct.hpp>

template <alpaqa::Config Conf>
PARAMS_TABLE_DECL(alpaqa::ConvexNewtonRegularizationParams<Conf>);
template <alpaqa::Config Conf>
PARAMS_TABLE_DECL(alpaqa::ConvexNewtonDirectionParams<Conf>);

// clang-format off
extern PARAMS_TABLE_INST(alpaqa::ConvexNewtonRegularizationParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::ConvexNewtonRegularizationParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::ConvexNewtonRegularizationParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::ConvexNewtonRegularizationParams<alpaqa::EigenConfigq>);)

extern PARAMS_TABLE_INST(alpaqa::ConvexNewtonDirectionParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::ConvexNewtonDirectionParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::ConvexNewtonDirectionParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::ConvexNewtonDirectionParams<alpaqa::EigenConfigq>);)
// clang-format on
