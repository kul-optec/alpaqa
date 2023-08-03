#include "params.hpp"

PARAMS_TABLE_DEF(alpaqa::LBFGSDirectionParams<Conf>,          //
                 PARAMS_MEMBER(rescale_on_step_size_changes), //
);

PARAMS_TABLE_INST(alpaqa::LBFGSDirectionParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::LBFGSDirectionParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::LBFGSDirectionParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::LBFGSDirectionParams<alpaqa::EigenConfigq>);)