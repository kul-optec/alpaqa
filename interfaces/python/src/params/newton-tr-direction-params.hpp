#pragma once

#include <alpaqa/inner/directions/pantr/newton-tr.hpp>
#include <params/params.hpp>

PARAMS_TABLE_DECL(alpaqa::NewtonTRDirectionParams<Conf>);

extern PARAMS_TABLE_INST(alpaqa::NewtonTRDirectionParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::NewtonTRDirectionParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::NewtonTRDirectionParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::NewtonTRDirectionParams<alpaqa::EigenConfigq>);)
