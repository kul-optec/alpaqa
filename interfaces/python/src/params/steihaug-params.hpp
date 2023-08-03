#pragma once

#include <alpaqa/accelerators/steihaugcg.hpp>
#include <params/params.hpp>

PARAMS_TABLE_DECL(alpaqa::SteihaugCGParams<Conf>);

extern PARAMS_TABLE_INST(alpaqa::SteihaugCGParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::SteihaugCGParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::SteihaugCGParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::SteihaugCGParams<alpaqa::EigenConfigq>);)
