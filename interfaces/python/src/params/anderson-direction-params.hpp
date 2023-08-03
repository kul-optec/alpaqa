#pragma once

#include <alpaqa/inner/directions/panoc/anderson.hpp>
#include <dict/kwargs-to-struct.hpp>

PARAMS_TABLE_DECL(alpaqa::AndersonDirectionParams<Conf>);

extern PARAMS_TABLE_INST(alpaqa::AndersonDirectionParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::AndersonDirectionParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::AndersonDirectionParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::AndersonDirectionParams<alpaqa::EigenConfigq>);)
