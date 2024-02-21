#include <alpaqa/implementation/params/json.tpp>

#include <alpaqa/params/vec-from-file.hpp>
#include <alpaqa/util/demangled-typename.hpp>
#include <alpaqa/util/duration-parse.hpp>
#include <alpaqa/util/io/csv.hpp>
#include <alpaqa/util/possible-alias.hpp>
#include <algorithm>
#include <cmath>
#include <concepts>
#include <fstream>
#include <limits>
#include <variant>

#include <alpaqa/inner/directions/panoc/anderson.hpp>
#include <alpaqa/inner/directions/panoc/convex-newton.hpp>
#include <alpaqa/inner/directions/panoc/lbfgs.hpp>
#include <alpaqa/inner/directions/panoc/structured-lbfgs.hpp>
#include <alpaqa/inner/directions/panoc/structured-newton.hpp>
#include <alpaqa/inner/directions/pantr/newton-tr.hpp>
#include <alpaqa/inner/fista.hpp>
#include <alpaqa/inner/internal/lipschitz.hpp>
#include <alpaqa/inner/internal/panoc-stop-crit.hpp>
#include <alpaqa/inner/panoc.hpp>
#include <alpaqa/inner/pantr.hpp>
#include <alpaqa/inner/zerofpr.hpp>
#include <alpaqa/outer/alm.hpp>
#if ALPAQA_WITH_OCP
#include <alpaqa/inner/panoc-ocp.hpp>
#endif

