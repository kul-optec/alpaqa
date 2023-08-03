#pragma once

#include <alpaqa/inner/inner-solve-options.hpp>
#include <dict/kwargs-to-struct.hpp>

PARAMS_TABLE_DECL(alpaqa::InnerSolveOptions<Conf>);

extern PARAMS_TABLE_INST(alpaqa::InnerSolveOptions<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(extern PARAMS_TABLE_INST(alpaqa::InnerSolveOptions<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(extern PARAMS_TABLE_INST(alpaqa::InnerSolveOptions<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(extern PARAMS_TABLE_INST(alpaqa::InnerSolveOptions<alpaqa::EigenConfigq>);)
