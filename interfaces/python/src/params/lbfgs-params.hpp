#pragma once

#include <alpaqa/accelerators/lbfgs.hpp>
#include <dict/kwargs-to-struct.hpp>

PARAMS_TABLE_DECL(alpaqa::LBFGSParams<Conf>);
PARAMS_TABLE_DECL(alpaqa::CBFGSParams<Conf>);

extern PARAMS_TABLE_INST(alpaqa::LBFGSParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::LBFGSParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::LBFGSParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::LBFGSParams<alpaqa::EigenConfigq>);)

extern PARAMS_TABLE_INST(alpaqa::CBFGSParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::CBFGSParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::CBFGSParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::CBFGSParams<alpaqa::EigenConfigq>);)
