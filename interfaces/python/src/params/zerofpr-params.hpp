#pragma once

#include <alpaqa/inner/zerofpr.hpp>
#include <params/params.hpp>

PARAMS_TABLE_DECL(alpaqa::ZeroFPRParams<Conf>);

extern PARAMS_TABLE_INST(alpaqa::ZeroFPRParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::ZeroFPRParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::ZeroFPRParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::ZeroFPRParams<alpaqa::EigenConfigq>);)
