ENUM_TABLE(PANOCStopCrit,                  //
           ENUM_MEMBER(ApproxKKT),         //
           ENUM_MEMBER(ApproxKKT2),        //
           ENUM_MEMBER(ProjGradNorm),      //
           ENUM_MEMBER(ProjGradNorm2),     //
           ENUM_MEMBER(ProjGradUnitNorm),  //
           ENUM_MEMBER(ProjGradUnitNorm2), //
           ENUM_MEMBER(FPRNorm),           //
           ENUM_MEMBER(FPRNorm2),          //
);

ENUM_TABLE(LBFGSStepSize,                        //
           ENUM_MEMBER(BasedOnExternalStepSize), //
           ENUM_MEMBER(BasedOnCurvature),        //
);

PARAMS_TABLE(LBFGSParams<config_t>,            //
             PARAMS_MEMBER(memory, ""),        //
             PARAMS_MEMBER(min_div_fac, ""),   //
             PARAMS_MEMBER(min_abs_s, ""),     //
             PARAMS_MEMBER(cbfgs, ""),         //
             PARAMS_MEMBER(force_pos_def, ""), //
             PARAMS_MEMBER(stepsize, ""),      //
);

PARAMS_TABLE(AndersonAccelParams<config_t>,  //
             PARAMS_MEMBER(memory, ""),      //
             PARAMS_MEMBER(min_div_fac, ""), //
);

PARAMS_TABLE(CBFGSParams<config_t>, //
             PARAMS_MEMBER(α, ""),  //
             PARAMS_MEMBER(ϵ, ""),  //
);
PARAMS_ALIAS_TABLE(CBFGSParams<config_t>,           //
                   PARAMS_MEMBER_ALIAS(alpha, α),   //
                   PARAMS_MEMBER_ALIAS(epsilon, ϵ), //
);

PARAMS_TABLE(LipschitzEstimateParams<config_t>, //
             PARAMS_MEMBER(L_0, ""),            //
             PARAMS_MEMBER(δ, ""),              //
             PARAMS_MEMBER(ε, ""),              //
             PARAMS_MEMBER(Lγ_factor, ""),      //
);
PARAMS_ALIAS_TABLE(LipschitzEstimateParams<config_t>,              //
                   PARAMS_MEMBER_ALIAS(delta, δ),                  //
                   PARAMS_MEMBER_ALIAS(epsilon, ε),                //
                   PARAMS_MEMBER_ALIAS(L_gamma_factor, Lγ_factor), //
);

PARAMS_TABLE(PANTRParams<config_t>,                                    //
             PARAMS_MEMBER(Lipschitz, ""),                             //
             PARAMS_MEMBER(max_iter, ""),                              //
             PARAMS_MEMBER(max_time, ""),                              //
             PARAMS_MEMBER(L_min, ""),                                 //
             PARAMS_MEMBER(L_max, ""),                                 //
             PARAMS_MEMBER(stop_crit, ""),                             //
             PARAMS_MEMBER(max_no_progress, ""),                       //
             PARAMS_MEMBER(print_interval, ""),                        //
             PARAMS_MEMBER(print_precision, ""),                       //
             PARAMS_MEMBER(quadratic_upperbound_tolerance_factor, ""), //
             PARAMS_MEMBER(TR_tolerance_factor, ""),                   //
             PARAMS_MEMBER(ratio_threshold_acceptable, ""),            //
             PARAMS_MEMBER(ratio_threshold_good, ""),                  //
             PARAMS_MEMBER(radius_factor_rejected, ""),                //
             PARAMS_MEMBER(radius_factor_acceptable, ""),              //
             PARAMS_MEMBER(radius_factor_good, ""),                    //
             PARAMS_MEMBER(initial_radius, ""),                        //
             PARAMS_MEMBER(min_radius, ""),                            //
             PARAMS_MEMBER(compute_ratio_using_new_stepsize, ""),      //
             PARAMS_MEMBER(update_direction_on_prox_step, ""),         //
             PARAMS_MEMBER(recompute_last_prox_step_after_direction_reset,
                           ""),                                   //
             PARAMS_MEMBER(disable_acceleration, ""),             //
             PARAMS_MEMBER(ratio_approx_fbe_quadratic_model, ""), //
);

