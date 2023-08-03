#include "structured-lbfgs-direction-params.hpp"

PARAMS_TABLE_DEF(alpaqa::StructuredLBFGSDirectionParams<Conf>,  //
                 PARAMS_MEMBER(hessian_vec_factor),             //
                 PARAMS_MEMBER(hessian_vec_finite_differences), //
                 PARAMS_MEMBER(full_augmented_hessian),         //
);

PARAMS_TABLE_INST(alpaqa::StructuredLBFGSDirectionParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::StructuredLBFGSDirectionParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::StructuredLBFGSDirectionParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::StructuredLBFGSDirectionParams<alpaqa::EigenConfigq>);)
