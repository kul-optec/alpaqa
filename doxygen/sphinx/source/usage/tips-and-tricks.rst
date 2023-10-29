.. include:: ../shared/definitions.rst

.. _Tips and tricks:

Tips and tricks
===============

Problem formulation
-------------------

There are two main ways to formulate an optimization problem:

1. Using symbolic `CasADi <https://web.casadi.org/>`_ expressions.
2. By creating a class that implements the necessary methods for evaluating
   problem functions.

To decide which formulation is most suitable for your application, it helps to
consider a few different aspects:

+-------------------------------------------------------+-------------------------------------------------------+
| CasADi                                                | Classes                                               |
+=======================================================+=======================================================+
| Declarative                                           | Imperative                                            |
+-------------------------------------------------------+-------------------------------------------------------+
| Intuitive symbolic expressions                        | Any Python code (NumPy, JAX, TensorFlow, PyTorch ...) |
+-------------------------------------------------------+-------------------------------------------------------+
| Automatic computation of derivatives                  | Requires explicit methods for evaluating derivatives  |
+-------------------------------------------------------+-------------------------------------------------------+
| Pre-compilation and caching for excellent performance | Overhead because of calls to Python functions         |
+-------------------------------------------------------+-------------------------------------------------------+

Going beyond Python, there are other ways to formulate problems that are
covered in the Doxygen documentation:

* `Doxygen: Problem formulations <../../Doxygen/page-problem-formulations.html>`_
* `Doxygen: Problems topic <../../Doxygen/group__grp__Problems.html>`_
* `Doxygen: C++/CustopCppProblem example <../../Doxygen/C_09_09_2CustomCppProblem_2main_8cpp-example.html>`_
* `Doxygen: C++/DLProblem example <../../Doxygen/C_09_09_2DLProblem_2main_8cpp-example.html>`_
* `Doxygen: C++/FortranProblem example <../../Doxygen/C_09_09_2FortranProblem_2main_8cpp-example.html>`_

In the following sections, we'll focus on the CasADi and classes-based problem
formulations.

CasADi
^^^^^^

An example of using CasADi to build the problem generation is given on the
:ref:`getting started` page. It makes use of the :ref:`high level problem formulation`:

.. testcode::
    :hide:

    # %% Build the problem (CasADi code, independent of alpaqa)
    import casadi as cs

    # Make symbolic decision variables
    x1, x2 = cs.SX.sym("x1"), cs.SX.sym("x2")
    x = cs.vertcat(x1, x2)  # Collect decision variables into one vector
    # Make a parameter symbol
    p = cs.SX.sym("p")

    # Objective function f and the constraints function g
    f = (1 - x1) ** 2 + p * (x2 - x1**2) ** 2
    g = cs.vertcat(
        (x1 - 0.5) ** 3 - x2 + 1,
        x1 + x2 - 1.5,
    )

    # Define the bounds
    C = [-0.25, -0.5], [1.5, 2.5]  # -0.25 <= x1 <= 1.5, -0.5 <= x2 <= 2.5
    D = [-cs.inf, -cs.inf], [0, 0]  #         g1 <= 0,           g2 <= 0

.. testcode::

    # %% Generate and compile C code for the objective and constraints using alpaqa
    from alpaqa import minimize

    problem = (
        minimize(f, x)  #       Objective function f(x)
        .subject_to_box(C)  #   Box constraints x ∊ C
        .subject_to(g, D)  #    General ALM constraints g(x) ∊ D
        .with_param(p, [1])  #  Parameter with default value (can be changed later)
    ).compile()

.. testoutput::
    :options: +ELLIPSIS
    :hide:

    ...

