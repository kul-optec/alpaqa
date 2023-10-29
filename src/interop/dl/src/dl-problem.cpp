#include <alpaqa/config/config.hpp>
#include <alpaqa/dl/dl-problem.h>
#include <alpaqa/dl/dl-problem.hpp>
#include <alpaqa/problem/sparsity.hpp>

#include <dlfcn.h>
#include <algorithm>
#include <cassert>
#include <charconv>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace alpaqa::dl {

namespace {

std::string format_abi_version(uint64_t version) {
    std::string s(16, '0');
    auto begin = s.data(), end = begin + s.size();
    auto [ptr, ec] = std::to_chars(begin, end, version, 16);
    if (ec != std::errc())
        throw std::logic_error(std::make_error_code(ec).message());
    std::rotate(begin, ptr, end);
    return s;
}

void check_abi_version(uint64_t abi_version) {
    if (abi_version != ALPAQA_DL_ABI_VERSION) {
        auto prob_version   = format_abi_version(abi_version);
        auto alpaqa_version = format_abi_version(ALPAQA_DL_ABI_VERSION);
        throw std::runtime_error(
            "alpaqa::dl::DLProblem::DLProblem: "
            "Incompatible problem definition (problem ABI version 0x" +
            prob_version + ", this version of alpaqa supports 0x" +
            alpaqa_version + ")");
    }
}

std::shared_ptr<void> load_lib(const std::string &so_filename) {
    assert(!so_filename.empty());
    ::dlerror();
    void *h = ::dlopen(so_filename.c_str(), RTLD_LOCAL | RTLD_NOW);
    if (auto *err = ::dlerror())
        throw std::runtime_error(err);
    return std::shared_ptr<void>{h, &::dlclose};
}

template <class F>
F *load_func(void *handle, const std::string &name) {
    assert(handle);
    ::dlerror();
    auto *h = ::dlsym(handle, name.c_str());
    if (auto *err = ::dlerror())
        throw std::runtime_error("Unable to load function '" + name +
                                 "': " + err);
    // We can only hope that the user got the signature right ...
    return reinterpret_cast<F *>(h);
}

std::mutex leaked_modules_mutex;
std::list<std::shared_ptr<void>> leaked_modules;
void leak_lib(std::shared_ptr<void> handle) {
    std::lock_guard lck{leaked_modules_mutex};
    leaked_modules.emplace_back(std::move(handle));
}

// clang-format off
template <Config Conf>
Sparsity<Conf> convert_sparsity(alpaqa_sparsity_t sp) {
    USING_ALPAQA_CONFIG(Conf);
    switch (sp.kind) {
            using sparsity::Symmetry;
        case alpaqa_sparsity_t::alpaqa_sparsity_dense:
            using Dense      = sparsity::Dense<config_t>;
            return Dense{
                .rows     = sp.dense.rows,
                .cols     = sp.dense.cols,
                .symmetry = static_cast<Symmetry>(sp.dense.symmetry),
            };
        case alpaqa_sparsity_t::alpaqa_sparsity_sparse_csc:
            using SparseCSC  = sparsity::SparseCSC<config_t, int>;
            return SparseCSC{
                .rows = sp.sparse_csc.rows,
                .cols = sp.sparse_csc.cols,
                .symmetry = static_cast<Symmetry>(sp.sparse_csc.symmetry),
                .inner_idx = typename SparseCSC::index_vector_map_t{sp.sparse_csc.inner_idx, sp.sparse_csc.nnz},
                .outer_ptr = typename SparseCSC::index_vector_map_t{sp.sparse_csc.outer_ptr, sp.sparse_csc.cols + 1},
                .order = static_cast<SparseCSC::Order>(sp.sparse_csc.order),
            };
        case alpaqa_sparsity_t::alpaqa_sparsity_sparse_csc_l:
            using SparseCSCl = sparsity::SparseCSC<config_t, long>;
            return SparseCSCl{
                .rows = sp.sparse_csc_l.rows,
                .cols = sp.sparse_csc_l.cols,
                .symmetry = static_cast<Symmetry>(sp.sparse_csc_l.symmetry),
                .inner_idx = typename SparseCSCl::index_vector_map_t{sp.sparse_csc_l.inner_idx, sp.sparse_csc_l.nnz},
                .outer_ptr = typename SparseCSCl::index_vector_map_t{sp.sparse_csc_l.outer_ptr, sp.sparse_csc_l.cols + 1},
                .order = static_cast<SparseCSCl::Order>(sp.sparse_csc_l.order),
            };
        case alpaqa_sparsity_t::alpaqa_sparsity_sparse_csc_ll:
            using SparseCSCll = sparsity::SparseCSC<config_t, long long>;
            return SparseCSCll{
                .rows = sp.sparse_csc_ll.rows,
                .cols = sp.sparse_csc_ll.cols,
                .symmetry = static_cast<Symmetry>(sp.sparse_csc_ll.symmetry),
                .inner_idx = typename SparseCSCll::index_vector_map_t{sp.sparse_csc_ll.inner_idx, sp.sparse_csc_ll.nnz},
                .outer_ptr = typename SparseCSCll::index_vector_map_t{sp.sparse_csc_ll.outer_ptr, sp.sparse_csc_ll.cols + 1},
                .order = static_cast<SparseCSCll::Order>(sp.sparse_csc_ll.order),
            };
        case alpaqa_sparsity_t::alpaqa_sparsity_sparse_coo:
            using SparseCOO  = sparsity::SparseCOO<config_t, int>;
            return SparseCOO{
                .rows = sp.sparse_coo.rows,
                .cols = sp.sparse_coo.cols,
                .symmetry = static_cast<Symmetry>(sp.sparse_coo.symmetry),
                .row_indices = typename SparseCOO::index_vector_map_t{sp.sparse_coo.row_indices, sp.sparse_coo.nnz},
                .col_indices = typename SparseCOO::index_vector_map_t{sp.sparse_coo.col_indices, sp.sparse_coo.nnz},
                .order = static_cast<SparseCOO::Order>(sp.sparse_coo.order),
                .first_index = sp.sparse_coo.first_index,
            };
        case alpaqa_sparsity_t::alpaqa_sparsity_sparse_coo_l:
            using SparseCOOl = sparsity::SparseCOO<config_t, long>;
            return SparseCOOl{
                .rows = sp.sparse_coo_l.rows,
                .cols = sp.sparse_coo_l.cols,
                .symmetry = static_cast<Symmetry>(sp.sparse_coo_l.symmetry),
                .row_indices = typename SparseCOOl::index_vector_map_t{sp.sparse_coo_l.row_indices, sp.sparse_coo_l.nnz},
                .col_indices = typename SparseCOOl::index_vector_map_t{sp.sparse_coo_l.col_indices, sp.sparse_coo_l.nnz},
                .order = static_cast<SparseCOOl::Order>(sp.sparse_coo_l.order),
                .first_index = sp.sparse_coo_l.first_index,
            };
        case alpaqa_sparsity_t::alpaqa_sparsity_sparse_coo_ll:
            using SparseCOOll = sparsity::SparseCOO<config_t, long long>;
            return SparseCOOll{
                .rows = sp.sparse_coo_ll.rows,
                .cols = sp.sparse_coo_ll.cols,
                .symmetry = static_cast<Symmetry>(sp.sparse_coo_ll.symmetry),
                .row_indices = typename SparseCOOll::index_vector_map_t{sp.sparse_coo_ll.row_indices, sp.sparse_coo_ll.nnz},
                .col_indices = typename SparseCOOll::index_vector_map_t{sp.sparse_coo_ll.col_indices, sp.sparse_coo_ll.nnz},
                .order = static_cast<SparseCOOll::Order>(sp.sparse_coo_ll.order),
                .first_index = sp.sparse_coo_ll.first_index,
            };
        default: throw std::invalid_argument("Invalid sparsity kind");
    }
}
// clang-format on

} // namespace

