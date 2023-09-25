#include <alpaqa/cutest/cutest-loader.hpp>

#include <alpaqa/cutest/cutest-errors.hpp>
#include <alpaqa/problem/sparsity.hpp>

#include <dlfcn.h>

#include <cassert>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "cutest-functions.hpp"

using namespace std::string_literals;

namespace {
void throw_error(std::string_view s, int code) {
    throw alpaqa::cutest::function_call_error(
        std::string(s), static_cast<alpaqa::cutest::Status>(code));
}
void throw_if_error(std::string_view s, int code) {
    if (code)
        throw_error(s, code);
}
void log_if_error(std::string_view s, int code) {
    if (code)
        std::cerr << s << " (" << code << ")\n";
}
template <class F>
auto checked(F &&func, std::string_view msg) {
    return [msg, func{std::forward<F>(func)}]<class... Args>(
               Args &&...args) mutable {
        alpaqa::cutest::integer status;
        std::forward<F>(func)(&status, std::forward<Args>(args)...);
        throw_if_error(msg, status);
    };
}

std::shared_ptr<void> load_lib(const char *so_filename) {
    assert(so_filename);
    ::dlerror();
    void *h = ::dlopen(so_filename, RTLD_LOCAL | RTLD_NOW);
    if (auto *err = ::dlerror())
        throw std::runtime_error(err);
    assert(h);
    return std::shared_ptr<void>{h, &::dlclose};
}
} // namespace

namespace alpaqa {

class CUTEstLoader {
  public:
    USING_ALPAQA_CONFIG(CUTEstProblem::config_t);
    using Box = alpaqa::Box<config_t>;

  private:
    using cleanup_t = std::shared_ptr<void>;
    template <class F>
    cleanup_t cleanup(F &&func) {
        return cleanup_t{nullptr,
                         [func{std::forward<F>(func)}](void *) { func(); }};
    }

    using integer = cutest::integer;
    using logical = cutest::logical;

    template <class F>
    auto load() -> F::signature_t * {
        return F::load(so_handle.get());
    }

    template <class F, class... Args>
    decltype(auto) call(Args &&...args) {
        return load<F>()(std::forward<Args>(args)...);
    }

    cleanup_t load_outsdif(const char *outsdif_fname) {
        std::filesystem::path p = outsdif_fname;
        if (!std::filesystem::is_regular_file(p))
            throw std::invalid_argument("CUTEstLoader: OUTSDIF path does not "
                                        "exist or is not a regular file: \"" +
                                        std::string(outsdif_fname) + '"');
        integer status;
        auto fptr_close = load<cutest::fortran_close>();
        call<cutest::fortran_open>(&funit, outsdif_fname, &status);
        throw_if_error("Failed to open "s + outsdif_fname, status);
        return cleanup([funit{this->funit}, fptr_close] {
            integer status;
            fptr_close(&funit, &status);
            log_if_error("Failed to close OUTSDIF.d file", status);
        });
    }

    cleanup_t terminator() {
        auto fptr_cterminate = load<cutest::cterminate>();
        return cleanup([fptr_cterminate] {
            integer status;
            fptr_cterminate(&status);
            log_if_error("Failed to call cutest_cterminate", status);
        });
    }

  public:
    CUTEstLoader(const char *so_fname, const char *outsdif_fname) {
        // Open the shared library
        so_handle = load_lib(so_fname);

        // Open the OUTSDIF.d file
        if (outsdif_fname && *outsdif_fname)
            cleanup_outsdif = load_outsdif(outsdif_fname);
        else
            cleanup_outsdif = load_outsdif(std::filesystem::path(so_fname)
                                               .replace_filename("OUTSDIF.d")
                                               .c_str());

        // Get the dimensions of the problem
        integer status;
        call<cutest::cdimen>(&status, &funit, &nvar, &ncon);
        throw_if_error("Failed to call cutest_cdimen", status);
    }

