# Problem formulations {#page-problem-formulations}

[TOC]

## General NLP formulation {#problem-formulations-general}

Most alpaqa solvers deal with problems in the following form:

@f[
\begin{equation}\tag{P}\label{eq:problem_main}
    \begin{aligned}
        & \underset{x}{\text{minimize}}
        & & f(x) &&&& f : \Rn \rightarrow \R \\
        & \text{subject to}
        & & \underline{x} \le x \le \overline{x} \\
        &&& \underline{z} \le g(x) \le \overline{z} &&&& g : \Rn \rightarrow \Rm
    \end{aligned}
\end{equation}
@f]

@f$ f @f$ is called the cost or objective function, and @f$ g @f$ is the
constraints function.

The solver needs to be able to evaluate the following required functions and
derivatives:
  - @ref alpaqa::TypeErasedProblem::eval_f "eval_f"                     @f$ : \Rn \to \R : x \mapsto f(x) @f$ (objective function)
  - @ref alpaqa::TypeErasedProblem::eval_grad_f "eval_grad_f"           @f$ : \Rn \to \Rn : x \mapsto \nabla f(x) @f$ (gradient of the objective)
  - @ref alpaqa::TypeErasedProblem::eval_g "eval_g"                     @f$ : \Rn \to \Rm : x \mapsto g(x) @f$ (constraint function)
  - @ref alpaqa::TypeErasedProblem::eval_grad_g_prod "eval_grad_g_prod" @f$ : \Rn \times \Rm \to \Rn : (x, y) \mapsto \nabla g(x)\, y @f$ (gradient-vector product of the constraints)

