#include <alpaqa/implementation/params/params.tpp>

#include <alpaqa/util/duration-parse.hpp>
#include <alpaqa/util/io/csv.hpp>
#include <alpaqa/util/possible-alias.hpp>
#include <fstream>

#include "from_chars-compat.ipp"

#include <alpaqa/inner/directions/panoc/anderson.hpp>
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

template <>
void ALPAQA_EXPORT set_param(bool &b, ParamString s) {
    assert_key_empty<bool>(s);
    if (s.value == "0" || s.value == "false")
        b = false;
    else if (s.value == "1" || s.value == "true")
        b = true;
    else
        throw std::invalid_argument(
            "Invalid value '" + std::string(s.value) +
            "' for type 'bool' in '" + std::string(s.full_key) +
            "',\n  "
            "possible values are: '0', '1', 'true', 'false'");
}

template <>
void ALPAQA_EXPORT set_param(std::string_view &v, ParamString s) {
    assert_key_empty<std::string_view>(s);
    v = s.value;
}

template <>
void ALPAQA_EXPORT set_param(std::string &v, ParamString s) {
    assert_key_empty<std::string>(s);
    v = s.value;
}

template <class T>
    requires((std::floating_point<T> || std::integral<T>) &&
             !std::is_enum_v<T> && !std::is_same_v<T, bool>)
void set_param(T &f, ParamString s) {
    assert_key_empty<T>(s);
    const auto *val_end = s.value.data() + s.value.size();
    const auto *ptr     = set_param_float_int(f, s);
    if (ptr != val_end)
        throw std::invalid_argument("Invalid suffix '" +
                                    std::string(ptr, val_end) + "' for type '" +
                                    demangled_typename(typeid(T)) + "' in '" +
                                    std::string(s.full_key) + "'");
}

#ifdef ALPAQA_WITH_QUAD_PRECISION
template <>
void ALPAQA_EXPORT set_param(__float128 &f, ParamString s) {
    long double ld;
    set_param(ld, s);
    f = static_cast<__float128>(ld);
}
#endif

template <>
void ALPAQA_EXPORT set_param(alpaqa::vec<config_t> &v, ParamString s) {
    v.resize(std::count(s.value.begin(), s.value.end(), ',') + 1);
    std::string_view value, remainder = s.value;
    for (auto &e : v) {
        std::tie(value, remainder) = split_key(remainder, ',');
        set_param(e, {.full_key = s.full_key, .key = "", .value = value});
    }
}

template <>
void ALPAQA_EXPORT set_param(vec_from_file<config_t> &v, ParamString s) {
    assert_key_empty<vec_from_file<config_t>>(s);
    if (s.value.starts_with('@')) {
        std::string fpath{s.value.substr(1)};
        std::ifstream f(fpath);
        if (!f)
            throw std::invalid_argument("Unable to open file '" + fpath +
                                        "' in '" + std::string(s.full_key) +
                                        '\'');
        try {
            auto r      = alpaqa::csv::read_row_std_vector<real_t<config_t>>(f);
            auto r_size = static_cast<length_t<config_t>>(r.size());
            if (v.expected_size >= 0 && r_size != v.expected_size)
                throw std::invalid_argument(
                    "Incorrect size in '" + std::string(s.full_key) +
                    "' (got " + std::to_string(r.size()) + ", expected " +
                    std::to_string(v.expected_size) + ')');
            v.value.emplace(cmvec<config_t>{r.data(), r_size});
        } catch (alpaqa::csv::read_error &e) {
            throw std::invalid_argument(
                "Unable to read from file '" + fpath + "' in '" +
                std::string(s.full_key) +
                "': alpaqa::csv::read_error: " + e.what());
        }
    } else {
        alpaqa::params::set_param(v.value.emplace(), s);
        if (v.expected_size >= 0 && v.value->size() != v.expected_size)
            throw std::invalid_argument(
                "Incorrect size in '" + std::string(s.full_key) + "' (got " +
                std::to_string(v.value->size()) + ", expected " +
                std::to_string(v.expected_size) + ')');
    }
}

template <class T>
inline constexpr bool is_duration = false;
template <class Rep, class Period>
inline constexpr bool is_duration<std::chrono::duration<Rep, Period>> = true;

template <class Duration>
    requires is_duration<Duration>
void set_param(Duration &t, ParamString s) {
    assert_key_empty<Duration>(s);
    try {
        util::parse_duration(t = {}, s.value);
    } catch (util::invalid_duration_value &e) {
        throw invalid_param(
            "Invalid value '" + std::string(s.value) + "' for type '" +
            demangled_typename(typeid(Duration)) + "': error at '" +
            std::string(std::string_view(s.value.data(), e.result.ptr)));
    } catch (util::invalid_duration_units &e) {
        throw invalid_param("Invalid units '" + std::string(e.units) +
                            "' for type '" +
                            demangled_typename(typeid(Duration)) + "' in '" +
                            std::string(s.value) + "'");
    }
}

template <>
void ALPAQA_EXPORT set_param(LBFGSStepSize &t, ParamString s) {
    if (s.value == "BasedOnExternalStepSize")
        t = LBFGSStepSize::BasedOnExternalStepSize;
    else if (s.value == "BasedOnCurvature")
        t = LBFGSStepSize::BasedOnCurvature;
    else
        throw std::invalid_argument("Invalid value '" + std::string(s.value) +
                                    "' for type 'LBFGSStepSize' in '" +
                                    std::string(s.full_key) + "'");
}

