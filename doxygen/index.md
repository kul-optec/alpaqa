# alpaqa

`alpaqa` is an efficient implementation of an Augmented Lagrangian method for general nonlinear programming problems.
It makes use of the first-order, matrix-free PANOC algorithm as an inner solver.
The numerical algorithms themselves are implemented in C++ for optimal 
performance, and they are also exposed as an easy-to-use Python package. An
experimental MATLAB interface is available as well.

The solvers in this library solve minimization problems of the following form:
@f[
\begin{aligned}
    & \underset{x}{\text{minimize}}
    & & f(x) &&&& f : \Rn \rightarrow \R \\
    & \text{subject to}
    & & \underline{x} \le x \le \overline{x} \\
    &&& \underline{z} \le g(x) \le \overline{z} &&&& g : \Rn \rightarrow \R^m
\end{aligned}
@f]

## Documentation

[**Documentation** (Sphinx)](../Sphinx/index.html)  
[**C++ Documentation** (Doxygen)](./index.html)  
[**Matlab documentation**](../Sphinx/reference/matlab-api.html)  

For the Python interface, it is recommended to consult the [Sphinx documentation](../Sphinx/index.html), whereas the Doxygen documentation provides
the best overview of the C++ API (see especially the [**Topics**](./topics.html)
page).

## Examples

[**Python examples**](../Sphinx/examples/index.html)  
[**C++ examples**](./examples.html)  

To get started using the C++ API, see the @ref C++/CustomCppProblem/main.cpp
example, which explains how to formulate an optimization problem, how to
configure the solver, and how to solve the problem.

Other examples are provided to demonstrate how to load external problems or
problems that have been defined in different programming languages:

  - @ref C++/FortranProblem/main.cpp (problem defined in Fortran, solved in C++)
  - @ref C++/DLProblem/main.cpp (problem defined in C, loaded from an external
    DLL, solved in C++)
  - @ref problems/sparse-logistic-regression.cpp (problem defined in C++,
    loaded from an external DLL, solved in using the `alpaqa-driver`
    command-line tool)
  - @ref C++/CasADi/Rosenbrock/main.cpp (problem defined using [CasADi](https://web.casadi.org/),
    loaded from an external DLL, solved in C++)

## Installation

The project is available on [PyPI](https://pypi.org/project/alpaqa):

```sh
python3 -m pip install --upgrade --pre alpaqa
```

For more information, please see the full
[installation instructions](../Sphinx/install/installation.html). To build
`alpaqa` or any of its interfaces from source, see [these instructions](@ref installation).

## Citations

If you use this software in your research, please cite the following publication: @cite pas2022alpaqa

> Pieter Pas, Mathijs Schuurmans, and Panagiotis Patrinos. [Alpaqa: A matrix-free solver for nonlinear MPC and large-scale nonconvex optimization](https://ieeexplore.ieee.org/document/9838172/). In _2022 European Control Conference (ECC)_, pages 417–422, 2022.
