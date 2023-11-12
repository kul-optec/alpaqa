#include <alpaqa/implementation/params/json.tpp>
#include <alpaqa/lbfgsb/lbfgsb-adapter.hpp>

namespace alpaqa::params {

#include <alpaqa/lbfgsb/lbfgsb-params.ipp>

template void LBFGSB_ADAPTER_EXPORT set_param(lbfgsb::LBFGSBSolver::Params &,
                                              const json &);
template void LBFGSB_ADAPTER_EXPORT
get_param(const lbfgsb::LBFGSBSolver::Params &, json &);

} // namespace alpaqa::params