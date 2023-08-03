#pragma once

#include <alpaqa/inner/directions/panoc/structured-lbfgs.hpp>
#include <dict/kwargs-to-struct.hpp>

PARAMS_TABLE_DECL(alpaqa::StructuredLBFGSDirectionParams<Conf>);

// clang-format off
extern PARAMS_TABLE_INST(alpaqa::StructuredLBFGSDirectionParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::StructuredLBFGSDirectionParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::StructuredLBFGSDirectionParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::StructuredLBFGSDirectionParams<alpaqa::EigenConfigq>);)
// clang-format on
