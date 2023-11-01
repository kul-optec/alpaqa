#include "lbfgsb-params.hpp"

PARAMS_TABLE_DEF(alpaqa::lbfgsb::LBFGSBParams,   //
                 PARAMS_MEMBER(memory),          //
                 PARAMS_MEMBER(max_iter),        //
                 PARAMS_MEMBER(max_time),        //
                 PARAMS_MEMBER(stop_crit),       //
                 PARAMS_MEMBER(print),           //
                 PARAMS_MEMBER(print_interval),  //
                 PARAMS_MEMBER(print_precision), //
);

PARAMS_TABLE_INST(alpaqa::lbfgsb::LBFGSBParams);