DLProblem::DLProblem(const std::string &so_filename,
                     const std::string &function_name, void *user_param)
    : BoxConstrProblem{0, 0} {
    handle = load_lib(so_filename);
    auto *register_func =
        load_func<problem_register_t(void *)>(handle.get(), function_name);
    auto r = register_func(user_param);
    // Avoid leaking if we throw (or if std::shared_ptr constructor throws)
    std::unique_ptr<void, void (*)(void *)> unique_inst{r.instance, r.cleanup};
    std::unique_ptr<alpaqa_function_dict_t> unique_extra{r.extra_functions};
    std::unique_ptr<alpaqa_exception_ptr_t> unique_exception{r.exception};
    check_abi_version(r.abi_version);
    // Check exception thrown by plugin
    if (unique_exception) {
        // Here we're facing an interesting problem: the exception we throw
        // might propagate upwards to a point where this instance is destroyed.
        // This would cause the shared module to be closed using dlclose.
        // However, the exception is still stored somewhere in the memory of
        // that module, causing a segmentation fault when accessed.
        // To get around this issue, we need to ensure that the shared module
        // is not closed. Here we simply leak it by storing a shared_ptr to it
        // in a global variable.
        leak_lib(handle);
        std::rethrow_exception(unique_exception->exc);
    }
    if (!r.functions)
        throw std::logic_error("alpaqa::dl::DLProblem::DLProblem: plugin did "
                               "not return any functions");
    // Store data returned by plugin
    instance    = std::shared_ptr<void>{std::move(unique_inst)};
    functions   = r.functions;
    extra_funcs = std::shared_ptr<function_dict_t>{std::move(unique_extra)};

    this->n = functions->n;
    this->m = functions->m;
    this->C = Box{this->n};
    this->D = Box{this->m};
    if (functions->initialize_box_C)
        functions->initialize_box_C(instance.get(), this->C.lowerbound.data(),
                                    this->C.upperbound.data());
    if (functions->initialize_box_D)
        functions->initialize_box_D(instance.get(), this->D.lowerbound.data(),
                                    this->D.upperbound.data());
    if (functions->initialize_l1_reg) {
        length_t nλ = 0;
        functions->initialize_l1_reg(instance.get(), nullptr, &nλ);
        if (nλ > 0) {
            this->l1_reg.resize(nλ);
            functions->initialize_l1_reg(instance.get(), this->l1_reg.data(),
                                         &nλ);
        }
    }
}

