.. _nuclear norm example:

Nuclear norm
============

In this example, we use the PANOC solver to solve a problem with a nuclear norm
as regularizer, in order to promote low-rank solutions.

.. math::
    \begin{aligned}
        & \minimize_X && \tfrac12 \normsq{AX - B}_F + \lambda \norm{X}_*
    \end{aligned}

Here, :math:`X \in \R^{m\times n}`, with a rank :math:`r` that's much smaller
than its dimensions.

The `JAX <https://github.com/google/jax#installation>`_ package is used to
compute gradients (although we could easily do without, since the loss is
quadratic).

.. literalinclude:: ../../../../../examples/Python/advanced/nuclear-norm.py
    :language: python
    :linenos:
