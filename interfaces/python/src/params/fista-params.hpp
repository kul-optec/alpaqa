#pragma once

#include <alpaqa/inner/fista.hpp>
#include <dict/kwargs-to-struct.hpp>

template <alpaqa::Config Conf>
PARAMS_TABLE_DECL(alpaqa::FISTAParams<Conf>);

extern PARAMS_TABLE_INST(alpaqa::FISTAParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::FISTAParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::FISTAParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::FISTAParams<alpaqa::EigenConfigq>);)
