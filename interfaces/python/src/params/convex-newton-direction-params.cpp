#include "convex-newton-direction-params.hpp"

template <alpaqa::Config Conf>
PARAMS_TABLE_DEF(alpaqa::ConvexNewtonRegularizationParams<Conf>, //
                 PARAMS_MEMBER(ζ),                               //
                 PARAMS_MEMBER(ν),                               //
                 PARAMS_MEMBER(ldlt),                            //
);
template <alpaqa::Config Conf>
PARAMS_TABLE_DEF(alpaqa::ConvexNewtonDirectionParams<Conf>, //
                 PARAMS_MEMBER(hessian_vec_factor),         //
                 PARAMS_MEMBER(quadratic),                  //
);

// clang-format off
PARAMS_TABLE_INST(alpaqa::ConvexNewtonRegularizationParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::ConvexNewtonRegularizationParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::ConvexNewtonRegularizationParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::ConvexNewtonRegularizationParams<alpaqa::EigenConfigq>);)

PARAMS_TABLE_INST(alpaqa::ConvexNewtonDirectionParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::ConvexNewtonDirectionParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::ConvexNewtonDirectionParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::ConvexNewtonDirectionParams<alpaqa::EigenConfigq>);)
// clang-format on