auto DLProblem::eval_prox_grad_step(real_t γ, crvec x, crvec grad_ψ, rvec x̂,
                                    rvec p) const -> real_t {
    if (functions->eval_prox_grad_step)
        return functions->eval_prox_grad_step(
            instance.get(), γ, x.data(), grad_ψ.data(), x̂.data(), p.data());
    return BoxConstrProblem<config_t>::eval_prox_grad_step(γ, x, grad_ψ, x̂, p);
}

// clang-format off
auto DLProblem::eval_f(crvec x) const -> real_t { return functions->eval_f(instance.get(), x.data()); }
auto DLProblem::eval_grad_f(crvec x, rvec grad_fx) const -> void { return functions->eval_grad_f(instance.get(), x.data(), grad_fx.data()); }
auto DLProblem::eval_g(crvec x, rvec gx) const -> void { return functions->eval_g(instance.get(), x.data(), gx.data()); }
auto DLProblem::eval_grad_g_prod(crvec x, crvec y, rvec grad_gxy) const -> void { return functions->eval_grad_g_prod(instance.get(), x.data(), y.data(), grad_gxy.data()); }
auto DLProblem::eval_grad_gi(crvec x, index_t i, rvec grad_gi) const -> void { return functions->eval_grad_gi(instance.get(), x.data(), i, grad_gi.data()); }
auto DLProblem::eval_jac_g(crvec x, rvec J_values) const -> void { return functions->eval_jac_g(instance.get(), x.data(), J_values.size() == 0 ? nullptr : J_values.data()); }
auto DLProblem::get_jac_g_sparsity() const -> Sparsity { return convert_sparsity<config_t>(functions->get_jac_g_sparsity(instance.get())); }
auto DLProblem::eval_hess_L_prod(crvec x, crvec y, real_t scale, crvec v, rvec Hv) const -> void { return functions->eval_hess_L_prod(instance.get(), x.data(), y.data(), scale, v.data(), Hv.data()); }
auto DLProblem::eval_hess_L(crvec x, crvec y, real_t scale, rvec H_values) const -> void { return functions->eval_hess_L(instance.get(), x.data(), y.data(), scale, H_values.size() == 0 ? nullptr : H_values.data()); }
auto DLProblem::get_hess_L_sparsity() const -> Sparsity { return convert_sparsity<config_t>(functions->get_hess_L_sparsity(instance.get())); }
auto DLProblem::eval_hess_ψ_prod(crvec x, crvec y, crvec Σ, real_t scale, crvec v, rvec Hv) const -> void { return functions->eval_hess_ψ_prod(instance.get(), x.data(), y.data(), Σ.data(), scale, D.lowerbound.data(), D.upperbound.data(), v.data(), Hv.data()); }
auto DLProblem::eval_hess_ψ(crvec x, crvec y, crvec Σ, real_t scale, rvec H_values) const -> void { return functions->eval_hess_ψ(instance.get(), x.data(), y.data(), Σ.data(), scale, D.lowerbound.data(), D.upperbound.data(), H_values.size() == 0 ? nullptr : H_values.data()); }
auto DLProblem::get_hess_ψ_sparsity() const -> Sparsity { return convert_sparsity<config_t>(functions->get_hess_ψ_sparsity(instance.get())); }
auto DLProblem::eval_f_grad_f(crvec x, rvec grad_fx) const -> real_t { return functions->eval_f_grad_f(instance.get(), x.data(), grad_fx.data()); }
auto DLProblem::eval_f_g(crvec x, rvec g) const -> real_t { return functions->eval_f_g(instance.get(), x.data(), g.data()); }
auto DLProblem::eval_grad_f_grad_g_prod(crvec x, crvec y, rvec grad_f, rvec grad_gxy) const -> void { return functions->eval_grad_f_grad_g_prod(instance.get(), x.data(), y.data(), grad_f.data(), grad_gxy.data()); }
auto DLProblem::eval_grad_L(crvec x, crvec y, rvec grad_L, rvec work_n) const -> void { return functions->eval_grad_L(instance.get(), x.data(), y.data(), grad_L.data(), work_n.data()); }
auto DLProblem::eval_ψ(crvec x, crvec y, crvec Σ, rvec ŷ) const -> real_t { return functions->eval_ψ(instance.get(), x.data(), y.data(), Σ.data(), D.lowerbound.data(), D.upperbound.data(), ŷ.data()); }
auto DLProblem::eval_grad_ψ(crvec x, crvec y, crvec Σ, rvec grad_ψ, rvec work_n, rvec work_m) const -> void { return functions->eval_grad_ψ(instance.get(), x.data(), y.data(), Σ.data(), D.lowerbound.data(), D.upperbound.data(), grad_ψ.data(), work_n.data(), work_m.data()); }
auto DLProblem::eval_ψ_grad_ψ(crvec x, crvec y, crvec Σ, rvec grad_ψ, rvec work_n, rvec work_m) const -> real_t { return functions->eval_ψ_grad_ψ(instance.get(), x.data(), y.data(), Σ.data(), D.lowerbound.data(), D.upperbound.data(), grad_ψ.data(), work_n.data(), work_m.data()); }

