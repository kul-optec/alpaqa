#pragma once

#include <alpaqa/casadi/CasADiFunctionWrapper.hpp>
#include <alpaqa/casadi/CasADiProblem.hpp>
#include <alpaqa/casadi/casadi-namespace.hpp>
#include <alpaqa/util/io/csv.hpp>
#include <alpaqa/util/not-implemented.hpp>
#include "CasADiLoader-util.hpp"
#include <tuple>

#if ALPAQA_WITH_EXTERNAL_CASADI
#include <casadi/core/external.hpp>
#endif

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>

namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE {

namespace fs = std::filesystem;

namespace casadi_loader {

template <Config Conf>
struct CasADiFunctionsWithParam {
    USING_ALPAQA_CONFIG(Conf);
    length_t n, m, p;
    CasADiFunctionEvaluator<Conf, 2, 1> f;
    CasADiFunctionEvaluator<Conf, 2, 2> f_grad_f;
    std::optional<CasADiFunctionEvaluator<Conf, 2, 1>> g = std::nullopt;
    std::optional<CasADiFunctionEvaluator<Conf, 3, 1>> grad_g_prod =
        std::nullopt;
    std::optional<CasADiFunctionEvaluator<Conf, 2, 1>> jac_g  = std::nullopt;
    std::optional<CasADiFunctionEvaluator<Conf, 3, 1>> grad_L = std::nullopt;
    std::optional<CasADiFunctionEvaluator<Conf, 5, 1>> hess_L_prod =
        std::nullopt;
    std::optional<CasADiFunctionEvaluator<Conf, 4, 1>> hess_L   = std::nullopt;
    std::optional<CasADiFunctionEvaluator<Conf, 6, 2>> ψ        = std::nullopt;
    std::optional<CasADiFunctionEvaluator<Conf, 6, 2>> ψ_grad_ψ = std::nullopt;
    std::optional<CasADiFunctionEvaluator<Conf, 8, 1>> hess_ψ_prod =
        std::nullopt;
    std::optional<CasADiFunctionEvaluator<Conf, 7, 1>> hess_ψ = std::nullopt;

    template <class Loader>
        requires requires(Loader &&loader, const char *name) {
            { loader(name) } -> std::same_as<casadi::Function>;
            { loader.format_name(name) } -> std::same_as<std::string>;
        }
    static std::unique_ptr<CasADiFunctionsWithParam> load(Loader &&loader) {
        length_t n = 0, m = 0, p = 0;
        auto load_g =
            [&]() -> std::optional<CasADiFunctionEvaluator<Conf, 2, 1>> {
            auto gfun = loader("g");
            using namespace std::literals::string_literals;
            if (gfun.n_in() != 2)
                throw invalid_argument_dimensions(
                    "Invalid number of input arguments: got "s +
                    std::to_string(gfun.n_in()) + ", should be 2.");
            if (gfun.n_out() > 1)
                throw invalid_argument_dimensions(
                    "Invalid number of output arguments: got "s +
                    std::to_string(gfun.n_in()) + ", should be 0 or 1.");
            if (gfun.size2_in(0) != 1)
                throw invalid_argument_dimensions(
                    "First input argument should be a column vector.");
            if (gfun.size2_in(1) != 1)
                throw invalid_argument_dimensions(
                    "Second input argument should be a column vector.");
            if (gfun.n_out() == 1 && gfun.size2_out(0) != 1)
                throw invalid_argument_dimensions(
                    "First output argument should be a column vector.");
            n = static_cast<length_t>(gfun.size1_in(0));
            if (gfun.n_out() == 1)
                m = static_cast<length_t>(gfun.size1_out(0));
            p = static_cast<length_t>(gfun.size1_in(1));
            if (gfun.n_out() == 0) {
                if (m != 0)
                    throw invalid_argument_dimensions(
                        "Function g has no outputs but m != 0");
                return std::nullopt;
            }
            CasADiFunctionEvaluator<Conf, 2, 1> g{std::move(gfun)};
            g.validate_dimensions({dim(n, 1), dim(p, 1)}, {dim(m, 1)});
            return std::make_optional(std::move(g));
        };

        auto g = wrap_load(loader, "g", load_g);

        return std::make_unique<CasADiFunctionsWithParam>(
            CasADiFunctionsWithParam{
                .n = n,
                .m = m,
                .p = p,
                .f = wrapped_load<CasADiFunctionEvaluator<Conf, 2, 1>>(
                    loader, "f", dims(n, p), dims(1)),
                .f_grad_f = wrapped_load<CasADiFunctionEvaluator<Conf, 2, 2>>(
                    loader, "f_grad_f", dims(n, p), dims(1, n)),
                .g           = std::move(g),
                .grad_g_prod = try_load<CasADiFunctionEvaluator<Conf, 3, 1>>(
                    loader, "grad_g_prod", dims(n, p, m), dims(n)),
                .jac_g = try_load<CasADiFunctionEvaluator<Conf, 2, 1>>(
                    loader, "jacobian_g", dims(n, p), dims(dim(m, n))),
                .grad_L = try_load<CasADiFunctionEvaluator<Conf, 3, 1>>(
                    loader, "grad_L", dims(n, p, m), dims(n)),
                .hess_L_prod = try_load<CasADiFunctionEvaluator<Conf, 5, 1>>(
                    loader, "hess_L_prod", dims(n, p, m, 1, n), dims(n)),
                .hess_L = try_load<CasADiFunctionEvaluator<Conf, 4, 1>>(
                    loader, "hess_L", dims(n, p, m, 1), dims(dim(n, n))),
                .ψ = try_load<CasADiFunctionEvaluator<Conf, 6, 2>>(
                    loader, "psi", dims(n, p, m, m, m, m), dims(1, m)),
                .ψ_grad_ψ = try_load<CasADiFunctionEvaluator<Conf, 6, 2>>(
                    loader, "psi_grad_psi", dims(n, p, m, m, m, m), dims(1, n)),
                .hess_ψ_prod = try_load<CasADiFunctionEvaluator<Conf, 8, 1>>(
                    loader, "hess_psi_prod", dims(n, p, m, m, 1, m, m, n),
                    dims(n)),
                .hess_ψ = try_load<CasADiFunctionEvaluator<Conf, 7, 1>>(
                    loader, "hess_psi", dims(n, p, m, m, 1, m, m),
                    dims(dim(n, n))),
            });
    }
};

} // namespace casadi_loader

namespace detail {

template <Config Conf>
auto casadi_to_index(casadi_int i) -> index_t<Conf> {
    return static_cast<index_t<Conf>>(i);
}
} // namespace detail

template <Config Conf>
CasADiProblem<Conf>::CasADiProblem(const std::string &filename)
    : BoxConstrProblem<Conf>{0, 0} {

    struct {
        const std::string &filename;
        auto operator()(const std::string &name) const {
            return casadi::external(name, filename);
        }
        auto format_name(const std::string &name) const {
            return filename + ':' + name;
        }
    } loader{filename};
    impl = casadi_loader::CasADiFunctionsWithParam<Conf>::load(loader);

    this->n     = impl->n;
    this->m     = impl->m;
    this->param = vec::Constant(impl->p, alpaqa::NaN<Conf>);
    this->C     = Box<config_t>{impl->n};
    this->D     = Box<config_t>{impl->m};

    auto data_filepath = fs::path{filename}.replace_extension("csv");
    if (fs::exists(data_filepath))
        load_numerical_data(data_filepath);
}

template <Config Conf>
CasADiProblem<Conf>::CasADiProblem(const SerializedCasADiFunctions &functions)
    : BoxConstrProblem<Conf>{0, 0} {
#if ALPAQA_WITH_EXTERNAL_CASADI
    struct {
        const SerializedCasADiFunctions &functions;
        auto operator()(const std::string &name) const {
            return casadi::Function::deserialize(functions.functions.at(name));
        }
        auto format_name(const std::string &name) const {
            return "SerializedCasADiFunctions['" + name + "']";
        }
    } loader{functions};
    impl = casadi_loader::CasADiFunctionsWithParam<Conf>::load(loader);

    this->n     = impl->n;
    this->m     = impl->m;
    this->param = vec::Constant(impl->p, alpaqa::NaN<Conf>);
    this->C     = Box<config_t>{impl->n};
    this->D     = Box<config_t>{impl->m};
#else
    std::ignore = functions;
    throw std::runtime_error(
        "This version of alpaqa was compiled without the CasADi C++ library");
#endif
}

template <Config Conf>
CasADiProblem<Conf>::CasADiProblem(const CasADiFunctions &functions)
    : BoxConstrProblem<Conf>{0, 0} {

    struct {
        const CasADiFunctions &functions;
        auto operator()(const std::string &name) const {
            return functions.functions.at(name);
        }
        auto format_name(const std::string &name) const {
            return "CasADiFunctions['" + name + "']";
        }
    } loader{functions};
    impl = casadi_loader::CasADiFunctionsWithParam<Conf>::load(loader);

    this->n     = impl->n;
    this->m     = impl->m;
    this->param = vec::Constant(impl->p, alpaqa::NaN<Conf>);
    this->C     = Box<config_t>{impl->n};
    this->D     = Box<config_t>{impl->m};
}

template <Config Conf>
void CasADiProblem<Conf>::load_numerical_data(
    const std::filesystem::path &filepath, char sep) {
    // Open data file
    std::ifstream data_file{filepath};
    if (!data_file)
        throw std::runtime_error("Unable to open data file \"" +
                                 filepath.string() + '"');

    // Helper function for reading single line of (float) data
    index_t line        = 0;
    auto wrap_data_load = [&](std::string_view name, auto &v, bool fixed_size) {
        try {
            ++line;
            if (data_file.peek() == '\n') // Ignore empty lines
                return static_cast<void>(data_file.get());
            if (fixed_size) {
                csv::read_row(data_file, v, sep);
            } else { // Dynamic size
                auto s = csv::read_row_std_vector<real_t>(data_file, sep);
                v      = cmvec{s.data(), static_cast<index_t>(s.size())};
            }
        } catch (csv::read_error &e) {
            // Transform any errors in something more readable
            throw std::runtime_error("Unable to read " + std::string(name) +
                                     " from data file \"" + filepath.string() +
                                     ':' + std::to_string(line) +
                                     "\": " + e.what());
        }
    };
    // Helper function for reading a single value
    auto read_single = [&](std::string_view name, auto &v) {
        data_file >> v;
        if (!data_file)
            throw std::runtime_error("Unable to read " + std::string(name) +
                                     " from data file \"" + filepath.string() +
                                     ':' + std::to_string(line) + '"');
    };
    // Read the bounds, parameter value, and regularization
    wrap_data_load("C.lowerbound", this->C.lowerbound, true);
    wrap_data_load("C.upperbound", this->C.upperbound, true);
    wrap_data_load("D.lowerbound", this->D.lowerbound, true);
    wrap_data_load("D.upperbound", this->D.upperbound, true);
    wrap_data_load("param", this->param, true);
    wrap_data_load("l1_reg", this->l1_reg, false);
    // Penalty/ALM split is a single integer
    read_single("penalty_alm_split", this->penalty_alm_split);
    // Name is a string
    data_file >> name;
}

template <Config Conf>
CasADiProblem<Conf>::CasADiProblem(const CasADiProblem &) = default;
template <Config Conf>
CasADiProblem<Conf> &
CasADiProblem<Conf>::operator=(const CasADiProblem &) = default;
template <Config Conf>
CasADiProblem<Conf>::CasADiProblem(CasADiProblem &&) noexcept = default;
template <Config Conf>
CasADiProblem<Conf> &
CasADiProblem<Conf>::operator=(CasADiProblem &&) noexcept = default;

template <Config Conf>
CasADiProblem<Conf>::~CasADiProblem() = default;

template <Config Conf>
auto CasADiProblem<Conf>::eval_f(crvec x) const -> real_t {
    real_t f;
    impl->f({x.data(), param.data()}, {&f});
    return f;
}

template <Config Conf>
void CasADiProblem<Conf>::eval_grad_f(crvec x, rvec grad_fx) const {
    real_t f;
    impl->f_grad_f({x.data(), param.data()}, {&f, grad_fx.data()});
}

template <Config Conf>
auto CasADiProblem<Conf>::eval_f_grad_f(crvec x, rvec grad_fx) const -> real_t {
    real_t f;
    impl->f_grad_f({x.data(), param.data()}, {&f, grad_fx.data()});
    return f;
}

template <Config Conf>
void CasADiProblem<Conf>::eval_g(crvec x, rvec g) const {
    if (impl->m == 0)
        return;
    if (impl->g)
        (*impl->g)({x.data(), param.data()}, {g.data()});
    else
        throw not_implemented_error("CasADiProblem::eval_g");
}

template <Config Conf>
void CasADiProblem<Conf>::eval_grad_g_prod(crvec x, crvec y, rvec gxy) const {
    if (impl->m == 0) {
        gxy.setZero();
        return;
    }
    if (impl->grad_g_prod)
        (*impl->grad_g_prod)({x.data(), param.data(), y.data()}, {gxy.data()});
    else
        throw not_implemented_error("CasADiProblem::eval_grad_g_prod"); // TODO
}

template <Config Conf>
void CasADiProblem<Conf>::eval_grad_ψ(crvec x, crvec y, crvec Σ, rvec grad_ψ,
                                      rvec work_n, rvec work_m) const {
#if 0
    impl->grad_ψ({x.data(), param.data(), y.data(), Σ.data(),
                  this->D.lowerbound.data(), this->D.upperbound.data()},
                 {grad_ψ.data()});
#else
    // This seems to be faster than having a specialized function. Possibly
    // cache-related?
    eval_ψ_grad_ψ(x, y, Σ, grad_ψ, work_n, work_m);
#endif
}

template <Config Conf>
typename CasADiProblem<Conf>::real_t
CasADiProblem<Conf>::eval_ψ_grad_ψ(crvec x, crvec y, crvec Σ, rvec grad_ψ, rvec,
                                   rvec) const {
    if (!impl->ψ_grad_ψ)
        throw std::logic_error("CasADiProblem::eval_ψ_grad_ψ");
    real_t ψ;
    (*impl->ψ_grad_ψ)({x.data(), param.data(), y.data(), Σ.data(),
                       this->D.lowerbound.data(), this->D.upperbound.data()},
                      {&ψ, grad_ψ.data()});
    return ψ;
}

template <Config Conf>
void CasADiProblem<Conf>::eval_grad_L(crvec x, crvec y, rvec grad_L,
                                      rvec) const {
    if (!impl->grad_L)
        throw std::logic_error("CasADiProblem::eval_grad_L");
    (*impl->grad_L)({x.data(), param.data(), y.data()}, {grad_L.data()});
}

template <Config Conf>
typename CasADiProblem<Conf>::real_t
CasADiProblem<Conf>::eval_ψ(crvec x, crvec y, crvec Σ, rvec ŷ) const {
    if (!impl->ψ)
        throw std::logic_error("CasADiProblem::eval_ψ");
    real_t ψ;
    (*impl->ψ)({x.data(), param.data(), y.data(), Σ.data(),
                this->D.lowerbound.data(), this->D.upperbound.data()},
               {&ψ, ŷ.data()});
    return ψ;
}

template <Config Conf>
void CasADiProblem<Conf>::eval_grad_gi(crvec, index_t, rvec) const {
    throw not_implemented_error("CasADiProblem::eval_grad_gi"); // TODO
}

template <Config Conf>
Sparsity<Conf> convert_csc(const auto &sp, sparsity::Symmetry symmetry) {
    USING_ALPAQA_CONFIG(Conf);
    using SparseCSC = sparsity::SparseCSC<Conf, casadi_int>;
    using map_t     = typename SparseCSC::index_vector_map_t;
    return SparseCSC{
        .rows      = static_cast<index_t>(sp.size1()),
        .cols      = static_cast<index_t>(sp.size2()),
        .symmetry  = symmetry,
        .inner_idx = map_t{sp.row(), static_cast<index_t>(sp.nnz())},
        .outer_ptr = map_t{sp.colind(), static_cast<index_t>(sp.size2()) + 1},
        .order     = SparseCSC::SortedRows,
    };
}

template <Config Conf>
auto CasADiProblem<Conf>::get_jac_g_sparsity() const -> Sparsity {
    sparsity::Dense<config_t> dense{
        .rows     = this->m,
        .cols     = this->n,
        .symmetry = sparsity::Symmetry::Unsymmetric,
    };
    if (!impl->jac_g.has_value())
        return dense;
    const auto &sp = impl->jac_g->fun.sparsity_out(0); // Reference!
    return sp.is_dense()
               ? Sparsity{dense}
               : convert_csc<config_t>(sp, sparsity::Symmetry::Unsymmetric);
}

template <Config Conf>
void CasADiProblem<Conf>::eval_jac_g(crvec x, rvec J_values) const {
    if (!impl->jac_g)
        throw std::logic_error("CasADiProblem::eval_jac_g");
    (*impl->jac_g)({x.data(), param.data()}, {J_values.data()});
}

template <Config Conf>
void CasADiProblem<Conf>::eval_hess_L_prod(crvec x, crvec y, real_t scale,
                                           crvec v, rvec Hv) const {
    if (!impl->hess_L_prod)
        throw std::logic_error("CasADiProblem::eval_ψ");
    (*impl->hess_L_prod)({x.data(), param.data(), y.data(), &scale, v.data()},
                         {Hv.data()});
}

template <Config Conf>
auto CasADiProblem<Conf>::get_hess_L_sparsity() const -> Sparsity {
    sparsity::Dense<config_t> dense{
        .rows     = this->n,
        .cols     = this->n,
        .symmetry = sparsity::Symmetry::Upper,
    };
    if (!impl->hess_L.has_value())
        return dense;
    const auto &sp = impl->hess_L->fun.sparsity_out(0); // Reference!
    return sp.is_dense() ? Sparsity{dense}
                         : convert_csc<config_t>(sp, sparsity::Symmetry::Upper);
}

template <Config Conf>
void CasADiProblem<Conf>::eval_hess_L(crvec x, crvec y, real_t scale,
                                      rvec H_values) const {
    if (!impl->hess_L)
        throw std::logic_error("CasADiProblem::eval_hess_L");
    (*impl->hess_L)({x.data(), param.data(), y.data(), &scale},
                    {H_values.data()});
}

template <Config Conf>
void CasADiProblem<Conf>::eval_hess_ψ_prod(crvec x, crvec y, crvec Σ,
                                           real_t scale, crvec v,
                                           rvec Hv) const {
    if (!impl->hess_ψ_prod)
        throw std::logic_error("CasADiProblem::eval_hess_ψ_prod");
    (*impl->hess_ψ_prod)({x.data(), param.data(), y.data(), Σ.data(), &scale,
                          this->D.lowerbound.data(), this->D.upperbound.data(),
                          v.data()},
                         {Hv.data()});
}

template <Config Conf>
auto CasADiProblem<Conf>::get_hess_ψ_sparsity() const -> Sparsity {
    sparsity::Dense<config_t> dense{
        .rows     = this->n,
        .cols     = this->n,
        .symmetry = sparsity::Symmetry::Upper,
    };
    if (!impl->hess_ψ.has_value())
        return dense;
    const auto &sp = impl->hess_ψ->fun.sparsity_out(0); // Reference!
    return sp.is_dense() ? Sparsity{dense}
                         : convert_csc<config_t>(sp, sparsity::Symmetry::Upper);
}

template <Config Conf>
void CasADiProblem<Conf>::eval_hess_ψ(crvec x, crvec y, crvec Σ, real_t scale,
                                      rvec H_values) const {
    if (!impl->hess_ψ)
        throw std::logic_error("CasADiProblem::eval_hess_ψ");
    (*impl->hess_ψ)({x.data(), param.data(), y.data(), Σ.data(), &scale,
                     this->D.lowerbound.data(), this->D.upperbound.data()},
                    {H_values.data()});
}

template <Config Conf>
bool CasADiProblem<Conf>::provides_eval_grad_gi() const {
    return false; // TODO
}
template <Config Conf>
bool CasADiProblem<Conf>::provides_eval_ψ() const {
    return impl->ψ.has_value();
}
template <Config Conf>
bool CasADiProblem<Conf>::provides_eval_grad_ψ() const {
    return impl->ψ_grad_ψ.has_value();
}
template <Config Conf>
bool CasADiProblem<Conf>::provides_eval_ψ_grad_ψ() const {
    return impl->ψ_grad_ψ.has_value();
}
template <Config Conf>
bool CasADiProblem<Conf>::provides_eval_grad_L() const {
    return impl->grad_L.has_value();
}
template <Config Conf>
bool CasADiProblem<Conf>::provides_eval_jac_g() const {
    return impl->jac_g.has_value();
}
template <Config Conf>
bool CasADiProblem<Conf>::provides_eval_hess_L_prod() const {
    return impl->hess_L_prod.has_value();
}
template <Config Conf>
bool CasADiProblem<Conf>::provides_eval_hess_L() const {
    return impl->hess_L.has_value();
}
template <Config Conf>
bool CasADiProblem<Conf>::provides_eval_hess_ψ_prod() const {
    return impl->hess_ψ_prod.has_value();
}
template <Config Conf>
bool CasADiProblem<Conf>::provides_eval_hess_ψ() const {
    return impl->hess_ψ.has_value();
}

template <Config Conf>
std::string CasADiProblem<Conf>::get_name() const {
    return name;
}

} // namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE
