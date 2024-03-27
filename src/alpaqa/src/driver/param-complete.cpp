#include "param-complete.hpp"

#include <alpaqa/config/config.hpp>
#include <alpaqa/implementation/params/params.tpp>
#include <alpaqa/util/demangled-typename.hpp>

// For parameters
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
// end

#include <cstdio>
#include <iostream>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string_view>
using namespace std::string_view_literals;

USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);

namespace alpaqa::params {

template <class T>
bool is_leaf() {
    return !requires { attribute_table<T, MemberGetter>::table; };
}

/// Catch-all
template <class T>
Result get_members(const MemberGetter &s) {
    auto pfx = std::string_view{s.full_key.begin(), s.key.begin()};
    return {.leaf = true, .prefix = pfx, .members = {}}; // no suggestions
}

/// Struct types
template <class T>
    requires requires { attribute_table<T, MemberGetter>::table; }
Result get_members(const MemberGetter &s) {
    const auto &m         = attribute_table<T, MemberGetter>::table;
    auto [key, remainder] = alpaqa::util::split(s.key, ".");
    auto it               = m.find(key);
    if (it == m.end()) {
        auto pfx = std::string_view{s.full_key.begin(), s.key.begin()};
        if (key.end() != s.key.end() || s.value)
            return {.leaf = false, .prefix = pfx, .members = {}};
        auto members = std::views::transform(m, [](const auto &e) {
            return Result::Member{
                .name   = e.first,
                .doc    = e.second.doc,
                .suffix = e.second.leaf ? '=' : '.',
            };
        });
        return {
            .leaf    = false,
            .prefix  = pfx,
            .members = {members.begin(), members.end()},
        };
    }
    auto recurse = s;
    recurse.key  = remainder;
    return it->second.get(recurse);
}

/// Enum types
template <class T>
    requires requires { enum_table<T, MemberGetter>::table; }
Result get_members(const MemberGetter &s) {
    const auto &m = enum_table<T, MemberGetter>::table;
    auto pfx      = std::string_view{s.full_key.begin(), s.key.begin()};
    auto members  = std::views::transform(m, [](const auto &e) {
        return Result::Member{
             .name   = e.first,
             .doc    = e.second.doc,
             .suffix = std::nullopt,
        };
    });
    return {
        .leaf    = true,
        .prefix  = pfx,
        .members = {members.begin(), members.end()},
    };
}

/// True/false
template <>
Result get_members<bool>(const MemberGetter &s) {
    auto pfx = std::string_view{s.full_key.begin(), s.key.begin()};
    return {
        .leaf    = true,
        .prefix  = pfx,
        .members = {{.name = "true"}, {.name = "false"}},
    };
}

struct Value {};
struct Struct {};

struct RootOpts {
    [[no_unique_address]] Value method, out, sol, x0, mul_g0, mul_x0, num_exp;
    bool extra_stats, show_funcs;
    Struct problem;
};

#include <alpaqa/params/structs.ipp>

PARAMS_TABLE(
    RootOpts,                                                              //
    PARAMS_MEMBER(method, "Solver to use"),                                //
    PARAMS_MEMBER(out, "File to write output to"),                         //
    PARAMS_MEMBER(sol, "Folder to write the solutions and statistics to"), //
    PARAMS_MEMBER(x0, "Initial guess for the solution"),                   //
    PARAMS_MEMBER(mul_g0,
                  "Initial guess for the general constraint multipliers"), //
    PARAMS_MEMBER(mul_x0,
                  "Initial guess for the bound constraint multipliers"),    //
    PARAMS_MEMBER(num_exp, "Number of times to repeat the experiment"),     //
    PARAMS_MEMBER(extra_stats, "Log more per-iteration solver statistics"), //
    PARAMS_MEMBER(show_funcs, "Print the provided problem functions"),      //
    PARAMS_MEMBER(problem, "Options to pass to the problem"),               //
);

PARAMS_TABLE(Struct);

} // namespace alpaqa::params

using alpaqa::params::get_members;
using alpaqa::params::MemberGetter;
using alpaqa::params::Result;

void add_root_opts(std::vector<Result::Member> &v) {
    MemberGetter s{};
    auto r = get_members<alpaqa::params::RootOpts>(s);
    for (auto &e : r.members)
        v.push_back(e);
}

template <class S>
Result get_results_panoc_like(const MemberGetter &s) {
    auto [key, remainder] = alpaqa::util::split(s.key, ".");
    auto recurse          = s;
    recurse.key           = remainder;

    if (key == "alm")
        return get_members<alpaqa::ALMParams<config_t>>(recurse);
    if (key == "solver")
        return get_members<typename S::Params>(recurse);
    if (key == "dir")
        return get_members<typename S::Direction::DirectionParams>(recurse);
    if (key == "accel")
        return get_members<typename S::Direction::AcceleratorParams>(recurse);
    if (key.end() != s.key.end() || s.value) // no remainder && no '.'
        return {.leaf = false, .prefix = "", .members = {}};
    return {
        .leaf    = false,
        .prefix  = "",
        .members = {{.name   = "alm",
                     .doc    = "Options for the augmented Lagrangian method",
                     .suffix = '.'},
                    {.name   = "solver",
                     .doc    = "Options for the inner solver",
                     .suffix = '.'},
                    {.name   = "dir",
                     .doc    = "Options for the direction provider",
                     .suffix = '.'},
                    {.name   = "accel",
                     .doc    = "Options for the accelerator",
                     .suffix = '.'}},
    };
}