template <>
void ALPAQA_EXPORT set_param(PANOCStopCrit &t, ParamString s) {
    if (s.value == "ApproxKKT")
        t = PANOCStopCrit::ApproxKKT;
    else if (s.value == "ApproxKKT2")
        t = PANOCStopCrit::ApproxKKT2;
    else if (s.value == "ProjGradNorm")
        t = PANOCStopCrit::ProjGradNorm;
    else if (s.value == "ProjGradNorm2")
        t = PANOCStopCrit::ProjGradNorm2;
    else if (s.value == "ProjGradUnitNorm")
        t = PANOCStopCrit::ProjGradUnitNorm;
    else if (s.value == "ProjGradUnitNorm2")
        t = PANOCStopCrit::ProjGradUnitNorm2;
    else if (s.value == "FPRNorm")
        t = PANOCStopCrit::FPRNorm;
    else if (s.value == "FPRNorm2")
        t = PANOCStopCrit::FPRNorm2;
    else if (s.value == "Ipopt")
        t = PANOCStopCrit::Ipopt;
    else if (s.value == "LBFGSBpp")
        t = PANOCStopCrit::LBFGSBpp;
    else
        throw std::invalid_argument("Invalid value '" + std::string(s.value) +
                                    "' for type 'PANOCStopCrit' in '" +
                                    std::string(s.full_key) + "'");
}

#include <alpaqa/params/structs.ipp>

template <class... Ts>
void set_param(util::detail::dummy<Ts...> &, ParamString) {}

#define ALPAQA_SET_PARAM_INST(...)                                             \
    template void ALPAQA_EXPORT set_param(                                     \
        util::possible_alias_t<__VA_ARGS__> &, ParamString)

ALPAQA_SET_PARAM_INST(float);
ALPAQA_SET_PARAM_INST(double, float);
ALPAQA_SET_PARAM_INST(long double, double, float);

ALPAQA_SET_PARAM_INST(int8_t);
ALPAQA_SET_PARAM_INST(uint8_t);
ALPAQA_SET_PARAM_INST(int16_t);
ALPAQA_SET_PARAM_INST(uint16_t);
ALPAQA_SET_PARAM_INST(int32_t);
ALPAQA_SET_PARAM_INST(int64_t);
ALPAQA_SET_PARAM_INST(uint32_t);
ALPAQA_SET_PARAM_INST(uint64_t);

// Here, we would like to instantiate alpaqa::params::set_param for all standard
// integer types, but the issue is that they might not be distinct types:
// For example, on some platforms, int32_t might be a weak alias to int, whereas
// on other platforms, it could be a distinct type.
// To resolve this issue, we use some metaprogramming to ensure distinct
// instantiations with unique dummy types.
#define ALPAQA_SET_PARAM_INST_INT(...)                                         \
    ALPAQA_SET_PARAM_INST(__VA_ARGS__, int8_t, uint8_t, int16_t, uint16_t,     \
                          int32_t, int64_t, uint32_t, uint64_t)

ALPAQA_SET_PARAM_INST_INT(short);
ALPAQA_SET_PARAM_INST_INT(int, short);
ALPAQA_SET_PARAM_INST_INT(long, int, short);
ALPAQA_SET_PARAM_INST_INT(long long, long, int, short);
ALPAQA_SET_PARAM_INST_INT(ptrdiff_t, long long, long, int, short);
ALPAQA_SET_PARAM_INST_INT(unsigned short);
ALPAQA_SET_PARAM_INST_INT(unsigned int, unsigned short);
ALPAQA_SET_PARAM_INST_INT(unsigned long, unsigned int, unsigned short);
ALPAQA_SET_PARAM_INST_INT(unsigned long long, unsigned long, unsigned int,
                          unsigned short);
ALPAQA_SET_PARAM_INST_INT(size_t, unsigned long long, unsigned long,
                          unsigned int, unsigned short);

ALPAQA_SET_PARAM_INST(std::chrono::nanoseconds);
ALPAQA_SET_PARAM_INST(std::chrono::microseconds);
ALPAQA_SET_PARAM_INST(std::chrono::milliseconds);
ALPAQA_SET_PARAM_INST(std::chrono::seconds);
ALPAQA_SET_PARAM_INST(std::chrono::minutes);
ALPAQA_SET_PARAM_INST(std::chrono::hours);

ALPAQA_SET_PARAM_INST(PANOCParams<config_t>);
ALPAQA_SET_PARAM_INST(FISTAParams<config_t>);
ALPAQA_SET_PARAM_INST(ZeroFPRParams<config_t>);
ALPAQA_SET_PARAM_INST(PANTRParams<config_t>);
ALPAQA_SET_PARAM_INST(LBFGSParams<config_t>);
ALPAQA_SET_PARAM_INST(AndersonAccelParams<config_t>);
ALPAQA_SET_PARAM_INST(LBFGSDirectionParams<config_t>);
ALPAQA_SET_PARAM_INST(AndersonDirectionParams<config_t>);
ALPAQA_SET_PARAM_INST(StructuredLBFGSDirectionParams<config_t>);
ALPAQA_SET_PARAM_INST(NewtonTRDirectionParams<config_t>);
ALPAQA_SET_PARAM_INST(SteihaugCGParams<config_t>);
ALPAQA_SET_PARAM_INST(StructuredNewtonRegularizationParams<config_t>);
ALPAQA_SET_PARAM_INST(StructuredNewtonDirectionParams<config_t>);
ALPAQA_SET_PARAM_INST(ALMParams<config_t>);
#if ALPAQA_WITH_OCP
ALPAQA_SET_PARAM_INST(PANOCOCPParams<config_t>);
#endif

} // namespace alpaqa::params