Compilation
"""""""""""

The :py:meth:`alpaqa.pyapi.minimize.MinimizationProblemDescription.compile`
method generates C code for the problem functions and their derivatives, and
compiles them into an optimized binary. Since |pylib_name| solvers spend most
of their time inside of problem function evaluations, this compilation can have
a significant impact in solver performance.

By default, the CasADi ``SX`` class is used for code generation. This causes the
subexpressions to be expanded, which is usually beneficial for performance.
However, the resulting expression trees can grow massive, and compiling the
generated C code can become very slow for large or complex problems. In such
cases, you can use the ``MX`` class, by passing ``sym=casadi.MX.sym`` as an
argument to the ``compile()`` function.

If you don't want to compile the problem at all (e.g. because no C compiler is
available, or because the resulting C files are too large), you can use the
The :py:meth:`alpaqa.pyapi.minimize.MinimizationProblemDescription.build`
method instead of :py:meth:`alpaqa.pyapi.minimize.MinimizationProblemDescription.compile`.
This will use CasADi's VM to evaluate the expressions.

Classes
^^^^^^^

The imperative class-based problem formulation allows users to select
alternative frameworks to implement the problem functions, such as NumPy, JAX,
TensorFlow, PyTorch, etc. However, this does mean that all required problem
functions and functions to evaluate their derivatives have to be supplied by the
user.

The following class can be used as a template:

.. testcode::
    :hide:

    import alpaqa
    import numpy as np

.. testcode::

    class MyProblem:
        def __init__(self):
            self.n = 3 # Number of variables
            self.m = 2 # Number of constraints
        def eval_proj_diff_g(self, z: np.ndarray, e: np.ndarray) -> None: ...
        def eval_proj_multipliers(self, y: np.ndarray, M: float) -> None: ...
        def eval_prox_grad_step(self, γ: float, x: np.ndarray, grad_ψ: np.ndarray, x_hat: np.ndarray, p: np.ndarray) -> float: ...
        def eval_inactive_indices_res_lna(self, γ: float, x: np.ndarray, grad_ψ: np.ndarray, J: np.ndarray) -> int: ...
        def eval_f(self, x: np.ndarray) -> float: ...
        def eval_grad_f(self, x: np.ndarray, grad_fx: np.ndarray) -> None: ...
        def eval_g(self, x: np.ndarray, gx: np.ndarray) -> None: ...
        def eval_grad_g_prod(self, x: np.ndarray, y: np.ndarray, grad_gxy: np.ndarray) -> None: ...
        def eval_grad_gi(self, x: np.ndarray, i: int, grad_gi: np.ndarray) -> None: ...
        def eval_hess_L_prod(self, x: np.ndarray, y: np.ndarray, scale: float, v: np.ndarray, Hv: np.ndarray) -> None: ...
        def eval_hess_ψ_prod(self, x: np.ndarray, y: np.ndarray, Σ: np.ndarray, scale: float, v: np.ndarray, Hv: np.ndarray) -> None: ...
        def eval_f_grad_f(self, x: np.ndarray, grad_fx: np.ndarray) -> float: ...
        def eval_f_g(self, x: np.ndarray, g: np.ndarray) -> float: ...
        def eval_grad_f_grad_g_prod(self, x: np.ndarray, y: np.ndarray, grad_f: np.ndarray, grad_gxy: np.ndarray) -> None: ...
        def eval_grad_L(self, x: np.ndarray, y: np.ndarray, grad_L: np.ndarray, work_n: np.ndarray) -> None: ...
        def eval_ψ(self, x: np.ndarray, y: np.ndarray, Σ: np.ndarray, ŷ: np.ndarray) -> float: ...
        def eval_grad_ψ(self, x: np.ndarray, y: np.ndarray, Σ: np.ndarray, grad_ψ: np.ndarray, work_n: np.ndarray, work_m: np.ndarray) -> None: ...
        def eval_ψ_grad_ψ(self, x: np.ndarray, y: np.ndarray, Σ: np.ndarray, grad_ψ: np.ndarray, work_n: np.ndarray, work_m: np.ndarray) -> float: ...
        def get_box_C(self) -> alpaqa.Box: ...
        def get_box_D(self) -> alpaqa.Box: ...
        def check(self): ...

.. testcode::
    :hide:

    alpaqa.Problem(MyProblem())

The meanings of different methods and their arguments are explained on the
`Problem formulations <../../Doxygen/page-problem-formulations.html>`_ page.
You can find a concrete example in :ref:`lasso jax example`.

.. note::
    To assign values to an output argument, you should use ``arg[:] = x``,
    and not ``arg = x``. For example:

    .. code-block:: python

        def eval_grad_f(self, x: np.ndarray, grad_f: np.ndarray) -> None:
            grad_f[:] = A @ x - b

Compilation and caching
-----------------------

Compiled CasADi problems are cached. To show the location of the cache, you can
use the following command:

.. code-block:: bash

    alpaqa cache path

If |pylib_name| is used inside of a virtual environment, it will create a cache
directory in that environment. You can force the global cache to be used instead
by setting the environment variable ``ALPAQA_GLOBAL_CACHE=1``.
To override the default cache directory, you can set the ``ALPAQA_CACHE_DIR``
environment variable.

To delete all cached problems, use:

.. code-block:: bash

    alpaqa cache clean

For the compilation of C code generated by CasADi, |pylib_name| relies on
`CMake <https://cmake.org/cmake/help/latest>`_. If you want to change the
compiler or the options used, you can clean |pylib_name|'s CMake build directory
and then set the appropriate environment variables, for example:

.. code-block:: bash

    alpaqa cache clean --cmake
    export CC="/usr/bin/gcc"                   # C compiler to use
    export CFLAGS="-march=native"              # Options to pass to the C compiler
    python "/path/to/your/alpaqa/script.py"

.. note::
    The CMake options set by these environment variables are cached: The values
    of the environment variables are picked up only during the very first
    compilation of a CasADi problem after clearing the cache. Later changes to
    the environment variables are ignored and do not affect the cached values.

Compiler installation
^^^^^^^^^^^^^^^^^^^^^

See the resources below if you do not have a C compiler installed on your system:

* **Linux**: GCC (``sudo apt install gcc``, ``sudo dnf install gcc``)
* **macOS**: Xcode (https://developer.apple.com/xcode/)
* **Windows**: Visual Studio (https://visualstudio.microsoft.com/)

Solver selection and parameter tuning
-------------------------------------

You can find more information about the different solvers and their parameters
in `this presentation <https://github.com/kul-optec/alpaqa/blob/develop/docs/slides/alpaqa-for-nmpc.pdf>`_.
