#pragma once

#include <alpaqa/accelerators/anderson.hpp>
#include <dict/kwargs-to-struct.hpp>

PARAMS_TABLE_DECL(alpaqa::AndersonAccelParams<Conf>);

extern PARAMS_TABLE_INST(alpaqa::AndersonAccelParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::AndersonAccelParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::AndersonAccelParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::AndersonAccelParams<alpaqa::EigenConfigq>);)