template <class S>
Result get_results_fista_like(const MemberGetter &s) {
    auto [key, remainder] = alpaqa::util::split(s.key, ".");
    auto recurse          = s;
    recurse.key           = remainder;

    if (key == "alm")
        return get_members<alpaqa::ALMParams<config_t>>(recurse);
    if (key == "solver")
        return get_members<typename S::Params>(recurse);
    if (key.end() != s.key.end() || s.value)
        return {.leaf = false, .prefix = "", .members = {}};
    return {
        .leaf    = false,
        .prefix  = "",
        .members = {{.name   = "alm",
                     .doc    = "Options for the augmented Lagrangian method",
                     .suffix = '.'},
                    {.name   = "solver",
                     .doc    = "Options for the inner solver",
                     .suffix = '.'}},
    };
}

using func_t = Result(const MemberGetter &);
struct Method {
    // std::function<func_t> func;
    func_t *func;
    std::string_view doc;
};
using dict_t = std::map<std::string_view, Method>;

const dict_t methods{
    // clang-format off
    {"panoc", {get_results_panoc_like<alpaqa::PANOCSolver<alpaqa::LBFGSDirection<config_t>>>, "PANOC + LBFGS solver (default)"}},
    {"panoc.lbfgs", {get_results_panoc_like<alpaqa::PANOCSolver<alpaqa::LBFGSDirection<config_t>>>, "PANOC + LBFGS solver"}},
    {"panoc.struclbfgs", {get_results_panoc_like<alpaqa::PANOCSolver<alpaqa::StructuredLBFGSDirection<config_t>>>, "PANOC + Structured LBFGS solver"}},
    {"panoc.anderson", {get_results_panoc_like<alpaqa::PANOCSolver<alpaqa::AndersonDirection<config_t>>>, "PANOC + Anderson acceleration solver"}},
    {"panoc.convex-newton", {get_results_panoc_like<alpaqa::PANOCSolver<alpaqa::ConvexNewtonDirection<config_t>>>, "PANOC + Newton (for convex problems)"}},
    {"zerofpr", {get_results_panoc_like<alpaqa::ZeroFPRSolver<alpaqa::LBFGSDirection<config_t>>>, "ZeroFPR + LBFGS solver"}},
    {"zerofpr.lbfgs", {get_results_panoc_like<alpaqa::ZeroFPRSolver<alpaqa::LBFGSDirection<config_t>>>, "ZeroFPR + LBFGS solver"}},
    {"zerofpr.struclbfgs", {get_results_panoc_like<alpaqa::ZeroFPRSolver<alpaqa::StructuredLBFGSDirection<config_t>>>, "ZeroFPR + Structured LBFGS solver"}},
    {"zerofpr.anderson", {get_results_panoc_like<alpaqa::ZeroFPRSolver<alpaqa::AndersonDirection<config_t>>>, "ZeroFPR + Anderson acceleration solver"}},
    {"zerofpr.convex-newton", {get_results_panoc_like<alpaqa::PANOCSolver<alpaqa::ConvexNewtonDirection<config_t>>>, "ZeroFPR + Newton (for convex problems)"}},
    {"pantr", {get_results_panoc_like<alpaqa::PANTRSolver<alpaqa::NewtonTRDirection<config_t>>>, "PANTR solver"}},
    {"fista", {get_results_fista_like<alpaqa::FISTASolver<config_t>>, "FISTA solver"}},
    {"ipopt", {alpaqa::params::get_members<void>, "Ipopt solver"}},
    {"qpalm", {alpaqa::params::get_members<void>, "QPALM solver"}},
    // clang-format on
};

Result get_results(std::string_view method, const MemberGetter &s) {
    if (s.key == "method") {
        auto members = std::views::transform(methods, [](const auto &e) {
            return Result::Member{
                .name   = e.first,
                .doc    = e.second.doc,
                .suffix = std::nullopt,
            };
        });
        return {
            .leaf    = true,
            .prefix  = "method",
            .members = {members.begin(), members.end()},
        };
    }

    if (method.empty())
        method = "panoc";
    try {
        const auto &m = methods.at(method);
        auto result   = m.func(s);
        if (result.members.empty())
            result = get_members<alpaqa::params::RootOpts>(s);
        else if (result.prefix.empty())
            add_root_opts(result.members);
        return result;
    } catch (std::out_of_range &) {
        return get_members<alpaqa::params::RootOpts>(s);
    }
}

void print_completion(std::string_view method, std::string_view params) {
    auto [key, value] = alpaqa::util::split(params, "=");
    bool has_value    = key.end() != params.end();
    MemberGetter s{
        .full_key = key,
        .key      = key,
        .value    = has_value ? std::make_optional(value) : std::nullopt,
    };
    auto result = get_results(method, s);
    if (!result.members.empty()) {
        std::cout << "_prefix:" << result.prefix;
        if (!result.prefix.empty() && result.prefix.back() != '.')
            std::cout << (result.leaf ? '=' : '.');
        std::cout << "\n_suffix:";
        if (result.leaf)
            std::cout << ' ';
        std::cout << '\n';
        size_t max_len =
            std::accumulate(result.members.begin(), result.members.end(),
                            size_t{0}, [](size_t acc, const auto &m) {
                                return std::max(acc, m.name.size());
                            });
        for (const auto &member : result.members) {
            auto name = member.name;
            std::string padding(max_len - std::min(max_len, name.size()), ' ');
            std::cout << name;
            if (member.suffix)
                std::cout << *member.suffix;
            std::cout << ':' << name;
            if (member.doc) {
                auto doc = member.doc->empty() ? "(undocumented)" : *member.doc;
                std::cout << padding << " -- " << doc << "    ";
            }
            std::cout << '\n';
        }
    }
}