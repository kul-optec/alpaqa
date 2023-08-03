#pragma once

#include <alpaqa/outer/alm.hpp>

#include <dict/kwargs-to-struct.hpp>

PARAMS_TABLE_DECL(alpaqa::ALMParams<Conf>);

extern PARAMS_TABLE_INST(alpaqa::ALMParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::ALMParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::ALMParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::ALMParams<alpaqa::EigenConfigq>);)
