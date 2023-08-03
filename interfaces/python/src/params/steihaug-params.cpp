#include "steihaug-params.hpp"

PARAMS_TABLE_DEF(alpaqa::SteihaugCGParams<Conf>, //
                 PARAMS_MEMBER(tol_scale),       //
                 PARAMS_MEMBER(tol_scale_root),  //
                 PARAMS_MEMBER(tol_max),         //
                 PARAMS_MEMBER(max_iter_factor), //
);

PARAMS_TABLE_INST(alpaqa::SteihaugCGParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::SteihaugCGParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::SteihaugCGParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::SteihaugCGParams<alpaqa::EigenConfigq>);)
