#include "params.hpp"

PARAMS_TABLE_DEF(alpaqa::ALMParams<Conf>,                       //
                 PARAMS_MEMBER(tolerance),                      //
                 PARAMS_MEMBER(dual_tolerance),                 //
                 PARAMS_MEMBER(penalty_update_factor),          //
                 PARAMS_MEMBER(initial_penalty),                //
                 PARAMS_MEMBER(initial_penalty_factor),         //
                 PARAMS_MEMBER(initial_tolerance),              //
                 PARAMS_MEMBER(tolerance_update_factor),        //
                 PARAMS_MEMBER(rel_penalty_increase_threshold), //
                 PARAMS_MEMBER(max_multiplier),                 //
                 PARAMS_MEMBER(max_penalty),                    //
                 PARAMS_MEMBER(min_penalty),                    //
                 PARAMS_MEMBER(max_iter),                       //
                 PARAMS_MEMBER(max_time),                       //
                 PARAMS_MEMBER(print_interval),                 //
                 PARAMS_MEMBER(print_precision),                //
                 PARAMS_MEMBER(single_penalty_factor),          //
);

PARAMS_TABLE_INST(alpaqa::ALMParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::ALMParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::ALMParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::ALMParams<alpaqa::EigenConfigq>);)