Usually, [automatic differentiation](https://en.wikipedia.org/wiki/Automatic_differentiation)
(AD) is used to evaluate the gradients and gradient-vector products. Many AD
software packages are available, see e.g. <https://autodiff.org/> for an overview.

Additionally, the solver needs to be able to project onto the rectangular sets
@f[
\begin{equation}
    \begin{aligned}
        C &\;\defeq\; \defset{x \in \Rn}{\underline{x} \le x \le \overline{x}}, \\
        D &\;\defeq\; \defset{z \in \Rm}{\underline{z} \le z \le \overline{z}}.
    \end{aligned}
\end{equation}
@f]

<!-- Given two boxes @f$ C @f$ and @f$ D @f$, the @ref alpaqa::BoxConstrProblem "BoxConstrProblem"
class provides default implementations for the necessary projections:
@ref alpaqa::TypeErasedProblem::eval_proj_diff_g "eval_proj_diff_g",
@ref alpaqa::TypeErasedProblem::eval_proj_multipliers "eval_proj_multipliers" and
@ref alpaqa::TypeErasedProblem::eval_prox_grad_step "eval_prox_grad_step". -->

## Problem API

The alpaqa solvers access the problem functions through the API outlined in
@ref alpaqa::TypeErasedProblem.  
Usually, problems are defined using C++ structs, providing the evaluations
described above as public member functions. These problem structs are
[structurally typed](https://en.wikipedia.org/wiki/Structural_type_system),
which means that they only need to provide member functions with the correct
names and signatures. Inheriting from a common base class is not required.

As an example, the following struct defines a problem that can be passed to the
alpaqa solvers. Detailed descriptions of each function can be found in the
@ref alpaqa::TypeErasedProblem documentation.

```cpp
struct RosenbrockProblem {
    USING_ALPAQA_CONFIG(alpaqa::DefaultConfig);
    // Problem dimensions
    length_t get_n() const; // number of unknowns
    length_t get_m() const; // number of general constraints

    // Cost
    real_t eval_f(crvec x) const;
    // Gradient of cost
    void eval_grad_f(crvec x, rvec grad_fx) const;

    // Constraints
    void eval_g(crvec x, rvec gx) const;
    // Gradient-vector product of constraints
    void eval_grad_g_prod(crvec x, crvec y, rvec grad_gxy) const;

    // Proximal gradient step
    real_t eval_prox_grad_step(real_t γ, crvec x, crvec grad, rvec x̂, rvec p) const;
    // Projecting difference onto constraint set D
    void eval_proj_diff_g(crvec z, rvec p) const;
    // Projection of Lagrange multipliers
    void eval_proj_multipliers(rvec y, real_t max_y) const;
};
```

### Base classes for common use cases

Convenience classes with default implementations of some of these functions are
provided for common use cases:
  - @ref alpaqa::BoxConstrProblem "BoxConstrProblem" defines the 
    @ref alpaqa::TypeErasedProblem::eval_proj_diff_g "eval_proj_diff_g",
    @ref alpaqa::TypeErasedProblem::eval_proj_multipliers "eval_proj_multipliers" and
    @ref alpaqa::TypeErasedProblem::eval_prox_grad_step "eval_prox_grad_step" functions
    for the specific case where @f$ C @f$ and @f$ D @f$ are rectangular boxes,
    as in @f$ \eqref{eq:problem_main} @f$.
  - @ref alpaqa::UnconstrProblem "UnconstrProblem" defines those same functions,
    and additional empty implementations of constraint-related functions to
    allow a more concise formulation of unconstrained problems.

The user can simply inherit from these classes to inject the default
implementations into their problem definition, as demonstrated in the following
examples.
  - @ref C++/CustomCppProblem/main.cpp
  - @ref C++/SimpleUnconstrProblem/main.cpp

It is highly recommended to study the @ref C++/CustomCppProblem/main.cpp example
now to see how optimization problems can be formulated in practice, before we
continue with some more specialized use cases.

### Second-order derivatives

Some solvers can exploit information about the Hessian of the (augmented)
Lagrangian of the problem. To use these solvers, some of the following functions
are required, they should be added as member functions to your problem struct.
  - @ref alpaqa::TypeErasedProblem::eval_jac_g "eval_jac_g": Jacobian matrix of the constraints
  - @ref alpaqa::TypeErasedProblem::get_jac_g_sparsity "get_jac_g_sparsity": sparsity pattern of the Jacobian of the constraints
  - @ref alpaqa::TypeErasedProblem::eval_grad_gi "eval_grad_gi": gradient of a specific constraint
  - @ref alpaqa::TypeErasedProblem::eval_hess_L_prod "eval_hess_L_prod": Hessian-vector product of the Lagrangian
  - @ref alpaqa::TypeErasedProblem::eval_hess_L "eval_hess_L": Hessian matrix of the Lagrangian
  - @ref alpaqa::TypeErasedProblem::get_hess_L_sparsity "get_hess_L_sparsity": sparsity pattern of the Hessian of the Lagrangian
  - @ref alpaqa::TypeErasedProblem::eval_hess_ψ_prod "eval_hess_ψ_prod": Hessian-vector product of the Hessian of the augmented Lagrangian
  - @ref alpaqa::TypeErasedProblem::eval_hess_ψ "eval_hess_ψ": Hessian matrix of the augmented Lagrangian
  - @ref alpaqa::TypeErasedProblem::get_hess_ψ_sparsity "get_hess_ψ_sparsity": sparsity pattern of the Hessian of the augmented Lagrangian

Matrices can be stored in a dense format, in [compressed sparse column storage](https://www.eigen.tuxfamily.org/dox/group__TutorialSparse.html#TutorialSparseIntro)
(CCS) format, or in sparse coordinate list format (COO). Solvers convert the
input to a format that they support, so some performance could be gained by
choosing the appropriate storage type, because conversions may involve sorting
indices and permuting the nonzero values. See @ref alpaqa::sparsity for details.
For sparse symmetric Hessian matrices, only the upper-triangular part is stored.
Dense matrices are always stored in full, even if they are symmetric.
The matrix evaluation functions only overwrite the nonzero values, vectorized
by column.

Some solvers do not require the full Hessian matrices, but use Hessian-vector
products only, for example when using Newton-CG. These products can often be
computed efficiently using automatic differentiation, at a computational cost
that's not much higher than a gradient evaluation.

The @ref alpaqa::TypeErasedProblem "TypeErasedProblem" class provides functions
to query which optional problem functions are available. For example,
@ref alpaqa::TypeErasedProblem::provides_eval_jac_g "provides_eval_jac_g"
returns true if the problem provides an implementation for
@ref alpaqa::TypeErasedProblem::eval_jac_g "eval_jac_g". Calling an optional
function that is not provided results in an @ref alpaqa::not_implemented_error
exception being thrown.

### Specialized combined evaluations

In practice, the solvers do not always evaluate the functions @f$ f(x) @f$ and
@f$ g(x) @f$ directly. Instead, they evaluate the Lagrangian and augmented
Lagrangian functions of the problem. In many applications, such as
single-shooting optimal control problems, some computations are common to the
evaluation of both @f$ f(x) @f$ and @f$ g(x) @f$, and significant speedups can
be achieved by providing implementations that evaluate both at the same time,
or even compute the (augmented) Lagrangian directly. Similarly, when using
automatic differentiation, evaluation of the gradient @f$ \nabla f(x) @f$
produces the function value @f$ f(x) @f$ as a byproduct, motivating the
simultaneous evaluation of these quantities as well.

The full list of these combined evaluations can be found in the @ref alpaqa::TypeErasedProblem "TypeErasedProblem"
documentation. They can be provided in the same fashion as `eval_f` above.
  - @ref alpaqa::TypeErasedProblem::eval_f_grad_f "eval_f_grad_f": @f$ f(x) @f$ and @f$ \nabla f(x) @f$
  - @ref alpaqa::TypeErasedProblem::eval_f_g "eval_f_g": @f$ f(x) @f$ and @f$ g(x) @f$
  - @ref alpaqa::TypeErasedProblem::eval_grad_f_grad_g_prod "eval_grad_f_grad_g_prod": @f$ \nabla f(x) @f$ and @f$ \nabla g(x)\,y @f$
  - @ref alpaqa::TypeErasedProblem::eval_grad_L "eval_grad_L": gradient of the Lagrangian: @f$ \nabla_{\!x} L(x, y) = \nabla f(x) + \nabla g(x)\,y @f$
  - @ref alpaqa::TypeErasedProblem::eval_ψ "eval_ψ": augmented Lagrangian: @f$ \psi(x) @f$
  - @ref alpaqa::TypeErasedProblem::eval_grad_ψ "eval_grad_ψ": gradient of the augmented Lagrangian: @f$ \nabla \psi(x) @f$
  - @ref alpaqa::TypeErasedProblem::eval_ψ_grad_ψ "eval_ψ_grad_ψ": augmented Lagrangian and gradient: @f$ \psi(x) @f$ and @f$ \nabla \psi(x) @f$

### Proximal operators

In addition to standard box constraints on the variables, some solvers also
allow the addition of a possibly non-smooth, proximal term to the objective.

@f[
\begin{equation}\tag{P-prox}\label{eq:problem_prox}
    \begin{aligned}
        & \underset{x}{\text{minimize}}
        & & f(x) + h(x) &&&& f : \Rn \rightarrow \R,\;\; h : \Rn \rightarrow \overline{\R} \\
        & \text{subject to}
        & & \underline{z} \le g(x) \le \overline{z} &&&& g : \Rn \rightarrow \Rm
    \end{aligned}
\end{equation}
@f]
By selecting
@f[ h(x) = \delta_C(x) \;\defeq\; \begin{cases}
0 & \text{if } x \in C \\ +\infty & \text{otherwise,} \end{cases} @f]
the standard NLP formulation @f$ \eqref{eq:problem_main} @f$ is recovered.

To add a custom function @f$ h(x) @f$ to the problem formulation, it suffices to
implement the @ref alpaqa::TypeErasedProblem::eval_prox_grad_step "eval_prox_grad_step"
function, which computes a forward-backward step
@f$ p = \prox_{\gamma h}\big(x - \gamma \nabla \psi(x)\big) - x @f$, where the
current iterate @f$ x @f$, the gradient @f$ \nabla \psi(x) @f$ and a positive
step size @f$ \gamma @f$ are given, and where @f$ \prox_{\gamma h}(z) \;\defeq\;
\argmin_x \left\{ h(x) + \tfrac{1}{2\gamma} \normsq{x - z} \right\} @f$ denotes
the proximal operator of @f$ h @f$ with step size @f$ \gamma @f$.

Note that in general, combining an arbitrary function @f$ h(x) @f$ with the box
constraints @f$ x \in C @f$ is not possible. One notable exception is the
@f$ \ell_1 @f$-norm @f$ h(x) = \lambda\norm{x}_1 @f$. This choice for @f$ h @f$,
in combination with the box constraints, is supported by the
@ref alpaqa::BoxConstrProblem class, by setting the
@ref alpaqa::BoxConstrProblem::l1_reg member.

The @ref alpaqa::prox_step utility function can be used to implement
@ref alpaqa::TypeErasedProblem::eval_prox_grad_step "eval_prox_grad_step". See
@ref grp_Functions for details.

## Dynamically loading problems

alpaqa has a dependency-free, single-header C API that can be used to define
problems in a shared library that can be dynamically loaded by the solvers.

The API is defined in @ref dl-problem.h. The main entry point of your shared
object should be a function called `register_alpaqa_problem` that returns a
struct of type @ref alpaqa_problem_register_t. This struct contains a pointer to
the problem instance, a function pointer that will be called to clean up the
problem instance, and a pointer to a struct of type
@ref alpaqa_problem_functions_t, which contains function pointers to all problem
functions.

Additional user-defined arguments can be passed through a parameter with type
@ref alpaqa_register_arg_t of the `register_alpaqa_problem` function.

In C++, you could register a problem like this:

```cpp
#include <alpaqa/dl/dl-problem.h>
using real_t = alpaqa_real_t;

/// Custom problem class to expose to alpaqa.
struct Problem {
    alpaqa_problem_functions_t funcs{};

    real_t eval_f(const real_t *x_) const;
    void eval_grad_f(const real_t *x_, real_t *gr_) const;
    void eval_g(const real_t *x_, real_t *g_) const;
    void eval_grad_g_prod(const real_t *x_, const real_t *y_, real_t *gr_) const;
    void initialize_box_C(real_t *lb_, real_t *ub_) const;
    void initialize_box_D(real_t *lb_, real_t *ub_) const;

    std::string get_name() const { return "example problem"; }

    /// Constructor initializes the problem and exposes the problem functions.
    Problem(/* ... */) {
        using alpaqa::member_caller;
        funcs.n                = 3; // number of variables
        funcs.m                = 2; // number of constraints
        funcs.eval_f           = member_caller<&Problem::eval_f>();
        funcs.eval_grad_f      = member_caller<&Problem::eval_grad_f>();
        funcs.eval_g           = member_caller<&Problem::eval_g>();
        funcs.eval_grad_g_prod = member_caller<&Problem::eval_grad_g_prod>();
        funcs.initialize_box_C = member_caller<&Problem::initialize_box_C>();
        funcs.initialize_box_D = member_caller<&Problem::initialize_box_D>();
    }
};

/// Main entry point: called by the @ref alpaqa::dl::DLProblem class.
extern "C" alpaqa_problem_register_t
register_alpaqa_problem(alpaqa_register_arg_t user_data) noexcept try {
    auto problem = std::make_unique<Problem>(/* ... */);
    alpaqa_problem_register_t result;
    alpaqa::register_member_function(result, "get_name", &Problem::get_name);
    result.functions = &problem->funcs;
    result.instance  = problem.release();
    result.cleanup   = [](void *instance) { delete static_cast<Problem *>(instance); };
    return result;
} catch (...) {
    return {.exception = new alpaqa_exception_ptr_t{std::current_exception()}};
}

/// Used by @ref alpaqa::dl::DLProblem to ensure binary compatibility.
extern "C" alpaqa_dl_abi_version_t
register_alpaqa_problem_version() { return ALPAQA_DL_ABI_VERSION; }
```

A full example can be found in @ref problems/sparse-logistic-regression.cpp.
While defining the `register_alpaqa_problem` in C++ is usually much more
ergonomic than in plain C, the latter is also supported, as demonstrated in
@ref C++/DLProblem/main.cpp.

The problem can then be loaded using the @ref alpaqa::dl::DLProblem class,
or using the `alpaqa-driver` command line interface. For more details, see the
two examples mentioned previously.

## Existing problem adapters

For interoperability with existing frameworks like [CasADi](https://web.casadi.org/)
and [CUTEst](https://github.com/ralna/CUTEst), alpaqa provides the following
problem adapters:

 - @ref alpaqa::CasADiProblem
 - @ref alpaqa::CUTEstProblem

@see @ref grp_Problems topic
