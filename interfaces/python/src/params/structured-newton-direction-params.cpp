#include "structured-newton-direction-params.hpp"

PARAMS_TABLE_DEF(alpaqa::StructuredNewtonRegularizationParams<Conf>, //
                 PARAMS_MEMBER(min_eig),                             //
                 PARAMS_MEMBER(print_eig),                           //
);
PARAMS_TABLE_DEF(alpaqa::StructuredNewtonDirectionParams<Conf>, //
                 PARAMS_MEMBER(hessian_vec_factor),             //
);

// clang-format off
PARAMS_TABLE_INST(alpaqa::StructuredNewtonRegularizationParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::StructuredNewtonRegularizationParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::StructuredNewtonRegularizationParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::StructuredNewtonRegularizationParams<alpaqa::EigenConfigq>);)

PARAMS_TABLE_INST(alpaqa::StructuredNewtonDirectionParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::StructuredNewtonDirectionParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::StructuredNewtonDirectionParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::StructuredNewtonDirectionParams<alpaqa::EigenConfigq>);)
// clang-format on