namespace alpaqa::params {

template <class T>
inline constexpr bool is_duration = false;
template <class Rep, class Period>
inline constexpr bool is_duration<std::chrono::duration<Rep, Period>> = true;

template <class Duration>
    requires is_duration<Duration>
void set_param_default(Duration &t, const json &j) {
    if (!j.is_string())
        throw invalid_json_param(
            "Invalid value " + to_string(j) + " for type '" +
            demangled_typename(typeid(Duration)) + "' (expected a string)");
    std::string value = j; // keep outside of try block
    try {
        util::parse_duration(t = {}, value);
    } catch (util::invalid_duration_value &e) {
        throw invalid_json_param(
            "Invalid value '" + value + "' for type '" +
            demangled_typename(typeid(Duration)) + "': error at '" +
            std::string(std::string_view(value.data(), e.result.ptr)));
    } catch (util::invalid_duration_units &e) {
        throw invalid_json_param(
            "Invalid units '" + std::string(e.units) + "' for type '" +
            demangled_typename(typeid(Duration)) + "' in '" + value + "'");
    }
}

template <class Duration>
    requires is_duration<Duration>
void get_param_default(const Duration &t, json &s) {
    namespace chr = std::chrono;
    auto dur      = t;
    std::string result;
    if (dur.count() == 0) {
        result = "0";
    } else {
        if (auto d = duration_cast<chr::hours>(dur); d.count() != 0) {
            result += std::to_string(d.count()) + "h";
            dur -= duration_cast<Duration>(d);
        }
        if (auto d = duration_cast<chr::minutes>(dur); d.count() != 0) {
            result += std::to_string(d.count()) + "min";
            dur -= duration_cast<Duration>(d);
        }
        if (auto d = duration_cast<chr::seconds>(dur); d.count() != 0) {
            result += std::to_string(d.count()) + "s";
            dur -= duration_cast<Duration>(d);
        }
        if (auto d = duration_cast<chr::milliseconds>(dur); d.count() != 0) {
            result += std::to_string(d.count()) + "ms";
            dur -= duration_cast<Duration>(d);
        }
        if (auto d = duration_cast<chr::microseconds>(dur); d.count() != 0) {
            result += std::to_string(d.count()) + "µs";
            dur -= duration_cast<Duration>(d);
        }
        if (auto d = duration_cast<chr::nanoseconds>(dur); d.count() != 0) {
            result += std::to_string(d.count()) + "ns";
            dur -= duration_cast<Duration>(d);
        }
    }
    s = std::move(result);
}

template <>
void ALPAQA_EXPORT set_param(alpaqa::vec<config_t> &v, const json &j) {
    if (!j.is_array())
        throw invalid_json_param("Invalid value " + to_string(j) +
                                 " for type '" + demangled_typename(typeid(v)) +
                                 "' (expected an array, but got " +
                                 j.type_name() + ')');
    v.resize(static_cast<length_t<config_t>>(j.size()));
    auto convert = [](const json &j) -> real_t<config_t> {
        try {
            return j;
        } catch (json::exception &e) {
            throw invalid_json_param("Invalid vector element " + to_string(j) +
                                     " (expected a number, but got " +
                                     j.type_name() + "): " + e.what());
        }
    };
    std::ranges::transform(j, v.begin(), convert);
}

template <>
void ALPAQA_EXPORT set_param(vec_from_file<config_t> &v, const json &j) {
    if (j.is_string()) {
        std::string fpath{j};
        std::ifstream f(fpath);
        if (!f)
            throw invalid_json_param("Unable to open file '" + fpath +
                                     "' for type '" +
                                     demangled_typename(typeid(v)));
        try {
            auto r      = alpaqa::csv::read_row_std_vector<real_t<config_t>>(f);
            auto r_size = static_cast<length_t<config_t>>(r.size());
            if (v.expected_size >= 0 && r_size != v.expected_size)
                throw invalid_json_param(
                    "Incorrect size in '" + fpath + "' (expected " +
                    std::to_string(v.expected_size) + ", but got " +
                    std::to_string(r.size()) + ")");
            v.value.emplace(cmvec<config_t>{r.data(), r_size});
        } catch (alpaqa::csv::read_error &e) {
            throw invalid_json_param("Unable to read from file '" + fpath +
                                     "': alpaqa::csv::read_error: " + e.what());
        }
    } else if (j.is_array()) {
        alpaqa::params::set_param(v.value.emplace(), j);
        if (v.expected_size >= 0 && v.value->size() != v.expected_size)
            throw invalid_json_param(
                "Incorrect size in " + to_string(j) + "' (expected " +
                std::to_string(v.expected_size) + ", but got " +
                std::to_string(v.value->size()) + ')');
    } else {
        throw invalid_json_param("Invalid value " + to_string(j) +
                                 " for type '" + demangled_typename(typeid(v)) +
                                 "' (expected string or array, but got " +
                                 j.type_name() + ')');
    }
}

template <>
void ALPAQA_EXPORT set_param(std::monostate &, const nlohmann::json &) {
    throw invalid_json_param("Cannot set value of std::monostate");
}

template <>
void ALPAQA_EXPORT set_param(bool &t, const nlohmann::json &j) {
    if (!j.is_boolean())
        throw invalid_json_param("Invalid value " + to_string(j) +
                                 " for type '" + demangled_typename(typeid(t)) +
                                 "' (expected boolean, but got " +
                                 j.type_name() + ')');
    t = static_cast<bool>(j);
}

template <>
void ALPAQA_EXPORT set_param(std::string &t, const nlohmann::json &j) {
    if (!j.is_string())
        throw invalid_json_param("Invalid value " + to_string(j) +
                                 " for type '" + demangled_typename(typeid(t)) +
                                 "' (expected string, but got " +
                                 j.type_name() + ')');
    t = static_cast<std::string>(j);
}

template <std::integral T>
    requires(!std::same_as<T, bool>)
void set_param_default(T &t, const nlohmann::json &j) {
    if (std::unsigned_integral<T> && !j.is_number_unsigned())
        throw invalid_json_param("Invalid value " + to_string(j) +
                                 " for type '" + demangled_typename(typeid(T)) +
                                 "' (expected unsigned integer, but got " +
                                 j.type_name() + ')');
    if (!j.is_number_integer())
        throw invalid_json_param("Invalid value " + to_string(j) +
                                 " for type '" + demangled_typename(typeid(T)) +
                                 "' (expected integer, but got " +
                                 j.type_name() + ')');
    t = static_cast<T>(j);
}

template <std::floating_point T>
void set_param_default(T &t, const nlohmann::json &j) {
    if (j.is_string()) {
        if (j == "nan") {
            t = std::numeric_limits<T>::quiet_NaN();
        } else if (j == "inf" || j == "+inf") {
            t = std::numeric_limits<T>::infinity();
        } else if (j == "-inf") {
            t = -std::numeric_limits<T>::infinity();
        } else {
            throw invalid_json_param("Invalid value " + to_string(j) +
                                     " for type '" +
                                     demangled_typename(typeid(T)) +
                                     "' (expected number or any of "
                                     "\"nan\", \"inf\", \"+inf\", \"-inf\")");
        }
    } else if (j.is_number()) {
        t = static_cast<T>(j);
    } else {
        throw invalid_json_param("Invalid value " + to_string(j) +
                                 " for type '" + demangled_typename(typeid(T)) +
                                 "' (expected number, but got " +
                                 j.type_name() + ')');
    }
}

template <class T>
    requires(std::integral<T> || std::same_as<T, bool> ||
             std::same_as<T, std::string>)
void get_param_default(const T &t, nlohmann::json &j) {
    j = t;
}

template <std::floating_point T>
void get_param_default(const T &t, nlohmann::json &j) {
    if (std::isnan(t))
        j = "nan";
    else if (t == +std::numeric_limits<T>::infinity())
        j = "inf";
    else if (t == -std::numeric_limits<T>::infinity())
        j = "-inf";
    else
        j = t;
}

#include <alpaqa/params/structs.ipp>

// Because of the new name mangling for concepts (https://reviews.llvm.org/D147655)
// we need this to be an unconstrained function template. The actual constraints
// are in set_param_default, which is not exported. Fully specialized
// instantiations of set_param are still allowed.
template <class T>
void set_param(T &t, const json &j) {
    set_param_default(t, j);
}
template <class T>
void get_param(const T &t, json &j) {
    get_param_default(t, j);
}

template <class... Ts>
void set_param(util::detail::dummy<Ts...> &, const json &) {}
template <class... Ts>
void get_param(const util::detail::dummy<Ts...> &, json &) {}

#define ALPAQA_GET_PARAM_INST(...)                                             \
    template void ALPAQA_EXPORT get_param(                                     \
        const util::possible_alias_t<__VA_ARGS__> &, json &)
#define ALPAQA_GETSET_PARAM_INST(...)                                          \
    template void ALPAQA_EXPORT set_param(                                     \
        util::possible_alias_t<__VA_ARGS__> &, const json &);                  \
    template void ALPAQA_EXPORT get_param(                                     \
        const util::possible_alias_t<__VA_ARGS__> &, json &)

ALPAQA_GET_PARAM_INST(std::string);

ALPAQA_GET_PARAM_INST(bool);

ALPAQA_GETSET_PARAM_INST(float);
ALPAQA_GETSET_PARAM_INST(double, float);
ALPAQA_GETSET_PARAM_INST(long double, double, float);

ALPAQA_GETSET_PARAM_INST(int8_t);
ALPAQA_GETSET_PARAM_INST(uint8_t);
ALPAQA_GETSET_PARAM_INST(int16_t);
ALPAQA_GETSET_PARAM_INST(uint16_t);
ALPAQA_GETSET_PARAM_INST(int32_t);
ALPAQA_GETSET_PARAM_INST(int64_t);
ALPAQA_GETSET_PARAM_INST(uint32_t);
ALPAQA_GETSET_PARAM_INST(uint64_t);

// Here, we would like to instantiate alpaqa::params::set_param for all standard
// integer types, but the issue is that they might not be distinct types:
// For example, on some platforms, int32_t might be a weak alias to int, whereas
// on other platforms, it could be a distinct type.
// To resolve this issue, we use some metaprogramming to ensure distinct
// instantiations with unique dummy types.
#define ALPAQA_GETSET_PARAM_INST_INT(...)                                      \
    ALPAQA_GETSET_PARAM_INST(__VA_ARGS__, int8_t, uint8_t, int16_t, uint16_t,  \
                             int32_t, int64_t, uint32_t, uint64_t)

ALPAQA_GETSET_PARAM_INST_INT(short);
ALPAQA_GETSET_PARAM_INST_INT(int, short);
ALPAQA_GETSET_PARAM_INST_INT(long, int, short);
ALPAQA_GETSET_PARAM_INST_INT(long long, long, int, short);
ALPAQA_GETSET_PARAM_INST_INT(ptrdiff_t, long long, long, int, short);
ALPAQA_GETSET_PARAM_INST_INT(unsigned short);
ALPAQA_GETSET_PARAM_INST_INT(unsigned int, unsigned short);
ALPAQA_GETSET_PARAM_INST_INT(unsigned long, unsigned int, unsigned short);
ALPAQA_GETSET_PARAM_INST_INT(unsigned long long, unsigned long, unsigned int,
                             unsigned short);
ALPAQA_GETSET_PARAM_INST_INT(size_t, unsigned long long, unsigned long,
                             unsigned int, unsigned short);

ALPAQA_GETSET_PARAM_INST(std::chrono::nanoseconds);
ALPAQA_GETSET_PARAM_INST(std::chrono::microseconds);
ALPAQA_GETSET_PARAM_INST(std::chrono::milliseconds);
ALPAQA_GETSET_PARAM_INST(std::chrono::seconds);
ALPAQA_GETSET_PARAM_INST(std::chrono::minutes);
ALPAQA_GETSET_PARAM_INST(std::chrono::hours);

ALPAQA_GETSET_PARAM_INST(PANOCStopCrit);
ALPAQA_GETSET_PARAM_INST(LBFGSStepSize);
ALPAQA_GETSET_PARAM_INST(CBFGSParams<config_t>);
ALPAQA_GETSET_PARAM_INST(LipschitzEstimateParams<config_t>);
ALPAQA_GETSET_PARAM_INST(PANOCParams<config_t>);
ALPAQA_GETSET_PARAM_INST(FISTAParams<config_t>);
ALPAQA_GETSET_PARAM_INST(ZeroFPRParams<config_t>);
ALPAQA_GETSET_PARAM_INST(PANTRParams<config_t>);
ALPAQA_GETSET_PARAM_INST(LBFGSParams<config_t>);
ALPAQA_GETSET_PARAM_INST(AndersonAccelParams<config_t>);
ALPAQA_GETSET_PARAM_INST(LBFGSDirectionParams<config_t>);
ALPAQA_GETSET_PARAM_INST(AndersonDirectionParams<config_t>);
ALPAQA_GETSET_PARAM_INST(StructuredLBFGSDirectionParams<config_t>);
ALPAQA_GETSET_PARAM_INST(NewtonTRDirectionParams<config_t>);
ALPAQA_GETSET_PARAM_INST(SteihaugCGParams<config_t>);
ALPAQA_GETSET_PARAM_INST(StructuredNewtonRegularizationParams<config_t>);
ALPAQA_GETSET_PARAM_INST(StructuredNewtonDirectionParams<config_t>);
ALPAQA_GETSET_PARAM_INST(ConvexNewtonRegularizationParams<config_t>);
ALPAQA_GETSET_PARAM_INST(ConvexNewtonDirectionParams<config_t>);
ALPAQA_GETSET_PARAM_INST(ALMParams<config_t>);
#if ALPAQA_WITH_OCP
ALPAQA_GETSET_PARAM_INST(PANOCOCPParams<config_t>);
#endif

} // namespace alpaqa::params