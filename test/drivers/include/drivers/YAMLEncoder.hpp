#include "panoc-alm/decl/alm.hpp"
#include "panoc-alm/inner/decl/panoc.hpp"
#include <panoc-alm/interop/cutest/CUTEstLoader.hpp>
#include <panoc-alm/util/solverstatus.hpp>

#include <yaml-cpp/emitter.h>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/yaml.h>

#include <sstream>

inline YAML::Emitter &operator<<(YAML::Emitter &out, const pa::vec &v) {
    out << YAML::Flow;
    out << YAML::BeginSeq;
    for (pa::vec::Index i = 0; i < v.size(); ++i)
        out << v[i];
    out << YAML::EndSeq;
    return out;
}

inline YAML::Emitter &operator<<(YAML::Emitter &out,
                                 CUTEstProblem::Report::Status s) {
    return out << enum_name(s);
}

inline YAML::Emitter &operator<<(YAML::Emitter &out,
                                 const CUTEstProblem::Report &r) {
    out << YAML::BeginMap;
    out << YAML::Key << r.name;
    out << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "nvar" << YAML::Value << r.nvar;
    out << YAML::Key << "ncon" << YAML::Value << r.ncon;
    out << YAML::Key << "status" << YAML::Value << r.status;
    out << YAML::Key << "calls" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "objective" << YAML::Value << r.calls.objective
        << YAML::Key << "objective_grad" << YAML::Value
        << r.calls.objective_grad //
        << YAML::Key << "objective_hess" << YAML::Value
        << r.calls.objective_hess //
        << YAML::Key << "hessian_times_vector" << YAML::Value
        << r.calls.hessian_times_vector;
    if (r.ncon > 0)
        out << YAML::Key << "constraints" << YAML::Value
            << r.calls.constraints //
            << YAML::Key << "constraints_grad" << YAML::Value
            << r.calls.constraints_grad //
            << YAML::Key << "constraints_hess" << YAML::Value
            << r.calls.constraints_hess;
    out << YAML::EndMap;
    out << YAML::Key << "time_setup" << YAML::Value << r.time_setup //
        << YAML::Key << "time" << YAML::Value << r.time;
    out << YAML::EndMap << YAML::EndMap;
    return out;
}

inline YAML::Emitter &operator<<(YAML::Emitter &out, pa::EvalCounter ctr) {
    out << YAML::BeginMap;
    out << YAML::Key << "f" << YAML::Value << ctr.f;
    out << YAML::Key << "grad_f" << YAML::Value << ctr.grad_f;
    out << YAML::Key << "g" << YAML::Value << ctr.g;
    out << YAML::Key << "grad_g_prod" << YAML::Value << ctr.grad_g_prod;
    out << YAML::Key << "grad_gi" << YAML::Value << ctr.grad_gi;
    out << YAML::Key << "hess_L" << YAML::Value << ctr.hess_L;
    out << YAML::EndMap;
    return out;
}

inline YAML::Emitter &operator<<(YAML::Emitter &out, pa::SolverStatus s) {
    return out << enum_name(s);
}

inline YAML::Emitter &operator<<(YAML::Emitter &out, const pa::PANOCParams &p) {
    out << YAML::BeginMap;
    out << YAML::Key << "Lipschitz" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "ε" << YAML::Value << p.Lipschitz.ε;
    out << YAML::Key << "δ" << YAML::Value << p.Lipschitz.δ;
    out << YAML::Key << "Lγ_factor" << YAML::Value << p.Lipschitz.Lγ_factor;
    out << YAML::EndMap;
    out << YAML::Key << "lbfgs_mem" << YAML::Value << p.lbfgs_mem;
    out << YAML::Key << "max_iter" << YAML::Value << p.max_iter;
    out << YAML::Key << "max_time" << YAML::Value << p.max_time.count();
    out << YAML::Key << "τ_min" << YAML::Value << p.τ_min;
    out << YAML::Key << "update_lipschitz_in_linesearch" << YAML::Value
        << p.update_lipschitz_in_linesearch;
    out << YAML::Key << "alternative_linesearch_cond" << YAML::Value
        << p.alternative_linesearch_cond;
    out << YAML::EndMap;
    return out;
}

inline YAML::Emitter &operator<<(YAML::Emitter &out, const pa::ALMParams &p) {
    out << YAML::BeginMap;
    out << YAML::Key << "ε" << YAML::Value << p.ε;
    out << YAML::Key << "δ" << YAML::Value << p.δ;
    out << YAML::Key << "Δ" << YAML::Value << p.Δ;
    out << YAML::Key << "Δ_lower" << YAML::Value << p.Δ_lower;
    out << YAML::Key << "Σ₀" << YAML::Value << p.Σ₀;
    out << YAML::Key << "σ₀" << YAML::Value << p.σ₀;
    out << YAML::Key << "Σ₀_lower" << YAML::Value << p.Σ₀_lower;
    out << YAML::Key << "ε₀" << YAML::Value << p.ε₀;
    out << YAML::Key << "ε₀_increase" << YAML::Value << p.ε₀_increase;
    out << YAML::Key << "ρ" << YAML::Value << p.ρ;
    out << YAML::Key << "ρ_increase" << YAML::Value << p.ρ_increase;
    out << YAML::Key << "θ" << YAML::Value << p.θ;
    out << YAML::Key << "M" << YAML::Value << p.M;
    out << YAML::Key << "Σₘₐₓ" << YAML::Value << p.Σₘₐₓ;
    out << YAML::Key << "max_iter" << YAML::Value << p.max_iter;
    out << YAML::Key << "max_time" << YAML::Value << p.max_time.count();
    out << YAML::Key << "max_num_initial_retries" << YAML::Value
        << p.max_num_initial_retries;
    out << YAML::Key << "max_num_retries" << YAML::Value << p.max_num_retries;
    out << YAML::Key << "max_total_num_retries" << YAML::Value
        << p.max_total_num_retries;
    out << YAML::Key << "print_interval" << YAML::Value << p.print_interval;
    out << YAML::Key << "preconditioning" << YAML::Value << p.preconditioning;
    out << YAML::EndMap;
    return out;
}