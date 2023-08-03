#pragma once

#include <alpaqa/inner/panoc-ocp.hpp>
#include <dict/kwargs-to-struct.hpp>

PARAMS_TABLE_DECL(alpaqa::PANOCOCPParams<Conf>);

extern PARAMS_TABLE_INST(alpaqa::PANOCOCPParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::PANOCOCPParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::PANOCOCPParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::PANOCOCPParams<alpaqa::EigenConfigq>);)
