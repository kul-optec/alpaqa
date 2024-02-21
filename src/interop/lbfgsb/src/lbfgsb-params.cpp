#include <alpaqa/implementation/params/params.tpp>
#include <alpaqa/lbfgsb/lbfgsb-adapter.hpp>

namespace alpaqa::params {

#include <alpaqa/lbfgsb/lbfgsb-structs.ipp>

template <>
void LBFGSB_ADAPTER_EXPORT set_param(lbfgsb::LBFGSBSolver::Params &t,
                                     ParamString s) {
    set_param_default(t, s);
}

} // namespace alpaqa::params