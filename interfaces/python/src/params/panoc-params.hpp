#pragma once

#include <alpaqa/inner/panoc.hpp>
#include <dict/kwargs-to-struct.hpp>

PARAMS_TABLE_DECL(alpaqa::PANOCParams<Conf>);
PARAMS_TABLE_DECL(alpaqa::LipschitzEstimateParams<Conf>);

extern PARAMS_TABLE_INST(alpaqa::PANOCParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::PANOCParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::PANOCParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::PANOCParams<alpaqa::EigenConfigq>);)

extern PARAMS_TABLE_INST(alpaqa::LipschitzEstimateParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::LipschitzEstimateParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::LipschitzEstimateParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::LipschitzEstimateParams<alpaqa::EigenConfigq>);)