    struct ConstrFuncs {
        cutest::cfn::signature_t *cfn;
        cutest::cofg::signature_t *cofg;
        cutest::ccfg::signature_t *ccfg;
        cutest::clfg::signature_t *clfg;
        cutest::cjprod::signature_t *cjprod;
        cutest::ccifg::signature_t *ccifg;
        cutest::cigr::signature_t *cigr;
        cutest::cdimsj::signature_t *cdimsj;
        cutest::csjp::signature_t *csjp;
        cutest::ccfsg::signature_t *ccfsg;
        cutest::cdh::signature_t *cdh;
        cutest::cdimsh::signature_t *cdimsh;
        cutest::cshp::signature_t *cshp;
        cutest::csh::signature_t *csh;
        cutest::chprod::signature_t *chprod;
    };

    void setup_problem(rvec x0, rvec y0, Box &C, Box &D) {
        assert(x0.size() == static_cast<length_t>(nvar));
        assert(C.lowerbound.size() == static_cast<length_t>(nvar));
        assert(C.upperbound.size() == static_cast<length_t>(nvar));
        assert(y0.size() == static_cast<length_t>(ncon));
        assert(D.lowerbound.size() == static_cast<length_t>(ncon));
        assert(D.upperbound.size() == static_cast<length_t>(ncon));

        // Variables returned and required by csetup
        equatn.resize(static_cast<length_t>(ncon));
        linear.resize(static_cast<length_t>(ncon));
        integer e_order = 0; // no specific order of equality constraints
        integer l_order = 0; // no specific order of linear constraints
        integer v_order = 0; // no specific order of linear variables
        integer status;

        // Initialize the problem
        call<cutest::csetup>(
            &status, &funit, &iout, &io_buffer, &nvar, &ncon, x0.data(),
            C.lowerbound.data(), C.upperbound.data(), y0.data(),
            D.lowerbound.data(), D.upperbound.data(), equatn.data(),
            linear.data(), &e_order, &l_order, &v_order);
        throw_if_error("Failed to call cutest_csetup", status);
        cutest_terminate = terminator();

        // Check the number of constraints
        if (ncon == 0)
            throw std::runtime_error(
                "Unconstrained CUTEst problems are currently unsupported");

        // Allocate workspaces
        work.resize(std::max(nvar, ncon));
        work2.resize(std::max(nvar, ncon));
        // Convert bounds
        std::ranges::replace(C.lowerbound, -cutest::inf, -inf<config_t>);
        std::ranges::replace(C.upperbound, +cutest::inf, +inf<config_t>);
        std::ranges::replace(D.lowerbound, -cutest::inf, -inf<config_t>);
        std::ranges::replace(D.upperbound, +cutest::inf, +inf<config_t>);
        // Load problem functions and gradients
        funcs = {
            .cfn    = load<cutest::cfn>(),
            .cofg   = load<cutest::cofg>(),
            .ccfg   = load<cutest::ccfg>(),
            .clfg   = load<cutest::clfg>(),
            .cjprod = load<cutest::cjprod>(),
            .ccifg  = load<cutest::ccifg>(),
            .cigr   = load<cutest::cigr>(),
            .cdimsj = load<cutest::cdimsj>(),
            .csjp   = load<cutest::csjp>(),
            .ccfsg  = load<cutest::ccfsg>(),
            .cdh    = load<cutest::cdh>(),
            .cdimsh = load<cutest::cdimsh>(),
            .cshp   = load<cutest::cshp>(),
            .csh    = load<cutest::csh>(),
            .chprod = load<cutest::chprod>(),
        };
    }

    std::string get_name() {
        std::string name(cutest::fstring_len, ' ');
        integer status;
        call<cutest::probname>(&status, name.data());
        throw_if_error("Failed to call CUTEST_probname", status);
        if (auto last = name.find_last_not_of(' '); last != name.npos)
            name.resize(last + 1);
        return name;
    }

    void get_report(double *calls, double *time) {
        integer status;
        if (ncon > 0) {
            call<cutest::creport>(&status, calls, time);
            throw_if_error("Failed to call CUTEST_creport", status);
        } else {
            call<cutest::ureport>(&status, calls, time);
            throw_if_error("Failed to call CUTEST_ureport", status);
        }
    }

    // Order of cleanup is important!
    std::shared_ptr<void> so_handle; ///< dlopen handle to shared library
    cleanup_t cleanup_outsdif;  ///< Responsible for closing the OUTSDIF.d file
    cleanup_t cutest_terminate; ///< Responsible for calling CUTEST_xterminate

