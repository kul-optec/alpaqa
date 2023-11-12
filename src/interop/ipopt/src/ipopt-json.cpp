#include <alpaqa/ipopt-adapter-export.h>
#include <alpaqa/params/json.hpp>
#include <nlohmann/json.hpp>

#include <IpIpoptApplication.hpp>

namespace alpaqa::params {

template <>
void IPOPT_ADAPTER_EXPORT set_param(Ipopt::IpoptApplication &, const json &j) {
    if (!j.empty())
        throw invalid_json_param(
            "JSON parameters for Ipopt are currently unsupported");
}

template <>
void IPOPT_ADAPTER_EXPORT get_param(const Ipopt::IpoptApplication &, json &j) {
    j = json::object();
}

} // namespace alpaqa::params
