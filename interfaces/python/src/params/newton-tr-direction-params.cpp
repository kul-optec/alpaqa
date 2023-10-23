#include "newton-tr-direction-params.hpp"

PARAMS_TABLE_DEF(alpaqa::NewtonTRDirectionParams<Conf>, //
                 PARAMS_MEMBER(hessian_vec_factor),     //
                 PARAMS_MEMBER(finite_diff),            //
                 PARAMS_MEMBER(finite_diff_stepsize),   //
);

PARAMS_TABLE_INST(alpaqa::NewtonTRDirectionParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::NewtonTRDirectionParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::NewtonTRDirectionParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::NewtonTRDirectionParams<alpaqa::EigenConfigq>);)
