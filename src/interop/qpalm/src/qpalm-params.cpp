#include <alpaqa/implementation/params/params.tpp>
#include <alpaqa/qpalm/qpalm-adapter.hpp>

namespace alpaqa::params {

#include <alpaqa/qpalm/qpalm-structs.ipp>

template void QPALM_ADAPTER_EXPORT set_param(qpalm::Settings &, ParamString);

} // namespace alpaqa::params