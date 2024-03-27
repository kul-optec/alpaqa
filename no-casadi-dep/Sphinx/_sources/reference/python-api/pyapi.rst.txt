.. _high level problem formulation:

High-level problem formulation
==============================

This is the main API for defining optimization problems using CasADi
expressions.

For examples, see the :ref:`lasso <lasso example>` and
:ref:`nonlinear regression <nonlinear regression example>` examples.

.. autofunction:: alpaqa.minimize
    :noindex:

.. autoclass:: alpaqa.MinimizationProblemDescription
    :noindex:
    :no-members:
    :no-special-members:

    .. automethod:: alpaqa.MinimizationProblemDescription.subject_to_box
        :noindex:

    .. automethod:: alpaqa.MinimizationProblemDescription.subject_to
        :noindex:

    .. automethod:: alpaqa.MinimizationProblemDescription.subject_to_penalty
        :noindex:

    .. automethod:: alpaqa.MinimizationProblemDescription.with_l1_regularizer
        :noindex:

    .. automethod:: alpaqa.MinimizationProblemDescription.with_param
        :noindex:

    .. automethod:: alpaqa.MinimizationProblemDescription.with_param_value
        :noindex:

    .. automethod:: alpaqa.MinimizationProblemDescription.compile
        :noindex:

    .. automethod:: alpaqa.MinimizationProblemDescription.build
        :noindex:
