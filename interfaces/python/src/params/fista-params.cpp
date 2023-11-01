#include "fista-params.hpp"

template <alpaqa::Config Conf>
PARAMS_TABLE_DEF(alpaqa::FISTAParams<Conf>,                            //
                 PARAMS_MEMBER(Lipschitz),                             //
                 PARAMS_MEMBER(max_iter),                              //
                 PARAMS_MEMBER(max_time),                              //
                 PARAMS_MEMBER(L_min),                                 //
                 PARAMS_MEMBER(L_max),                                 //
                 PARAMS_MEMBER(stop_crit),                             //
                 PARAMS_MEMBER(max_no_progress),                       //
                 PARAMS_MEMBER(print_interval),                        //
                 PARAMS_MEMBER(print_precision),                       //
                 PARAMS_MEMBER(quadratic_upperbound_tolerance_factor), //
);

PARAMS_TABLE_INST(alpaqa::FISTAParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::FISTAParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::FISTAParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::FISTAParams<alpaqa::EigenConfigq>);)
