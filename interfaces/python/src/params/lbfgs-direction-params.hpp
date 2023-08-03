#pragma once

#include <alpaqa/inner/directions/panoc/lbfgs.hpp>
#include <dict/kwargs-to-struct.hpp>

PARAMS_TABLE_DECL(alpaqa::LBFGSDirectionParams<Conf>);

extern PARAMS_TABLE_INST(alpaqa::LBFGSDirectionParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::LBFGSDirectionParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::LBFGSDirectionParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::LBFGSDirectionParams<alpaqa::EigenConfigq>);)
