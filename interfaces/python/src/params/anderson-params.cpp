#include <params/anderson-params.hpp>

PARAMS_TABLE_DEF(alpaqa::AndersonAccelParams<Conf>, //
                 PARAMS_MEMBER(memory),             //
                 PARAMS_MEMBER(min_div_fac),        //
);

PARAMS_TABLE_INST(alpaqa::AndersonAccelParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::AndersonAccelParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::AndersonAccelParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::AndersonAccelParams<alpaqa::EigenConfigq>);)
