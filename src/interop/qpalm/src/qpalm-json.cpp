#include <alpaqa/implementation/params/json.tpp>
#include <alpaqa/qpalm/qpalm-adapter.hpp>

namespace alpaqa::params {

#include <alpaqa/qpalm/qpalm-structs.ipp>

template <>
void QPALM_ADAPTER_EXPORT set_param(qpalm::Settings &t, const json &j) {
    set_param_default(t, j);
}
template <>
void QPALM_ADAPTER_EXPORT get_param(const qpalm::Settings &t, json &j) {
    get_param_default(t, j);
}

} // namespace alpaqa::params