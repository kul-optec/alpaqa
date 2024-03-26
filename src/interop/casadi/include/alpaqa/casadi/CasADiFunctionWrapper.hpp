#pragma once

#include <alpaqa/casadi-loader-export.h>
#include <alpaqa/casadi/casadi-namespace.hpp>
#include <alpaqa/config/config.hpp>

#include <stdexcept>
#include <string>
#include <vector>

#if ALPAQA_WITH_EXTERNAL_CASADI
#include <casadi/core/function.hpp>
#include <casadi/mem.h>
#else
#include <alpaqa/casadi/casadi-external-function.hpp>
#endif

namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE::casadi_loader {

struct CASADI_LOADER_EXPORT invalid_argument_dimensions
    : std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

/// Class for evaluating CasADi functions, allocating the necessary workspace
/// storage in advance for allocation-free evaluations.
template <Config Conf, size_t N_in, size_t N_out>
class CasADiFunctionEvaluator {
  public:
    USING_ALPAQA_CONFIG(Conf);
    static_assert(std::is_same_v<real_t, casadi_real>);

    using casadi_dim = std::pair<casadi_int, casadi_int>;

    /// @throws invalid_argument_dimensions
    CasADiFunctionEvaluator(casadi::Function &&f)
#if ALPAQA_WITH_EXTERNAL_CASADI
        : fun(std::move(f)), iwork(fun.sz_iw()), dwork(fun.sz_w()),
          arg_work(fun.sz_arg()), res_work(fun.sz_res())
#else
        : fun(std::move(f))
#endif
    {
        validate_num_args(fun);
    }

    /// @throws invalid_argument_dimensions
    CasADiFunctionEvaluator(casadi::Function &&f,
                            const std::array<casadi_dim, N_in> &dim_in,
                            const std::array<casadi_dim, N_out> &dim_out)
        : CasADiFunctionEvaluator{std::move(f)} {
        validate_dimensions(dim_in, dim_out);
    }

    /// @throws invalid_argument_dimensions
    static void validate_num_args(const casadi::Function &fun) {
        using namespace std::literals::string_literals;
        if (N_in != fun.n_in())
            throw invalid_argument_dimensions(
                "Invalid number of input arguments: got "s +
                std::to_string(fun.n_in()) + ", should be " +
                std::to_string(N_in) + ".");
        if (N_out != fun.n_out())
            throw invalid_argument_dimensions(
                "Invalid number of output arguments: got "s +
                std::to_string(fun.n_out()) + ", should be " +
                std::to_string(N_out) + ".");
    }

    /// @throws invalid_argument_dimensions
    static void
    validate_dimensions(const casadi::Function &fun,
                        const std::array<casadi_dim, N_in> &dim_in   = {},
                        const std::array<casadi_dim, N_out> &dim_out = {}) {
        using namespace std::literals::string_literals;
        static constexpr std::array count{"first",   "second", "third",
                                          "fourth",  "fifth",  "sixth",
                                          "seventh", "eighth"};
        static_assert(N_in <= count.size());
        static_assert(N_out <= count.size());
        auto to_string = [](casadi_dim d) {
            return "(" + std::to_string(d.first) + ", " +
                   std::to_string(d.second) + ")";
        };
        for (size_t n = 0; n < N_in; ++n) {
            auto cs_n = static_cast<casadi_int>(n);
            if (dim_in[n].first != 0 && dim_in[n] != fun.size_in(cs_n))
                throw invalid_argument_dimensions(
                    "Invalid dimension of "s + count[n] +
                    " input argument: got " + to_string(fun.size_in(cs_n)) +
                    ", should be " + to_string(dim_in[n]) + ".");
        }
        for (size_t n = 0; n < N_out; ++n) {
            auto cs_n = static_cast<casadi_int>(n);
            if (dim_out[n].first != 0 && dim_out[n] != fun.size_out(cs_n))
                throw invalid_argument_dimensions(
                    "Invalid dimension of "s + count[n] +
                    " output argument: got " + to_string(fun.size_out(cs_n)) +
                    ", should be " + to_string(dim_out[n]) + ".");
        }
    }

    /// @throws invalid_argument_dimensions
    void
    validate_dimensions(const std::array<casadi_dim, N_in> &dim_in   = {},
                        const std::array<casadi_dim, N_out> &dim_out = {}) {
        validate_dimensions(fun, dim_in, dim_out);
    }

#if ALPAQA_WITH_EXTERNAL_CASADI
  protected:
    void operator()(const double *const *in, double *const *out) const {
        std::copy_n(in, N_in, arg_work.begin());
        std::copy_n(out, N_out, res_work.begin());
        fun(arg_work.data(), res_work.data(), iwork.data(), dwork.data(), 0);
    }

  public:
    void operator()(const double *const (&in)[N_in],
                    double *const (&out)[N_out]) const {
        this->operator()(&in[0], &out[0]);
    }
#else
  public:
    void operator()(const double *const (&in)[N_in],
                    double *const (&out)[N_out]) {
        fun(std::span{in}, std::span{out});
    }
#endif

  public:
    casadi::Function fun;

#if ALPAQA_WITH_EXTERNAL_CASADI
  private:
    mutable std::vector<casadi_int> iwork;
    mutable std::vector<double> dwork;
    mutable std::vector<const double *> arg_work;
    mutable std::vector<double *> res_work;
#endif
};

} // namespace alpaqa::inline ALPAQA_CASADI_LOADER_NAMESPACE::casadi_loader
