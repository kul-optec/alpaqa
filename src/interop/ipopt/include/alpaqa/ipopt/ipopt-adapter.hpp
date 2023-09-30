#pragma once

#include <alpaqa/ipopt-adapter-export.h>
#include <alpaqa/problem/sparsity-conversions.hpp>
#include <alpaqa/problem/sparsity.hpp>
#include <alpaqa/problem/type-erased-problem.hpp>

#include <IpTNLP.hpp>

namespace alpaqa {

/// Based on https://coin-or.github.io/Ipopt/INTERFACES.html
class IPOPT_ADAPTER_EXPORT IpoptAdapter : public Ipopt::TNLP {
  public:
    USING_ALPAQA_CONFIG(EigenConfigd);
    using Sparsity = sparsity::Sparsity<config_t>;
    using Problem  = TypeErasedProblem<config_t>;
    const Problem &problem;
    vec initial_guess;
    vec initial_guess_bounds_multipliers_l;
    vec initial_guess_bounds_multipliers_u;
    vec initial_guess_multipliers;
    using Index  = Ipopt::Index;
    using Number = Ipopt::Number;

    struct Results {
        Ipopt::SolverReturn status = Ipopt::SolverReturn::UNASSIGNED;
        vec solution_x, solution_z_L, solution_z_U, solution_y, solution_g;
        real_t solution_f = NaN<config_t>, infeasibility = NaN<config_t>,
               nlp_error    = NaN<config_t>;
        length_t iter_count = 0;
    } results;

    IpoptAdapter(const Problem &problem);
    IpoptAdapter(Problem &&) = delete;

    /// @name Functions required by Ipopt
    /// @{

    bool get_nlp_info(Index &n, Index &m, Index &nnz_jac_g, Index &nnz_h_lag,
                      IndexStyleEnum &index_style) override;

    bool get_bounds_info(Index n, Number *x_l, Number *x_u, Index m,
                         Number *g_l, Number *g_u) override;

    bool get_starting_point(Index n, bool init_x, Number *x, bool init_z,
                            Number *z_L, Number *z_U, Index m, bool init_lambda,
                            Number *lambda) override;

    bool eval_f(Index n, const Number *x, bool new_x,
                Number &obj_value) override;

    bool eval_grad_f(Index n, const Number *x, bool new_x,
                     Number *grad_f) override;

    bool eval_g(Index n, const Number *x, bool new_x, Index m,
                Number *g) override;

    bool eval_jac_g(Index n, const Number *x, bool new_x, Index m,
                    Index nele_jac, Index *iRow, Index *jCol,
                    Number *values) override;

    bool eval_h(Index n, const Number *x, bool new_x, Number obj_factor,
                Index m, const Number *lambda, bool new_lambda, Index nele_hess,
                Index *iRow, Index *jCol, Number *values) override;

    void finalize_solution(Ipopt::SolverReturn status, Index n, const Number *x,
                           const Number *z_L, const Number *z_U, Index m,
                           const Number *g, const Number *lambda,
                           Number obj_value, const Ipopt::IpoptData *ip_data,
                           Ipopt::IpoptCalculatedQuantities *ip_cq) override;

    /// @}

  private:
    using SparsityConv =
        sparsity::SparsityConverter<Sparsity,
                                    sparsity::SparseCOO<config_t, Index>>;
    Sparsity orig_sparsity_jac_g     = problem.get_jac_g_sparsity(),
             orig_sparsity_hess_L    = problem.get_hess_L_sparsity();
    SparsityConv cvt_sparsity_jac_g  = orig_sparsity_jac_g,
                 cvt_sparsity_hess_L = orig_sparsity_hess_L;
};

} // namespace alpaqa