PARAMS_TABLE(PANOCParams<config_t>,                                    //
             PARAMS_MEMBER(Lipschitz, ""),                             //
             PARAMS_MEMBER(max_iter, ""),                              //
             PARAMS_MEMBER(max_time, ""),                              //
             PARAMS_MEMBER(min_linesearch_coefficient, ""),            //
             PARAMS_MEMBER(linesearch_coefficient_update_factor, ""),  //
             PARAMS_MEMBER(force_linesearch, ""),                      //
             PARAMS_MEMBER(linesearch_strictness_factor, ""),          //
             PARAMS_MEMBER(L_min, ""),                                 //
             PARAMS_MEMBER(L_max, ""),                                 //
             PARAMS_MEMBER(stop_crit, ""),                             //
             PARAMS_MEMBER(max_no_progress, ""),                       //
             PARAMS_MEMBER(print_interval, ""),                        //
             PARAMS_MEMBER(print_precision, ""),                       //
             PARAMS_MEMBER(quadratic_upperbound_tolerance_factor, ""), //
             PARAMS_MEMBER(linesearch_tolerance_factor, ""),           //
             PARAMS_MEMBER(update_direction_in_candidate, ""),         //
             PARAMS_MEMBER(recompute_last_prox_step_after_stepsize_change,
                           ""),                      //
             PARAMS_MEMBER(eager_gradient_eval, ""), //
);

PARAMS_TABLE(FISTAParams<config_t>,                                    //
             PARAMS_MEMBER(Lipschitz, ""),                             //
             PARAMS_MEMBER(max_iter, ""),                              //
             PARAMS_MEMBER(max_time, ""),                              //
             PARAMS_MEMBER(L_min, ""),                                 //
             PARAMS_MEMBER(L_max, ""),                                 //
             PARAMS_MEMBER(stop_crit, ""),                             //
             PARAMS_MEMBER(max_no_progress, ""),                       //
             PARAMS_MEMBER(print_interval, ""),                        //
             PARAMS_MEMBER(print_precision, ""),                       //
             PARAMS_MEMBER(quadratic_upperbound_tolerance_factor, ""), //
             PARAMS_MEMBER(disable_acceleration, ""),                  //
);

PARAMS_TABLE(ZeroFPRParams<config_t>,                                  //
             PARAMS_MEMBER(Lipschitz, ""),                             //
             PARAMS_MEMBER(max_iter, ""),                              //
             PARAMS_MEMBER(max_time, ""),                              //
             PARAMS_MEMBER(min_linesearch_coefficient, ""),            //
             PARAMS_MEMBER(force_linesearch, ""),                      //
             PARAMS_MEMBER(linesearch_strictness_factor, ""),          //
             PARAMS_MEMBER(L_min, ""),                                 //
             PARAMS_MEMBER(L_max, ""),                                 //
             PARAMS_MEMBER(stop_crit, ""),                             //
             PARAMS_MEMBER(max_no_progress, ""),                       //
             PARAMS_MEMBER(print_interval, ""),                        //
             PARAMS_MEMBER(print_precision, ""),                       //
             PARAMS_MEMBER(quadratic_upperbound_tolerance_factor, ""), //
             PARAMS_MEMBER(linesearch_tolerance_factor, ""),           //
             PARAMS_MEMBER(update_direction_in_candidate, ""),         //
             PARAMS_MEMBER(recompute_last_prox_step_after_stepsize_change,
                           ""),                                  //
             PARAMS_MEMBER(update_direction_from_prox_step, ""), //
);

PARAMS_TABLE(LBFGSDirectionParams<config_t>,                  //
             PARAMS_MEMBER(rescale_on_step_size_changes, ""), //
);

PARAMS_TABLE(AndersonDirectionParams<config_t>,               //
             PARAMS_MEMBER(rescale_on_step_size_changes, ""), //
);

PARAMS_TABLE(StructuredLBFGSDirectionParams<config_t>,          //
             PARAMS_MEMBER(hessian_vec_factor, ""),             //
             PARAMS_MEMBER(hessian_vec_finite_differences, ""), //
             PARAMS_MEMBER(full_augmented_hessian, ""),         //
);

