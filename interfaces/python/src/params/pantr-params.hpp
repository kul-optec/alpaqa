#pragma once

#include <alpaqa/inner/pantr.hpp>
#include <params/params.hpp>

PARAMS_TABLE_DECL(alpaqa::PANTRParams<Conf>);

extern PARAMS_TABLE_INST(alpaqa::PANTRParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::PANTRParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::PANTRParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::PANTRParams<alpaqa::EigenConfigq>);)
