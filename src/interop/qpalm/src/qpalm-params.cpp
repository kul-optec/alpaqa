#include <alpaqa/implementation/params/params.tpp>
#include <alpaqa/qpalm/qpalm-adapter.hpp>

namespace alpaqa::params {

PARAMS_TABLE(qpalm::Settings,                         //
             PARAMS_MEMBER(max_iter),                 //
             PARAMS_MEMBER(inner_max_iter),           //
             PARAMS_MEMBER(eps_abs),                  //
             PARAMS_MEMBER(eps_rel),                  //
             PARAMS_MEMBER(eps_abs_in),               //
             PARAMS_MEMBER(eps_rel_in),               //
             PARAMS_MEMBER(rho),                      //
             PARAMS_MEMBER(eps_prim_inf),             //
             PARAMS_MEMBER(eps_dual_inf),             //
             PARAMS_MEMBER(theta),                    //
             PARAMS_MEMBER(delta),                    //
             PARAMS_MEMBER(sigma_max),                //
             PARAMS_MEMBER(sigma_init),               //
             PARAMS_MEMBER(proximal),                 //
             PARAMS_MEMBER(gamma_init),               //
             PARAMS_MEMBER(gamma_upd),                //
             PARAMS_MEMBER(gamma_max),                //
             PARAMS_MEMBER(scaling),                  //
             PARAMS_MEMBER(nonconvex),                //
             PARAMS_MEMBER(verbose),                  //
             PARAMS_MEMBER(print_iter),               //
             PARAMS_MEMBER(warm_start),               //
             PARAMS_MEMBER(reset_newton_iter),        //
             PARAMS_MEMBER(enable_dual_termination),  //
             PARAMS_MEMBER(dual_objective_limit),     //
             PARAMS_MEMBER(time_limit),               //
             PARAMS_MEMBER(ordering),                 //
             PARAMS_MEMBER(factorization_method),     //
             PARAMS_MEMBER(max_rank_update),          //
             PARAMS_MEMBER(max_rank_update_fraction), //
);

template void QPALM_ADAPTER_EXPORT set_param(qpalm::Settings &, ParamString);

} // namespace alpaqa::params