    integer funit     = 42; ///< Fortran Unit Number for OUTSDIF.d file
    integer iout      = 6;  ///< Fortran Unit Number for standard output
    integer io_buffer = 11; ///< Fortran Unit Number for internal IO

    integer nvar;      ///< Number of decision variabls
    integer ncon;      ///< Number of constraints
    ConstrFuncs funcs; /// Pointers to loaded problem functions

    using logical_vec = Eigen::VectorX<logical>;
    logical_vec equatn;      ///< whether the constraint is an equality
    logical_vec linear;      ///< whether the constraint is linear
    mutable vec work, work2; ///< work vectors
};

CUTEstProblem::CUTEstProblem(const char *so_fname, const char *outsdif_fname,
                             bool sparse)
    : BoxConstrProblem<config_t>{0, 0}, sparse{sparse} {
    impl = std::make_unique<CUTEstLoader>(so_fname, outsdif_fname);
    resize(static_cast<length_t>(impl->nvar),
           static_cast<length_t>(impl->ncon));
    x0.resize(n);
    y0.resize(m);
    impl->setup_problem(x0, y0, C, D);
    name = impl->get_name();
}

CUTEstProblem::CUTEstProblem(const CUTEstProblem &)                = default;
CUTEstProblem &CUTEstProblem::operator=(const CUTEstProblem &)     = default;
CUTEstProblem::CUTEstProblem(CUTEstProblem &&) noexcept            = default;
CUTEstProblem &CUTEstProblem::operator=(CUTEstProblem &&) noexcept = default;
CUTEstProblem::~CUTEstProblem()                                    = default;

auto CUTEstProblem::get_report() const -> Report {
    double calls[7]; // NOLINT(*-c-arrays)
    double time[2];  // NOLINT(*-c-arrays)
    impl->get_report(calls, time);
    const bool constr = impl->ncon > 0;
    return {
        .calls{
            .objective            = static_cast<unsigned>(calls[0]),
            .objective_grad       = static_cast<unsigned>(calls[1]),
            .objective_hess       = static_cast<unsigned>(calls[2]),
            .hessian_times_vector = static_cast<unsigned>(calls[3]),
            .constraints      = constr ? static_cast<unsigned>(calls[4]) : 0,
            .constraints_grad = constr ? static_cast<unsigned>(calls[5]) : 0,
            .constraints_hess = constr ? static_cast<unsigned>(calls[6]) : 0,
        },
        .time_setup = time[0],
        .time       = time[1],
    };
}

auto CUTEstProblem::eval_f(crvec x) const -> real_t {
    assert(x.size() == static_cast<length_t>(impl->nvar));
    real_t f;
    cutest::logical grad = cutest::False;
    checked(impl->funcs.cofg, "eval_f: CUTEST_cofg")(&impl->nvar, x.data(), &f,
                                                     nullptr, &grad);
    return f;
}
void CUTEstProblem::eval_grad_f(crvec x, rvec grad_fx) const {
    assert(x.size() == static_cast<length_t>(impl->nvar));
    assert(grad_fx.size() == static_cast<length_t>(impl->nvar));
    real_t f;
    cutest::logical grad = cutest::True;
    checked(impl->funcs.cofg, "eval_grad_f: CUTEST_cofg")(
        &impl->nvar, x.data(), &f, grad_fx.data(), &grad);
}
void CUTEstProblem::eval_g(crvec x, rvec gx) const {
    assert(x.size() == static_cast<length_t>(impl->nvar));
    assert(gx.size() == static_cast<length_t>(impl->ncon));
    cutest::logical jtrans = cutest::True, grad = cutest::False;
    cutest::integer zero = 0;
    checked(impl->funcs.ccfg, "eval_g: CUTEST_ccfg")(
        &impl->nvar, &impl->ncon, x.data(), gx.data(), &jtrans, &zero, &zero,
        nullptr, &grad);
}
void CUTEstProblem::eval_grad_g_prod(crvec x, crvec y, rvec grad_gxy) const {
    assert(x.size() == static_cast<length_t>(impl->nvar));
    assert(y.size() == static_cast<length_t>(impl->ncon));
    assert(grad_gxy.size() == static_cast<length_t>(impl->nvar));
    auto lvector         = static_cast<cutest::integer>(y.size()),
         lresult         = static_cast<cutest::integer>(grad_gxy.size());
    cutest::logical gotj = cutest::False, jtrans = cutest::True;
    checked(impl->funcs.cjprod, "eval_grad_g_prod: CUTEST_cjprod")(
        &impl->nvar, &impl->ncon, &gotj, &jtrans, x.data(), y.data(), &lvector,
        grad_gxy.data(), &lresult);
}

void CUTEstProblem::eval_jac_g(crvec x, rvec J_values) const {
    // Compute the nonzero values
    assert(x.size() == static_cast<length_t>(impl->nvar));
    // Sparse Jacobian
    if (sparse) {
        assert(nnz_J >= 0);
        assert(J_values.size() == static_cast<length_t>(nnz_J));
        assert(storage_jac_g.rows.size() == static_cast<length_t>(nnz_J));
        assert(storage_jac_g.cols.size() == static_cast<length_t>(nnz_J));
        const cutest::integer nnz = nnz_J;
        cutest::logical grad      = cutest::True;
        checked(impl->funcs.ccfsg, "eval_jac_g: CUTEST_ccfsg")(
            &impl->nvar, &impl->ncon, x.data(), impl->work.data(), &nnz_J, &nnz,
            J_values.data(), storage_jac_g.cols.data(),
            storage_jac_g.rows.data(), &grad);
    }
    // Dense Jacobian
    else {
        assert(J_values.size() == static_cast<length_t>(impl->nvar) *
                                      static_cast<length_t>(impl->ncon));
        cutest::logical jtrans = cutest::False, grad = cutest::True;
        checked(impl->funcs.ccfg, "eval_jac_g: CUTEST_ccfg")(
            &impl->nvar, &impl->ncon, x.data(), impl->work.data(), &jtrans,
            &impl->ncon, &impl->nvar, J_values.data(), &grad);
    }
}
auto CUTEstProblem::get_jac_g_sparsity() const -> Sparsity {
    if (!sparse)
        return sparsity::Dense<config_t>{
            .rows     = m,
            .cols     = n,
            .symmetry = sparsity::Symmetry::Unsymmetric,
        };
    if (nnz_J < 0) {
        checked(impl->funcs.cdimsj,
                "get_jac_g_sparsity: CUTEST_cdimsj")(&nnz_J);
        nnz_J -= impl->nvar;
        assert(nnz_J >= 0);
        storage_jac_g.cols.resize(nnz_J);
        storage_jac_g.rows.resize(nnz_J);
        const cutest::integer nnz = nnz_J;
        checked(impl->funcs.csjp, "eval_jac_g: CUTEST_csjp")(
            &nnz_J, &nnz, storage_jac_g.cols.data(), storage_jac_g.rows.data());
    }
    using SparseCOO = sparsity::SparseCOO<config_t, int>;
    return SparseCOO{
        .rows        = m,
        .cols        = n,
        .symmetry    = sparsity::Symmetry::Unsymmetric,
        .row_indices = storage_jac_g.rows,
        .col_indices = storage_jac_g.cols,
        .order       = SparseCOO::Unsorted,
        .first_index = 1, // Fortran-style indices
    };
}
void CUTEstProblem::eval_grad_gi(crvec x, index_t i, rvec grad_gi) const {
    assert(x.size() == static_cast<length_t>(impl->nvar));
    assert(grad_gi.size() == static_cast<length_t>(impl->nvar));
    auto iprob = static_cast<cutest::integer>(i + 1); // index zero is objective
    checked(impl->funcs.cigr, "eval_grad_gi: CUTEST_cigr")(
        &impl->nvar, &iprob, x.data(), grad_gi.data());
}
void CUTEstProblem::eval_hess_L_prod(crvec x, crvec y, real_t scale, crvec v,
                                     rvec Hv) const {
    assert(x.size() == static_cast<length_t>(impl->nvar));
    assert(y.size() == static_cast<length_t>(impl->ncon));
    assert(v.size() == static_cast<length_t>(impl->nvar));
    assert(Hv.size() == static_cast<length_t>(impl->nvar));
    const auto *mult = y.data();
    if (scale != 1) {
        impl->work = y * (real_t(1) / scale);
        mult       = impl->work.data();
    }
    cutest::logical goth = cutest::False;
    checked(impl->funcs.chprod, "eval_hess_L_prod: CUTEST_chprod")(
        &impl->nvar, &impl->ncon, &goth, x.data(), mult,
        const_cast<real_t *>(v.data()), Hv.data());
    if (scale != 1)
        Hv *= scale;
}
void CUTEstProblem::eval_hess_ψ_prod(crvec x, crvec y, crvec Σ, real_t scale,
                                     crvec v, rvec Hv) const {
    assert(x.size() == static_cast<length_t>(impl->nvar));
    assert(y.size() == static_cast<length_t>(impl->ncon));
    assert(Σ.size() == static_cast<length_t>(impl->ncon));
    assert(v.size() == static_cast<length_t>(impl->nvar));
    assert(Hv.size() == static_cast<length_t>(impl->nvar));
    auto &&ζ = impl->work.topRows(impl->ncon);
    auto &&ŷ = impl->work2.topRows(impl->ncon);
    // ζ = g(x) + Σ⁻¹y
    eval_g(x, ζ);
    ζ += Σ.asDiagonal().inverse() * y;
    // d = ζ - Π(ζ, D)
    this->eval_proj_diff_g(ζ, ŷ);
    // ŷ = Σ d
    ŷ.array() *= Σ.array();
    // Hv = ∇²ℒ(x, ŷ) v
    eval_hess_L_prod(x, ŷ, scale, v, Hv);
    // Find active constraints
    for (index_t i = 0; i < impl->ncon; ++i)
        ζ(i) = (ζ(i) <= D.lowerbound(i)) || (D.upperbound(i) <= ζ(i))
                   ? Σ(i)
                   : real_t(0);
    // Jg(x) v
    auto &&Jv            = impl->work2.topRows(impl->ncon);
    auto lvector         = static_cast<cutest::integer>(v.size()),
         lresult         = static_cast<cutest::integer>(Jv.size());
    cutest::logical gotj = cutest::False, jtrans = cutest::False;
    checked(impl->funcs.cjprod, "eval_hess_ψ_prod: CUTEST_cjprod-1")(
        &impl->nvar, &impl->ncon, &gotj, &jtrans, x.data(), v.data(), &lvector,
        Jv.data(), &lresult);
    // Σ Jg v
    Jv.array() *= ζ.array();
    // Jgᵀ Σ Jg v
    std::swap(lvector, lresult);
    gotj = jtrans = cutest::True;
    auto &&JΣJv   = impl->work.topRows(impl->nvar);
    checked(impl->funcs.cjprod, "eval_hess_ψ_prod: CUTEST_cjprod-2")(
        &impl->nvar, &impl->ncon, &gotj, &jtrans, x.data(), Jv.data(), &lvector,
        JΣJv.data(), &lresult);
    Hv += JΣJv;
}
void CUTEstProblem::eval_hess_L(crvec x, crvec y, real_t scale,
                                rvec H_values) const {
    // Compute the nonzero values
    assert(x.size() == static_cast<length_t>(impl->nvar));
    assert(y.size() == static_cast<length_t>(impl->ncon));
    const auto *mult = y.data();
    if (scale != 1) {
        impl->work = y * (real_t(1) / scale);
        mult       = impl->work.data();
    }
    // Sparse Hessian
    if (sparse) {
        assert(nnz_H >= 0);
        assert(H_values.size() == static_cast<length_t>(nnz_H));
        assert(storage_hess_L.rows.size() == static_cast<length_t>(nnz_H));
        assert(storage_hess_L.cols.size() == static_cast<length_t>(nnz_H));
        const cutest::integer nnz = nnz_H;
        checked(impl->funcs.csh, "eval_hess_L: CUTEST_csh")(
            &impl->nvar, &impl->ncon, x.data(), y.data(), &nnz_H, &nnz,
            H_values.data(), storage_hess_L.rows.data(),
            storage_hess_L.cols.data());
    }
    // Dense Hessian
    else {
        assert(H_values.size() == static_cast<length_t>(impl->nvar) *
                                      static_cast<length_t>(impl->nvar));
        checked(impl->funcs.cdh,
                "eval_hess_L: CUTEST_cdh")(&impl->nvar, &impl->ncon, x.data(),
                                           mult, &impl->nvar, H_values.data());
    }
    if (scale != 1)
        H_values *= scale;
}
auto CUTEstProblem::get_hess_L_sparsity() const -> Sparsity {
    if (!sparse)
        return sparsity::Dense<config_t>{
            .rows     = n,
            .cols     = n,
            .symmetry = sparsity::Symmetry::Upper,
        };
    if (nnz_H < 0) {
        checked(impl->funcs.cdimsh,
                "get_hess_L_sparsity: CUTEST_cdimsh")(&nnz_H);
        assert(nnz_H >= 0);
        storage_hess_L.rows.resize(nnz_H);
        storage_hess_L.cols.resize(nnz_H);
        const cutest::integer nnz = nnz_H;
        checked(impl->funcs.cshp, "eval_hess_L: CUTEST_cshp")(
            &impl->nvar, &nnz_H, &nnz, storage_hess_L.rows.data(),
            storage_hess_L.cols.data());
    }
    using SparseCOO = sparsity::SparseCOO<config_t, int>;
    return SparseCOO{
        .rows        = n,
        .cols        = n,
        .symmetry    = sparsity::Symmetry::Upper,
        .row_indices = storage_hess_L.rows,
        .col_indices = storage_hess_L.cols,
        .order       = SparseCOO::Unsorted,
        .first_index = 1, // Fortran-style indices
    };
}
auto CUTEstProblem::eval_f_grad_f(crvec x, rvec grad_fx) const -> real_t {
    assert(x.size() == static_cast<length_t>(impl->nvar));
    assert(grad_fx.size() == static_cast<length_t>(impl->nvar));
    real_t f;
    cutest::logical grad = cutest::True;
    checked(impl->funcs.cofg, "eval_f_grad_f: CUTEST_cofg")(
        &impl->nvar, x.data(), &f, grad_fx.data(), &grad);
    return f;
}
auto CUTEstProblem::eval_f_g(crvec x, rvec g) const -> real_t {
    assert(x.size() == static_cast<length_t>(impl->nvar));
    assert(g.size() == static_cast<length_t>(impl->ncon));
    real_t f;
    checked(impl->funcs.cfn, "eval_f_g: CUTEST_cfn")(&impl->nvar, &impl->ncon,
                                                     x.data(), &f, g.data());
    return f;
}
void CUTEstProblem::eval_grad_L(crvec x, crvec y, rvec grad_L, rvec) const {
    assert(x.size() == static_cast<length_t>(impl->nvar));
    assert(y.size() == static_cast<length_t>(impl->ncon));
    assert(grad_L.size() == static_cast<length_t>(impl->nvar));
    real_t L;
    cutest::logical grad = cutest::True;
    checked(impl->funcs.clfg, "eval_f_g: CUTEST_clfg")(
        &impl->nvar, &impl->ncon, x.data(), y.data(), &L, grad_L.data(), &grad);
}

std::ostream &CUTEstProblem::format_report(std::ostream &os,
                                           const Report &r) const {
    os << "CUTEst problem: " << name << "\r\n\n"
       << "Number of variables:   " << n << "\r\n"
       << "Number of constraints: " << m << "\r\n\n"
       << "Objective function evaluations:            " //
       << r.calls.objective << "\r\n"
       << "Objective function gradient evaluations:   " //
       << r.calls.objective_grad << "\r\n"
       << "Objective function Hessian evaluations:    " //
       << r.calls.objective_hess << "\r\n"
       << "Hessian times vector products:             " //
       << r.calls.objective_hess << "\r\n\n";
    if (m > 0) {
        os << "Constraint function evaluations:           " //
           << r.calls.constraints << "\r\n"
           << "Constraint function gradients evaluations: " //
           << r.calls.constraints_grad << "\r\n"
           << "Constraint function Hessian evaluations:   " //
           << r.calls.constraints_hess << "\r\n\n";
    }
    return os << "Setup time:       " << r.time_setup << "s\r\n"
              << "Time since setup: " << r.time << "s";
}

} // namespace alpaqa