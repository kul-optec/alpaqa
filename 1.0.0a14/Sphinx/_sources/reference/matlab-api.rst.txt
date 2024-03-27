MATLAB API Reference
====================

.. warning::
    The alpaqa MATLAB API is experimental. Options are subject to change, and
    there may be some problems or bugs that need to be ironed out.

    Please share your experience on https://github.com/kul-optec/alpaqa/issues

.. mat:module:: alpaqa


.. mat:function:: alpaqa.minimize(problem, x0, y0, method="panoc", params=struct)

    Solves the given minimization problem using one of alpaqa's solvers.

    :param problem: A description of the minimization problem to solve.
    :type problem: alpaqa.Problem
    :param x0: Optional initial guess.
    :type x0: double(1,:)
    :param y0: Optional initial guess for the Lagrange multipliers.
    :type y0: double(1,:)
    :param method: Name of the solver to use. (Keyword-only)

        Possible values are: ``panoc``, ``panoc.lbfgs``, ``panoc.struclbfgs``,
        ``panoc.anderson``, ``zerofpr``, ``zerofpr.lbfgs``, ``zerofpr.struclbfgs``,
        ``zerofpr.anderson``, ``pantr``
    :type method: string
    :param params: Options to pass to the solver. (Keyword-only)

        The struct can contain four top-level fields, specifying options for the
        different components of the solver:

        * ``alm``: Parameters for the outer ALM solver :cpp:class:`alpaqa::ALMParams`
        * ``solver``: Parameters for the inner solver :cpp:class:`alpaqa::PANOCParams`, :cpp:class:`alpaqa::ZeroFPRParams`, :cpp:class:`alpaqa::PANTRParams`
        * ``dir``: Parameters for the inner solver's direction provider :cpp:class:`alpaqa::LBFGSDirectionParams`, :cpp:class:`alpaqa::StructuredLBFGSDirectionParams`, :cpp:class:`alpaqa::NewtonTRDirectionParams`
        * ``accel``: Parameters for the direction's accelerator :cpp:class:`alpaqa::LBFGSParams`, :cpp:class:`alpaqa::SteihaugCGParams`
    :type params: struct
    :return: The solution, corresponding Lagrange multipliers, and a struct containing solver statistics.
    :rtype: [``double(1,:)``, ``double(1,:)``, ``struct``]


.. mat:class:: alpaqa.Problem

    Describes a minimization problem.
    For more details, please see `Problem formulations (Doxygen) <../../Doxygen/page-problem-formulations.html>`_.

    .. mat:attribute:: f

        Objective function.

        :type: ``casadi.SX`` | ``casadi.MX``
    
    .. mat:attribute:: x

        Optimization variables.

        :type: ``casadi.SX`` | ``casadi.MX``

    .. mat:attribute:: g

        General constraints.

        :type: ``casadi.SX`` | ``casadi.MX``
        
    .. mat:attribute:: param

        Optional problem parameter variable.

        :type: ``casadi.SX`` | ``casadi.MX``

    .. mat:attribute:: C_lowerbound

        Lower bound on the optimization variables.

        :type: ``double(1,:)``

    .. mat:attribute:: C_upperbound

        Upper bound on the optimization variables.

        :type: ``double(1,:)``

    .. mat:attribute:: D_lowerbound

        Lower bound on the general constraints.

        :type: ``double(1,:)``

    .. mat:attribute:: D_upperbound

        Upper bound on the general constraints.

        :type: ``double(1,:)``

    .. mat:attribute:: l1_regularization

        Weight factor(s) of the optional 1-norm regularization of the
        optimization variables.

        :type: ``double(1,1)`` | ``double(1,:)``

    .. mat:attribute:: param_value

        Numerical value of the problem parameters.

        :type: ``double(1,:)``

Example
-------

The following is a transcription of the Python example from the
:ref:`getting started` page.

.. literalinclude:: ../../../../examples/Matlab/getting_started.m
   :language: matlab

Limitations and known issues
----------------------------

* Only problems formulated using **CasADi expressions** are currently supported.
* Pre-compilation of CasADi functions is not implemented, which means that the
  MATLAB version of alpaqa may be **significantly slower** than its Python and
  C++ counterparts.
* User-defined callbacks, per-iteration statistics, and function counters are
  not available.
* Parallelization and cancellation are not possible.
* Options with a non-ASCII name must be specified using their ASCII aliases.
* Duration options must be encoded as a string (e.g. ``'1min 30s 456µs'``).
* Statistics that are NaN or infinite are converted to a string.
* Error checking and reporting need to be improved.
* Console output contains ANSI escape sequences.
