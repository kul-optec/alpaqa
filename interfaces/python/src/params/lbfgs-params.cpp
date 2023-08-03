#include "lbfgs-params.hpp"

PARAMS_TABLE_DEF(alpaqa::LBFGSParams<Conf>,    //
                 PARAMS_MEMBER(memory),        //
                 PARAMS_MEMBER(min_div_fac),   //
                 PARAMS_MEMBER(min_abs_s),     //
                 PARAMS_MEMBER(cbfgs),         //
                 PARAMS_MEMBER(force_pos_def), //
                 PARAMS_MEMBER(stepsize),      //
);

PARAMS_TABLE_DEF(alpaqa::CBFGSParams<Conf>, //
                 PARAMS_MEMBER(α),          //
                 PARAMS_MEMBER(ϵ),          //
);

PARAMS_TABLE_INST(alpaqa::LBFGSParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::LBFGSParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::LBFGSParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::LBFGSParams<alpaqa::EigenConfigq>);)

PARAMS_TABLE_INST(alpaqa::CBFGSParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::CBFGSParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::CBFGSParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::CBFGSParams<alpaqa::EigenConfigq>);)