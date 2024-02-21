#include <alpaqa/implementation/params/params.tpp>
#include <alpaqa/qpalm/qpalm-adapter.hpp>

namespace alpaqa::params {

#include <alpaqa/qpalm/qpalm-structs.ipp>

template <>
void QPALM_ADAPTER_EXPORT set_param(qpalm::Settings &t, ParamString s) {
    set_param_default(t, s);
}

} // namespace alpaqa::params