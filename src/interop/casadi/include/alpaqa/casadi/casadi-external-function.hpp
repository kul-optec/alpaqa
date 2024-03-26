#pragma once

#include <alpaqa/casadi-loader-export.h>
#include "casadi-functions.hpp"
#include "casadi-namespace.hpp"

#include <cassert>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE::casadi {

/// Class that loads and calls pre-compiled CasADi functions in a DLL/SO file.
/// Designed to match (part of) the `casadi::Function` API.
class CASADI_LOADER_EXPORT Function {
  public:
    Function(std::shared_ptr<void> so_handle, const std::string &func_name);
    Function(const Function &);
    Function(Function &&) noexcept;
    ~Function();

  public:
    class CASADI_LOADER_EXPORT Sparsity {
      public:
        [[nodiscard]] std::pair<casadi_int, casadi_int> size() const;
        [[nodiscard]] casadi_int size1() const;
        [[nodiscard]] casadi_int size2() const;
        [[nodiscard]] casadi_int nnz() const;
        [[nodiscard]] bool is_dense() const;
        [[nodiscard]] const casadi_int *row() const;
        [[nodiscard]] const casadi_int *colind() const;

      private:
        friend class Function;
        explicit Sparsity(const casadi_int *data) : data{data} {}
        const casadi_int *data;
    };
    [[nodiscard]] casadi_int n_in() const;
    [[nodiscard]] casadi_int n_out() const;
    [[nodiscard]] std::pair<casadi_int, casadi_int> size_in(casadi_int) const;
    [[nodiscard]] std::pair<casadi_int, casadi_int> size_out(casadi_int) const;
    [[nodiscard]] casadi_int size1_in(casadi_int) const;
    [[nodiscard]] casadi_int size1_out(casadi_int) const;
    [[nodiscard]] casadi_int size2_in(casadi_int) const;
    [[nodiscard]] casadi_int size2_out(casadi_int) const;
    [[nodiscard]] Sparsity sparsity_in(casadi_int) const;
    [[nodiscard]] Sparsity sparsity_out(casadi_int) const;

  public:
    void operator()(std::span<const double *const> arg,
                    std::span<double *const> res) {
        if (arg.size() != static_cast<size_t>(n_in()))
            throw std::invalid_argument("Wrong number of arguments to CasADi "
                                        "function");
        if (res.size() != static_cast<size_t>(n_out()))
            throw std::invalid_argument("Wrong number of outputs to CasADi "
                                        "function");
        init_work();
        assert(work);
        std::ranges::copy(arg, work->arg.begin());
        std::ranges::copy(res, work->res.begin());
        functions.call(work->arg.data(), work->res.data(), work->iw.data(),
                       work->w.data(), mem);
        // TODO: what to do upon failure?
    }

  private:
    void load(void *so_handle, const std::string &func_name);
    void init_work();

  private:
    std::shared_ptr<void> so_handle;
    struct Functions {
        fname_incref::signature_t *incref             = nullptr;
        fname_decref::signature_t *decref             = nullptr;
        fname_n_in::signature_t *n_in                 = nullptr;
        fname_n_out::signature_t *n_out               = nullptr;
        fname_name_in::signature_t *name_in           = nullptr;
        fname_name_out::signature_t *name_out         = nullptr;
        fname_sparsity_in::signature_t *sparsity_in   = nullptr;
        fname_sparsity_out::signature_t *sparsity_out = nullptr;
        fname_alloc_mem::signature_t *alloc_mem       = nullptr;
        fname_init_mem::signature_t *init_mem         = nullptr;
        fname_free_mem::signature_t *free_mem         = nullptr;
        fname_work::signature_t *work                 = nullptr;
        fname::signature_t *call                      = nullptr;
    } functions;
    struct Work {
        std::vector<const casadi_real *> arg;
        std::vector<casadi_real *> res;
        std::vector<casadi_int> iw;
        std::vector<casadi_real> w;
    };
    std::optional<Work> work;
    void *mem = nullptr;
};

inline std::pair<casadi_int, casadi_int> Function::Sparsity::size() const {
    return {size1(), size2()};
}
inline casadi_int Function::Sparsity::size1() const { return data[0]; }
inline casadi_int Function::Sparsity::size2() const { return data[1]; }
inline casadi_int Function::Sparsity::nnz() const { return colind()[size2()]; }
inline bool Function::Sparsity::is_dense() const { return data[2] != 0; }
inline const casadi_int *Function::Sparsity::row() const {
    return &colind()[size2() + 1];
}
inline const casadi_int *Function::Sparsity::colind() const {
    assert(!is_dense());
    return &data[2];
}

/// Load the given CasADi function from the given DLL/SO file.
CASADI_LOADER_EXPORT Function external(const std::string &name,
                                       const std::string &bin_name);

} // namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE::casadi
