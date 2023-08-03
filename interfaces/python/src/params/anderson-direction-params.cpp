#include "params.hpp"

PARAMS_TABLE_DEF(alpaqa::AndersonDirectionParams<Conf>,       //
                 PARAMS_MEMBER(rescale_on_step_size_changes), //
);

PARAMS_TABLE_INST(alpaqa::AndersonDirectionParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::AndersonDirectionParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::AndersonDirectionParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::AndersonDirectionParams<alpaqa::EigenConfigq>);)
