#include <alpaqa/implementation/params/json.tpp>
#include <alpaqa/qpalm/qpalm-adapter.hpp>

namespace alpaqa::params {

#include <alpaqa/qpalm/qpalm-structs.ipp>

template void QPALM_ADAPTER_EXPORT set_param(qpalm::Settings &, const json &);
template void QPALM_ADAPTER_EXPORT get_param(const qpalm::Settings &, json &);

} // namespace alpaqa::params