bool DLProblem::provides_eval_f() const { return functions->eval_f != nullptr; }
bool DLProblem::provides_eval_grad_f() const { return functions->eval_grad_f != nullptr; }
bool DLProblem::provides_eval_g() const { return functions->eval_g != nullptr; }
bool DLProblem::provides_eval_grad_g_prod() const { return functions->eval_grad_g_prod != nullptr; }
bool DLProblem::provides_eval_jac_g() const { return functions->eval_jac_g != nullptr; }
bool DLProblem::provides_get_jac_g_sparsity() const { return functions->get_jac_g_sparsity != nullptr; }
bool DLProblem::provides_eval_grad_gi() const { return functions->eval_grad_gi != nullptr; }
bool DLProblem::provides_eval_hess_L_prod() const { return functions->eval_hess_L_prod != nullptr; }
bool DLProblem::provides_eval_hess_L() const { return functions->eval_hess_L != nullptr; }
bool DLProblem::provides_get_hess_L_sparsity() const { return functions->get_hess_L_sparsity != nullptr; }
bool DLProblem::provides_eval_hess_ψ_prod() const { return functions->eval_hess_ψ_prod != nullptr; }
bool DLProblem::provides_eval_hess_ψ() const { return functions->eval_hess_ψ != nullptr; }
bool DLProblem::provides_get_hess_ψ_sparsity() const { return functions->get_hess_ψ_sparsity != nullptr; }
bool DLProblem::provides_eval_f_grad_f() const { return functions->eval_f_grad_f != nullptr; }
bool DLProblem::provides_eval_f_g() const { return functions->eval_f_g != nullptr; }
bool DLProblem::provides_eval_grad_f_grad_g_prod() const { return functions->eval_grad_f_grad_g_prod != nullptr; }
bool DLProblem::provides_eval_grad_L() const { return functions->eval_grad_L != nullptr; }
bool DLProblem::provides_eval_ψ() const { return functions->eval_ψ != nullptr; }
bool DLProblem::provides_eval_grad_ψ() const { return functions->eval_grad_ψ != nullptr; }
bool DLProblem::provides_eval_ψ_grad_ψ() const { return functions->eval_ψ_grad_ψ != nullptr; }
bool DLProblem::provides_get_box_C() const { return functions->eval_prox_grad_step == nullptr; }
// clang-format on

