#pragma once

#include <alpaqa/inner/directions/panoc/structured-newton.hpp>
#include <dict/kwargs-to-struct.hpp>

PARAMS_TABLE_DECL(alpaqa::StructuredNewtonRegularizationParams<Conf>);
PARAMS_TABLE_DECL(alpaqa::StructuredNewtonDirectionParams<Conf>);

// clang-format off
extern PARAMS_TABLE_INST(alpaqa::StructuredNewtonRegularizationParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::StructuredNewtonRegularizationParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::StructuredNewtonRegularizationParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::StructuredNewtonRegularizationParams<alpaqa::EigenConfigq>);)

extern PARAMS_TABLE_INST(alpaqa::StructuredNewtonDirectionParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::StructuredNewtonDirectionParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::StructuredNewtonDirectionParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::StructuredNewtonDirectionParams<alpaqa::EigenConfigq>);)
// clang-format on