PARAMS_TABLE(NewtonTRDirectionParams<config_t>,       //
             PARAMS_MEMBER(hessian_vec_factor, ""),   //
             PARAMS_MEMBER(finite_diff, ""),          //
             PARAMS_MEMBER(finite_diff_stepsize, ""), //
);

PARAMS_TABLE(SteihaugCGParams<config_t>,         //
             PARAMS_MEMBER(tol_scale, ""),       //
             PARAMS_MEMBER(tol_scale_root, ""),  //
             PARAMS_MEMBER(tol_max, ""),         //
             PARAMS_MEMBER(max_iter_factor, ""), //
);

PARAMS_TABLE(StructuredNewtonRegularizationParams<config_t>, //
             PARAMS_MEMBER(min_eig, ""),                     //
             PARAMS_MEMBER(print_eig, ""),                   //
);

PARAMS_TABLE(StructuredNewtonDirectionParams<config_t>, //
             PARAMS_MEMBER(hessian_vec_factor, ""),     //
);

PARAMS_TABLE(ConvexNewtonRegularizationParams<config_t>, //
             PARAMS_MEMBER(ζ, ""),                       //
             PARAMS_MEMBER(ν, ""),                       //
             PARAMS_MEMBER(ldlt, ""),                    //
);

PARAMS_TABLE(ConvexNewtonDirectionParams<config_t>, //
             PARAMS_MEMBER(hessian_vec_factor, ""), //
             PARAMS_MEMBER(quadratic, ""),          //
);

PARAMS_TABLE(ALMParams<config_t>,                               //
             PARAMS_MEMBER(tolerance, ""),                      //
             PARAMS_MEMBER(dual_tolerance, ""),                 //
             PARAMS_MEMBER(penalty_update_factor, ""),          //
             PARAMS_MEMBER(initial_penalty, ""),                //
             PARAMS_MEMBER(initial_penalty_factor, ""),         //
             PARAMS_MEMBER(initial_tolerance, ""),              //
             PARAMS_MEMBER(tolerance_update_factor, ""),        //
             PARAMS_MEMBER(rel_penalty_increase_threshold, ""), //
             PARAMS_MEMBER(max_multiplier, ""),                 //
             PARAMS_MEMBER(max_penalty, ""),                    //
             PARAMS_MEMBER(min_penalty, ""),                    //
             PARAMS_MEMBER(max_iter, ""),                       //
             PARAMS_MEMBER(max_time, ""),                       //
             PARAMS_MEMBER(print_interval, ""),                 //
             PARAMS_MEMBER(print_precision, ""),                //
             PARAMS_MEMBER(single_penalty_factor, ""),          //
);

#if ALPAQA_WITH_OCP
PARAMS_TABLE(PANOCOCPParams<config_t>, PARAMS_MEMBER(Lipschitz, ""),   //
             PARAMS_MEMBER(max_iter, ""),                              //
             PARAMS_MEMBER(max_time, ""),                              //
             PARAMS_MEMBER(min_linesearch_coefficient, ""),            //
             PARAMS_MEMBER(linesearch_strictness_factor, ""),          //
             PARAMS_MEMBER(L_min, ""),                                 //
             PARAMS_MEMBER(L_max, ""),                                 //
             PARAMS_MEMBER(L_max_inc, ""),                             //
             PARAMS_MEMBER(stop_crit, ""),                             //
             PARAMS_MEMBER(max_no_progress, ""),                       //
             PARAMS_MEMBER(gn_interval, ""),                           //
             PARAMS_MEMBER(gn_sticky, ""),                             //
             PARAMS_MEMBER(reset_lbfgs_on_gn_step, ""),                //
             PARAMS_MEMBER(lqr_factor_cholesky, ""),                   //
             PARAMS_MEMBER(lbfgs_params, ""),                          //
             PARAMS_MEMBER(print_interval, ""),                        //
             PARAMS_MEMBER(print_precision, ""),                       //
             PARAMS_MEMBER(quadratic_upperbound_tolerance_factor, ""), //
             PARAMS_MEMBER(linesearch_tolerance_factor, ""),           //
             PARAMS_MEMBER(disable_acceleration, ""),                  //
);
#endif