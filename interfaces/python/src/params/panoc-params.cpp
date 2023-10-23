#include "panoc-params.hpp"

PARAMS_TABLE_DEF(alpaqa::PANOCParams<Conf>,                                     //
                 PARAMS_MEMBER(Lipschitz),                                      //
                 PARAMS_MEMBER(max_iter),                                       //
                 PARAMS_MEMBER(max_time),                                       //
                 PARAMS_MEMBER(min_linesearch_coefficient),                     //
                 PARAMS_MEMBER(force_linesearch),                               //
                 PARAMS_MEMBER(linesearch_strictness_factor),                   //
                 PARAMS_MEMBER(L_min),                                          //
                 PARAMS_MEMBER(L_max),                                          //
                 PARAMS_MEMBER(stop_crit),                                      //
                 PARAMS_MEMBER(max_no_progress),                                //
                 PARAMS_MEMBER(print_interval),                                 //
                 PARAMS_MEMBER(print_precision),                                //
                 PARAMS_MEMBER(quadratic_upperbound_tolerance_factor),          //
                 PARAMS_MEMBER(linesearch_tolerance_factor),                    //
                 PARAMS_MEMBER(update_direction_in_candidate),                  //
                 PARAMS_MEMBER(recompute_last_prox_step_after_stepsize_change), //
                 PARAMS_MEMBER(eager_gradient_eval),                            //
);

PARAMS_TABLE_DEF(alpaqa::LipschitzEstimateParams<Conf>, //
                 PARAMS_MEMBER(L_0),                    //
                 PARAMS_MEMBER(δ),                      //
                 PARAMS_MEMBER(ε),                      //
                 PARAMS_MEMBER(Lγ_factor),              //
);

PARAMS_TABLE_INST(alpaqa::PANOCParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::PANOCParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::PANOCParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::PANOCParams<alpaqa::EigenConfigq>);)

PARAMS_TABLE_INST(alpaqa::LipschitzEstimateParams<alpaqa::EigenConfigd>);
ALPAQA_IF_FLOAT(PARAMS_TABLE_INST(alpaqa::LipschitzEstimateParams<alpaqa::EigenConfigf>);)
ALPAQA_IF_LONGD(PARAMS_TABLE_INST(alpaqa::LipschitzEstimateParams<alpaqa::EigenConfigl>);)
ALPAQA_IF_QUADF(PARAMS_TABLE_INST(alpaqa::LipschitzEstimateParams<alpaqa::EigenConfigq>);)
