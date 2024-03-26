#include <alpaqa/casadi/casadi-external-function.hpp>
#include <alpaqa/casadi/casadi-namespace.hpp>
#include <alpaqa/util/dl.hpp>

namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE::casadi {

void Function::load(void *so_handle, const std::string &func_name) {
    functions = {
        .incref       = fname_incref::load(so_handle, func_name),
        .decref       = fname_decref::load(so_handle, func_name),
        .n_in         = fname_n_in::load(so_handle, func_name),
        .n_out        = fname_n_out::load(so_handle, func_name),
        .name_in      = fname_name_in::load(so_handle, func_name),
        .name_out     = fname_name_out::load(so_handle, func_name),
        .sparsity_in  = fname_sparsity_in::load(so_handle, func_name),
        .sparsity_out = fname_sparsity_out::load(so_handle, func_name),
        .alloc_mem    = fname_alloc_mem::load(so_handle, func_name),
        .init_mem     = fname_init_mem::load(so_handle, func_name),
        .free_mem     = fname_free_mem::load(so_handle, func_name),
        .work         = fname_work::load(so_handle, func_name),
        .call         = fname::load(so_handle, func_name),
    };
    functions.incref();
}

void Function::init_work() {
    if (work)
        return;
    mem = functions.alloc_mem();
    functions.init_mem(mem); // TODO: what to do upon failure?
    casadi_int sz_arg, sz_res, sz_iw, sz_w;
    functions.work(&sz_arg, &sz_res, &sz_iw, &sz_w);
    auto &w = work.emplace();
    w.arg.resize(static_cast<size_t>(sz_arg));
    w.res.resize(static_cast<size_t>(sz_res));
    w.iw.resize(static_cast<size_t>(sz_iw));
    w.w.resize(static_cast<size_t>(sz_w));
}

Function::Function(std::shared_ptr<void> so_handle,
                   const std::string &func_name)
    : so_handle{std::move(so_handle)} {
    load(this->so_handle.get(), func_name);
}
Function::Function(const Function &o)
    : so_handle{o.so_handle}, functions{o.functions} {
    functions.incref();
}
Function::Function(Function &&o) noexcept
    : so_handle{std::move(o.so_handle)}, functions{o.functions},
      work{std::move(o.work)}, mem{std::exchange(o.mem, nullptr)} {}
Function::~Function() {
    if (so_handle) {
        if (mem)
            functions.free_mem(mem);
        functions.decref();
    }
}

casadi_int Function::n_in() const { return functions.n_in(); }
casadi_int Function::n_out() const { return functions.n_out(); }
std::pair<casadi_int, casadi_int> Function::size_in(casadi_int n) const {
    return sparsity_in(n).size();
}
std::pair<casadi_int, casadi_int> Function::size_out(casadi_int n) const {
    return sparsity_out(n).size();
}
casadi_int Function::size1_in(casadi_int n) const {
    return sparsity_in(n).size1();
}
casadi_int Function::size1_out(casadi_int n) const {
    return sparsity_out(n).size1();
}
casadi_int Function::size2_in(casadi_int n) const {
    return sparsity_in(n).size2();
}
casadi_int Function::size2_out(casadi_int n) const {
    return sparsity_out(n).size2();
}
Function::Sparsity Function::sparsity_in(casadi_int n) const {
    return Sparsity{functions.sparsity_in(n)};
}
Function::Sparsity Function::sparsity_out(casadi_int n) const {
    return Sparsity{functions.sparsity_out(n)};
}

Function external(const std::string &name, const std::string &bin_name) {
    return Function{util::load_lib(std::filesystem::path{bin_name}), name};
}

} // namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE::casadi
