.. _cmake_api_ref:

CMake API Reference
===================

To include alpaqa in your CMake project, use the ``find_package`` command:

.. code-block:: cmake

    find_package(alpaqa 1.1.0 [EXACT] [QUIET] [REQUIRED]
                 [COMPONENTS <components> ...]
                 [OPTIONAL_COMPONENTS <components> ...])


Components and targets
----------------------

alpaqa comes with multiple optional components, some of which can be packaged
and installed independently. The available components are:

+--------------+--------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------+
|  Component   |  Description                                                                                                 |  Targets                                                                                                       |
+==============+==============================================================================================================+================================================================================================================+
|  ``Core``    |  The main alpaqa solvers and other core functionality. If no components are specified, this is the default.  |  ``alpaqa``                                                                                                    |
+--------------+--------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------+
|  ``Dl``      |  The dynamic loading C API headers for creating problems that can be loaded by the alpaqa solvers.           |  ``dl-api``                                                                                                    |
+--------------+--------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------+
|  ``DlLoader``|  Dynamically loading problems implemented using the Dl API.                                                  |  ``dl-loader``                                                                                                 |
+--------------+--------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------+
|  ``CasADi``  |  Classes for loading and building problem definitions using CasADi.                                          |  ``casadi-loader``, ``casadi-ocp-loader``                                                                      |
+--------------+--------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------+
|  ``CUTEst``  |  Interface for loading problems formulated using SIF/CUTEst.                                                 |  ``cutest-interface``                                                                                          |
+--------------+--------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------+
|  ``Extra``   |  Additional solver adapters and problem loaders that fall outside of the core library.                       |  ``ipopt-adapter``, ``lbfgsb-fortran``, ``lbfgsb-adapter``, ``qpalm-adapter``, ``problem-loader``, ``drivers`` |
+--------------+--------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------+
|  ``Tools``   |  Command-line tools for invoking the solvers.                                                                |  ``driver``, ``gradient-checker``                                                                              |
+--------------+--------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------+

Targets are prefixed with ``alpaqa::``. For example, to link with the main alpaqa library,
use:

.. code-block:: cmake

    target_link_libraries(<target> PUBLIC alpaqa::alpaqa)

To check whether a certain target is available, you can use:

.. code-block:: cmake

    if (TARGET alpaqa::qpalm-adapter)
        # ...
    endif()

Commands
--------

.. cmake-module::
    ../../../../src/cmake/dl-problem.cmake
