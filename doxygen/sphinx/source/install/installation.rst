.. _installation:

Installation
============

Pip
---

The preferred way to install the alpaqa Python interface is using pip:

.. code-block:: sh

    python3 -m pip install --upgrade --pre alpaqa

(`PyPI <https://pypi.org/project/alpaqa>`_)

To compile problems using the Python interface, you will need a C compiler, such
as GCC or Clang on Linux, Xcode on macOS, or MSVC on Windows (see the
:ref:`Tips and tricks` page for details and alternatives).

From source
-----------

Building alpaqa from source requires the installation of some C++ dependencies, 
see `Installation (Doxygen) <../../Doxygen/installation.html>`_ for detailed
instructions.

C++ Library
-----------

Pre-built binaries for Linux are available from the
`Releases page on GitHub <https://github.com/kul-optec/alpaqa/releases>`_.

For Debian-based systems, the .deb packages can be installed using

.. code-block:: sh

    sudo apt update
    sudo apt install ./libalpaqa*_1.0.0a15.dev0_amd64.deb

Different components are available:

* ``libalpaqa`` contains the shared libraries needed to run applications that
  use alpaqa.
* ``libalpaqa-debug`` contains the debugging symbols for those libraries.
* ``libalpaqa-dl_dev`` contains the C header files needed to compile problem
  specifications that can be dynamically loaded by alpaqa.
* ``libalpaqa-dev`` contains all development files such as headers and CMake
  configuration files needed to compile software that invokes alpaqa solvers.
* ``libalpaqa-tools`` contains command line utilities such as alpaqa-driver,
  which can be used to invoke the solvers directly, without the need to write
  any C++ code.

The following distributions are tested:

* Debian: 11 (Bullseye), 12 (Bookworm), Sid
* Ubuntu: 20.04 (Focal), 22.04 (Jammy), rolling

Alternatively, the .tar.gz file can be extracted and installed manually.

.. code-block:: sh

    sudo tar xzf alpaqa-1.0.0a15.dev0-Linux-x86_64.tar.gz -C /usr/local --strip-components=1

This requires glibc 2.17 or later. You may need to install or pre-load the
following additional runtime dependencies:

* ``libgfortran5`` (GFortran 10 or later)
* ``libquadmath0``

MATLAB interface
----------------

The experimental MATLAB/MEX interface can be installed by running the following
command in the MATLAB command window:

.. code-block:: matlab

    unzip(['https://github.com/kul-optec/alpaqa/releases/download/1.0.0a15.dev0/alpaqa-matlab-' computer('arch') '.zip'], userpath)

You need CasADi to be installed as well: https://web.casadi.org/get

To uninstall the alpaqa MATLAB interface, simply remove the ``+alpaqa`` folder:

.. code-block:: matlab

    rmdir(fullfile(userpath, '+alpaqa'), 's');
