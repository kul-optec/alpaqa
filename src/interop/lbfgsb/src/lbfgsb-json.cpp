#include <alpaqa/implementation/params/json.tpp>
#include <alpaqa/lbfgsb/lbfgsb-adapter.hpp>

namespace alpaqa::params {

#include <alpaqa/lbfgsb/lbfgsb-structs.ipp>

template <>
void LBFGSB_ADAPTER_EXPORT set_param(lbfgsb::LBFGSBSolver::Params &t,
                                     const json &j) {
    set_param_default(t, j);
}
template <>
void LBFGSB_ADAPTER_EXPORT get_param(const lbfgsb::LBFGSBSolver::Params &t,
                                     json &j) {
    get_param_default(t, j);
}

} // namespace alpaqa::params