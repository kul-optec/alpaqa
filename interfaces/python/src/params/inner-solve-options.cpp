#include "params.hpp"

PARAMS_TABLE_DEF(alpaqa::InnerSolveOptions<Conf>,
                 PARAMS_MEMBER(always_overwrite_results), //
                 PARAMS_MEMBER(max_time),                 //
                 PARAMS_MEMBER(tolerance),                //
);

PARAMS_TABLE_INST(alpaqa::InnerSolveOptions<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::InnerSolveOptions<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::InnerSolveOptions<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::InnerSolveOptions<alpaqa::EigenConfigq>);)
