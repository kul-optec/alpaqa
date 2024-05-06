#include <alpaqa/config/config.hpp>
#include <alpaqa/dl/dl-problem.h>
#include <alpaqa/dl/dl-problem.hpp>
#include <alpaqa/problem/sparsity.hpp>

#include <algorithm>
#include <cassert>
#include <charconv>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>

#if _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

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
        throw invalid_abi_error(
            "alpaqa::dl::DLProblem::DLProblem: "
            "Incompatible problem definition (problem ABI version 0x" +
            prob_version + ", this version of alpaqa supports 0x" +
            alpaqa_version + ")");
    }
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
                .order = static_cast<typename SparseCSC::Order>(sp.sparse_csc.order),
            };
        case alpaqa_sparsity_t::alpaqa_sparsity_sparse_csc_l:
            using SparseCSCl = sparsity::SparseCSC<config_t, long>;
            return SparseCSCl{
                .rows = sp.sparse_csc_l.rows,
                .cols = sp.sparse_csc_l.cols,
                .symmetry = static_cast<Symmetry>(sp.sparse_csc_l.symmetry),
                .inner_idx = typename SparseCSCl::index_vector_map_t{sp.sparse_csc_l.inner_idx, sp.sparse_csc_l.nnz},
                .outer_ptr = typename SparseCSCl::index_vector_map_t{sp.sparse_csc_l.outer_ptr, sp.sparse_csc_l.cols + 1},
                .order = static_cast<typename SparseCSCl::Order>(sp.sparse_csc_l.order),
            };
        case alpaqa_sparsity_t::alpaqa_sparsity_sparse_csc_ll:
            using SparseCSCll = sparsity::SparseCSC<config_t, long long>;
            return SparseCSCll{
                .rows = sp.sparse_csc_ll.rows,
                .cols = sp.sparse_csc_ll.cols,
                .symmetry = static_cast<Symmetry>(sp.sparse_csc_ll.symmetry),
                .inner_idx = typename SparseCSCll::index_vector_map_t{sp.sparse_csc_ll.inner_idx, sp.sparse_csc_ll.nnz},
                .outer_ptr = typename SparseCSCll::index_vector_map_t{sp.sparse_csc_ll.outer_ptr, sp.sparse_csc_ll.cols + 1},
                .order = static_cast<typename SparseCSCll::Order>(sp.sparse_csc_ll.order),
            };
        case alpaqa_sparsity_t::alpaqa_sparsity_sparse_coo:
            using SparseCOO  = sparsity::SparseCOO<config_t, int>;
            return SparseCOO{
                .rows = sp.sparse_coo.rows,
                .cols = sp.sparse_coo.cols,
                .symmetry = static_cast<Symmetry>(sp.sparse_coo.symmetry),
                .row_indices = typename SparseCOO::index_vector_map_t{sp.sparse_coo.row_indices, sp.sparse_coo.nnz},
                .col_indices = typename SparseCOO::index_vector_map_t{sp.sparse_coo.col_indices, sp.sparse_coo.nnz},
                .order = static_cast<typename SparseCOO::Order>(sp.sparse_coo.order),
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
                .order = static_cast<typename SparseCOOl::Order>(sp.sparse_coo_l.order),
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
                .order = static_cast<typename SparseCOOll::Order>(sp.sparse_coo_ll.order),
                .first_index = sp.sparse_coo_ll.first_index,
            };
        default: throw std::invalid_argument("Invalid sparsity kind");
    }
}
// clang-format on

} // namespace