#if ALPAQA_WITH_OCP

DLControlProblem::DLControlProblem(const std::string &so_filename,
                                   const std::string &function_name,
                                   void *user_param) {
    handle              = load_lib(so_filename);
    auto *register_func = load_func<control_problem_register_t(void *)>(
        handle.get(), function_name);
    auto r = register_func(user_param);
    // Avoid leaking if we throw (or if std::shared_ptr constructor throws)
    std::unique_ptr<void, void (*)(void *)> unique_inst{r.instance, r.cleanup};
    std::unique_ptr<alpaqa_function_dict_t> unique_extra{r.extra_functions};
    std::unique_ptr<alpaqa_exception_ptr_t> unique_exception{r.exception};
    check_abi_version(r.abi_version);
    // Check exception thrown by plugin
    if (unique_exception) {
        // Here we're facing an interesting problem: the exception we throw
        // might propagate upwards to a point where this instance is destroyed.
        // This would cause the shared module to be closed using dlclose.
        // However, the exception is still stored somewhere in the memory of
        // that module, causing a segmentation fault when accessed.
        // To get around this issue, we need to ensure that the shared module
        // is not closed. Here we simply leak it by storing a shared_ptr to it
        // in a global variable.
        leak_lib(handle);
        std::rethrow_exception(unique_exception->exc);
    }
    if (!functions)
        throw std::logic_error("alpaqa::dl::DLControlProblem::DLControlProblem:"
                               " plugin did not return any functions");
    // Store data returned by plugin
    instance    = std::shared_ptr<void>{std::move(unique_inst)};
    functions   = r.functions;
    extra_funcs = std::shared_ptr<function_dict_t>{std::move(unique_extra)};
}