DLProblem::DLProblem(const std::filesystem::path &so_filename,
                     const std::string &function_name,
                     alpaqa_register_arg_t user_param,
                     DynamicLoadFlags dl_flags)
    : BoxConstrProblem{0, 0}, file{so_filename} {
    if (so_filename.empty())
        throw std::invalid_argument("Invalid problem filename");
    handle = util::load_lib(so_filename, dl_flags);
    try {
        auto *version_func = reinterpret_cast<alpaqa_dl_abi_version_t (*)()>(
            util::load_func(handle.get(), function_name + "_version"));
        check_abi_version(version_func());
    } catch (const dynamic_load_error &) {
        std::cerr << "Warning: problem " << so_filename
                  << " does not provide a function to query the ABI version, "
                     "alpaqa_dl_abi_version_t "
                  << function_name << "_version(void)\n";
    }
    auto *register_func =
        reinterpret_cast<problem_register_t (*)(alpaqa_register_arg_t)>(
            util::load_func(handle.get(), function_name));
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

DLProblem::DLProblem(const std::filesystem::path &so_filename,
                     const std::string &function_name, std::any &user_param,
                     DynamicLoadFlags dl_flags)
    : DLProblem{so_filename, function_name,
                alpaqa_register_arg_t{reinterpret_cast<void *>(&user_param),
                                      alpaqa_register_arg_std_any},
                dl_flags} {}

DLProblem::DLProblem(const std::filesystem::path &so_filename,
                     const std::string &function_name,
                     std::span<std::string_view> user_param,
                     DynamicLoadFlags dl_flags)
    : DLProblem{so_filename, function_name,
                alpaqa_register_arg_t{reinterpret_cast<void *>(&user_param),
                                      alpaqa_register_arg_strings},
                dl_flags} {}

auto DLProblem::eval_projecting_difference_constraints(crvec z, rvec e) const -> void {
    if (functions->eval_projecting_difference_constraints)
        return functions->eval_projecting_difference_constraints(instance.get(), z.data(), e.data());
    return BoxConstrProblem<config_t>::eval_projecting_difference_constraints(z, e);
}

auto DLProblem::eval_projection_multipliers(rvec y, real_t M) const -> void {
    if (functions->eval_projection_multipliers)
        return functions->eval_projection_multipliers(instance.get(), y.data(), M);
    return BoxConstrProblem<config_t>::eval_projection_multipliers(y, M);
}

auto DLProblem::eval_proximal_gradient_step(real_t γ, crvec x, crvec grad_ψ, rvec x̂,
                                    rvec p) const -> real_t {
    if (functions->eval_proximal_gradient_step)
        return functions->eval_proximal_gradient_step(
            instance.get(), γ, x.data(), grad_ψ.data(), x̂.data(), p.data());
    return BoxConstrProblem<config_t>::eval_proximal_gradient_step(γ, x, grad_ψ, x̂, p);
}

auto DLProblem::eval_inactive_indices_res_lna(real_t γ, crvec x, crvec grad_ψ,
                                              rindexvec J) const -> index_t {
    if (functions->eval_inactive_indices_res_lna)
        return functions->eval_inactive_indices_res_lna(
            instance.get(), γ, x.data(), grad_ψ.data(), J.data());
    return BoxConstrProblem<config_t>::eval_inactive_indices_res_lna(γ, x,
                                                                     grad_ψ, J);
}

auto DLProblem::get_name() const -> std::string {
    if (functions->name)
        return functions->name;
    return file.filename().string();
}

// clang-format off
auto DLProblem::eval_objective(crvec x) const -> real_t { return functions->eval_objective(instance.get(), x.data()); }
auto DLProblem::eval_objective_gradient(crvec x, rvec grad_fx) const -> void { return functions->eval_objective_gradient(instance.get(), x.data(), grad_fx.data()); }
auto DLProblem::eval_constraints(crvec x, rvec gx) const -> void { return functions->eval_constraints(instance.get(), x.data(), gx.data()); }
auto DLProblem::eval_constraints_gradient_product(crvec x, crvec y, rvec grad_gxy) const -> void { return functions->eval_constraints_gradient_product(instance.get(), x.data(), y.data(), grad_gxy.data()); }
auto DLProblem::eval_grad_gi(crvec x, index_t i, rvec grad_gi) const -> void { return functions->eval_grad_gi(instance.get(), x.data(), i, grad_gi.data()); }
auto DLProblem::eval_constraints_jacobian(crvec x, rvec J_values) const -> void { return functions->eval_constraints_jacobian(instance.get(), x.data(), J_values.size() == 0 ? nullptr : J_values.data()); }
auto DLProblem::get_constraints_jacobian_sparsity() const -> Sparsity { return convert_sparsity<config_t>(functions->get_constraints_jacobian_sparsity(instance.get())); }
auto DLProblem::eval_lagrangian_hessian_product(crvec x, crvec y, real_t scale, crvec v, rvec Hv) const -> void { return functions->eval_lagrangian_hessian_product(instance.get(), x.data(), y.data(), scale, v.data(), Hv.data()); }
auto DLProblem::eval_lagrangian_hessian(crvec x, crvec y, real_t scale, rvec H_values) const -> void { return functions->eval_lagrangian_hessian(instance.get(), x.data(), y.data(), scale, H_values.size() == 0 ? nullptr : H_values.data()); }
auto DLProblem::get_lagrangian_hessian_sparsity() const -> Sparsity { return convert_sparsity<config_t>(functions->get_lagrangian_hessian_sparsity(instance.get())); }
auto DLProblem::eval_augmented_lagrangian_hessian_product(crvec x, crvec y, crvec Σ, real_t scale, crvec v, rvec Hv) const -> void { return functions->eval_augmented_lagrangian_hessian_product(instance.get(), x.data(), y.data(), Σ.data(), scale, D.lowerbound.data(), D.upperbound.data(), v.data(), Hv.data()); }
auto DLProblem::eval_augmented_lagrangian_hessian(crvec x, crvec y, crvec Σ, real_t scale, rvec H_values) const -> void { return functions->eval_augmented_lagrangian_hessian(instance.get(), x.data(), y.data(), Σ.data(), scale, D.lowerbound.data(), D.upperbound.data(), H_values.size() == 0 ? nullptr : H_values.data()); }
auto DLProblem::get_augmented_lagrangian_hessian_sparsity() const -> Sparsity { return convert_sparsity<config_t>(functions->get_augmented_lagrangian_hessian_sparsity(instance.get())); }
auto DLProblem::eval_objective_and_gradient(crvec x, rvec grad_fx) const -> real_t { return functions->eval_objective_and_gradient(instance.get(), x.data(), grad_fx.data()); }
auto DLProblem::eval_objective_and_constraints(crvec x, rvec g) const -> real_t { return functions->eval_objective_and_constraints(instance.get(), x.data(), g.data()); }
auto DLProblem::eval_objective_gradient_and_constraints_gradient_product(crvec x, crvec y, rvec grad_f, rvec grad_gxy) const -> void { return functions->eval_objective_gradient_and_constraints_gradient_product(instance.get(), x.data(), y.data(), grad_f.data(), grad_gxy.data()); }
auto DLProblem::eval_lagrangian_gradient(crvec x, crvec y, rvec grad_L, rvec work_n) const -> void { return functions->eval_lagrangian_gradient(instance.get(), x.data(), y.data(), grad_L.data(), work_n.data()); }
auto DLProblem::eval_augmented_lagrangian(crvec x, crvec y, crvec Σ, rvec ŷ) const -> real_t { return functions->eval_augmented_lagrangian(instance.get(), x.data(), y.data(), Σ.data(), D.lowerbound.data(), D.upperbound.data(), ŷ.data()); }
auto DLProblem::eval_augmented_lagrangian_gradient(crvec x, crvec y, crvec Σ, rvec grad_ψ, rvec work_n, rvec work_m) const -> void { return functions->eval_augmented_lagrangian_gradient(instance.get(), x.data(), y.data(), Σ.data(), D.lowerbound.data(), D.upperbound.data(), grad_ψ.data(), work_n.data(), work_m.data()); }
auto DLProblem::eval_augmented_lagrangian_and_gradient(crvec x, crvec y, crvec Σ, rvec grad_ψ, rvec work_n, rvec work_m) const -> real_t { return functions->eval_augmented_lagrangian_and_gradient(instance.get(), x.data(), y.data(), Σ.data(), D.lowerbound.data(), D.upperbound.data(), grad_ψ.data(), work_n.data(), work_m.data()); }

bool DLProblem::provides_eval_objective() const { return functions->eval_objective != nullptr; }
bool DLProblem::provides_eval_objective_gradient() const { return functions->eval_objective_gradient != nullptr; }
bool DLProblem::provides_eval_constraints() const { return functions->eval_constraints != nullptr; }
bool DLProblem::provides_eval_constraints_gradient_product() const { return functions->eval_constraints_gradient_product != nullptr; }
bool DLProblem::provides_eval_constraints_jacobian() const { return functions->eval_constraints_jacobian != nullptr; }
bool DLProblem::provides_get_constraints_jacobian_sparsity() const { return functions->get_constraints_jacobian_sparsity != nullptr; }
bool DLProblem::provides_eval_grad_gi() const { return functions->eval_grad_gi != nullptr; }
bool DLProblem::provides_eval_lagrangian_hessian_product() const { return functions->eval_lagrangian_hessian_product != nullptr; }
bool DLProblem::provides_eval_lagrangian_hessian() const { return functions->eval_lagrangian_hessian != nullptr; }
bool DLProblem::provides_get_lagrangian_hessian_sparsity() const { return functions->get_lagrangian_hessian_sparsity != nullptr; }
bool DLProblem::provides_eval_augmented_lagrangian_hessian_product() const { return functions->eval_augmented_lagrangian_hessian_product != nullptr; }
bool DLProblem::provides_eval_augmented_lagrangian_hessian() const { return functions->eval_augmented_lagrangian_hessian != nullptr; }
bool DLProblem::provides_get_augmented_lagrangian_hessian_sparsity() const { return functions->get_augmented_lagrangian_hessian_sparsity != nullptr; }
bool DLProblem::provides_eval_objective_and_gradient() const { return functions->eval_objective_and_gradient != nullptr; }
bool DLProblem::provides_eval_objective_and_constraints() const { return functions->eval_objective_and_constraints != nullptr; }
bool DLProblem::provides_eval_objective_gradient_and_constraints_gradient_product() const { return functions->eval_objective_gradient_and_constraints_gradient_product != nullptr; }
bool DLProblem::provides_eval_lagrangian_gradient() const { return functions->eval_lagrangian_gradient != nullptr; }
bool DLProblem::provides_eval_augmented_lagrangian() const { return functions->eval_augmented_lagrangian != nullptr; }
bool DLProblem::provides_eval_augmented_lagrangian_gradient() const { return functions->eval_augmented_lagrangian_gradient != nullptr; }
bool DLProblem::provides_eval_augmented_lagrangian_and_gradient() const { return functions->eval_augmented_lagrangian_and_gradient != nullptr; }
bool DLProblem::provides_get_box_variables() const { return functions->eval_proximal_gradient_step == nullptr && BoxConstrProblem::provides_get_box_variables(); }
bool DLProblem::provides_get_box_general_constraints() const { return functions->eval_projecting_difference_constraints == nullptr; }
bool DLProblem::provides_eval_inactive_indices_res_lna() const { return functions->eval_proximal_gradient_step == nullptr || functions->eval_inactive_indices_res_lna != nullptr; }
// clang-format on

#if ALPAQA_WITH_OCP

DLControlProblem::DLControlProblem(const std::filesystem::path &so_filename,
                                   const std::string &function_name,
                                   alpaqa_register_arg_t user_param,
                                   DynamicLoadFlags dl_flags) {
    if (so_filename.empty())
        throw std::invalid_argument("Invalid problem filename");
    handle = util::load_lib(so_filename, dl_flags);
    try {
        auto *version_func = reinterpret_cast<alpaqa_dl_abi_version_t (*)()>(
            util::load_func(handle.get(), function_name + "_version"));
        check_abi_version(version_func());
    } catch (const dynamic_load_error &) {
        std::cerr << "Warning: problem " << so_filename
                  << " does not provide a function to query the ABI version, "
                     "alpaqa_dl_abi_version_t "
                  << function_name << "_version(void)\n";
    }
    auto *register_func =
        reinterpret_cast<control_problem_register_t (*)(alpaqa_register_arg_t)>(
            util::load_func(handle.get(), function_name));
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