// clang-format off
auto DLControlProblem::get_U(Box &U) const -> void { return functions->get_U(instance.get(), U.lowerbound.data(), U.upperbound.data()); }
auto DLControlProblem::get_D(Box &D) const -> void { return functions->get_D(instance.get(), D.lowerbound.data(), D.upperbound.data()); }
auto DLControlProblem::get_D_N(Box &D) const -> void { return functions->get_D_N(instance.get(), D.lowerbound.data(), D.upperbound.data()); }
auto DLControlProblem::get_x_init(rvec x_init) const -> void { return functions->get_x_init(instance.get(), x_init.data()); }
auto DLControlProblem::eval_f(index_t timestep, crvec x, crvec u, rvec fxu) const -> void { return functions->eval_f(instance.get(), timestep, x.data(), u.data(), fxu.data()); }
auto DLControlProblem::eval_jac_f(index_t timestep, crvec x, crvec u, rmat J_fxu) const -> void { return functions->eval_jac_f(instance.get(), timestep, x.data(), u.data(), J_fxu.data()); }
auto DLControlProblem::eval_grad_f_prod(index_t timestep, crvec x, crvec u, crvec p, rvec grad_fxu_p) const -> void { return functions->eval_grad_f_prod(instance.get(), timestep, x.data(), u.data(), p.data(), grad_fxu_p.data()); }
auto DLControlProblem::eval_h(index_t timestep, crvec x, crvec u, rvec h) const -> void { return functions->eval_h(instance.get(), timestep, x.data(), u.data(), h.data()); }
auto DLControlProblem::eval_h_N(crvec x, rvec h) const -> void { return functions->eval_h_N(instance.get(), x.data(), h.data()); }
auto DLControlProblem::eval_l(index_t timestep, crvec h) const -> real_t { return functions->eval_l(instance.get(), timestep, h.data()); }
auto DLControlProblem::eval_l_N(crvec h) const -> real_t { return functions->eval_l_N(instance.get(), h.data()); }
auto DLControlProblem::eval_qr(index_t timestep, crvec xu, crvec h, rvec qr) const -> void { return functions->eval_qr(instance.get(), timestep, xu.data(), h.data(), qr.data()); }
auto DLControlProblem::eval_q_N(crvec x, crvec h, rvec q) const -> void { return functions->eval_q_N(instance.get(), x.data(), h.data(), q.data()); }
auto DLControlProblem::eval_add_Q(index_t timestep, crvec xu, crvec h, rmat Q) const -> void { return functions->eval_add_Q(instance.get(), timestep, xu.data(), h.data(), Q.data()); }
auto DLControlProblem::eval_add_Q_N(crvec x, crvec h, rmat Q) const -> void { return functions->eval_add_Q_N(instance.get(), x.data(), h.data(), Q.data()); }
auto DLControlProblem::eval_add_R_masked(index_t timestep, crvec xu, crvec h, crindexvec mask, rmat R, rvec work) const -> void { return functions->eval_add_R_masked(instance.get(), timestep, xu.data(), h.data(), mask.data(), R.data(), work.data()); }
auto DLControlProblem::eval_add_S_masked(index_t timestep, crvec xu, crvec h, crindexvec mask, rmat S, rvec work) const -> void { return functions->eval_add_S_masked(instance.get(), timestep, xu.data(), h.data(), mask.data(), S.data(), work.data()); }
auto DLControlProblem::eval_add_R_prod_masked(index_t timestep, crvec xu, crvec h, crindexvec mask_J, crindexvec mask_K, crvec v, rvec out, rvec work) const -> void { return functions->eval_add_R_prod_masked(instance.get(), timestep, xu.data(), h.data(), mask_J.data(), mask_K.data(), v.data(), out.data(), work.data()); }
auto DLControlProblem::eval_add_S_prod_masked(index_t timestep, crvec xu, crvec h, crindexvec mask_K, crvec v, rvec out, rvec work) const -> void { return functions->eval_add_S_prod_masked(instance.get(), timestep, xu.data(), h.data(), mask_K.data(), v.data(), out.data(), work.data()); }
auto DLControlProblem::get_R_work_size() const -> length_t { return functions->get_R_work_size(instance.get()); }
auto DLControlProblem::get_S_work_size() const -> length_t { return functions->get_S_work_size(instance.get()); }
auto DLControlProblem::eval_constr(index_t timestep, crvec x, rvec c) const -> void { return functions->eval_constr(instance.get(), timestep, x.data(), c.data()); }
auto DLControlProblem::eval_constr_N(crvec x, rvec c) const -> void { return functions->eval_constr_N(instance.get(), x.data(), c.data()); }
auto DLControlProblem::eval_grad_constr_prod(index_t timestep, crvec x, crvec p, rvec grad_cx_p) const -> void { return functions->eval_grad_constr_prod(instance.get(), timestep, x.data(), p.data(), grad_cx_p.data()); }
auto DLControlProblem::eval_grad_constr_prod_N(crvec x, crvec p, rvec grad_cx_p) const -> void { return functions->eval_grad_constr_prod_N(instance.get(), x.data(), p.data(), grad_cx_p.data()); }
auto DLControlProblem::eval_add_gn_hess_constr(index_t timestep, crvec x, crvec M, rmat out) const -> void { return functions->eval_add_gn_hess_constr(instance.get(), timestep, x.data(), M.data(), out.data()); }
auto DLControlProblem::eval_add_gn_hess_constr_N(crvec x, crvec M, rmat out) const -> void { return functions->eval_add_gn_hess_constr_N(instance.get(), x.data(), M.data(), out.data()); }

bool DLControlProblem::provides_get_D() const { return functions->get_D != nullptr; }
bool DLControlProblem::provides_get_D_N() const { return functions->get_D_N != nullptr; }
bool DLControlProblem::provides_eval_add_Q_N() const { return functions->eval_add_Q_N != nullptr; }
bool DLControlProblem::provides_eval_add_R_prod_masked() const { return functions->eval_add_R_prod_masked != nullptr; }
bool DLControlProblem::provides_eval_add_S_prod_masked() const { return functions->eval_add_S_prod_masked != nullptr; }
bool DLControlProblem::provides_get_R_work_size() const { return functions->get_R_work_size != nullptr; }
bool DLControlProblem::provides_get_S_work_size() const { return functions->get_S_work_size != nullptr; }
bool DLControlProblem::provides_eval_constr() const { return functions->eval_constr != nullptr; }
bool DLControlProblem::provides_eval_constr_N() const { return functions->eval_constr_N != nullptr; }
bool DLControlProblem::provides_eval_grad_constr_prod() const { return functions->eval_grad_constr_prod != nullptr; }
bool DLControlProblem::provides_eval_grad_constr_prod_N() const { return functions->eval_grad_constr_prod_N != nullptr; }
bool DLControlProblem::provides_eval_add_gn_hess_constr() const { return functions->eval_add_gn_hess_constr != nullptr; }
bool DLControlProblem::provides_eval_add_gn_hess_constr_N() const { return functions->eval_add_gn_hess_constr_N != nullptr; }
// clang-format on

#endif

} // namespace alpaqa::dl