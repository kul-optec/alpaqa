from __future__ import annotations

import casadi as cs
import contextlib
from pathlib import Path
import os
from os.path import join, basename
import shelve
import uuid
import pickle
import base64
import glob
import subprocess
import platform
from textwrap import dedent
import warnings
from .. import alpaqa as pa
from ..casadi_generator import (
    _prepare_casadi_problem,
    SECOND_ORDER_SPEC,
    write_casadi_problem_data,
)
from ..cache import get_alpaqa_cache_dir

# TODO: factor out caching logic


def _load_casadi_problem(sofile: Path):
    print("-- Loading:", sofile)
    prob = pa.load_casadi_problem(str(sofile))
    return prob


def _python_sysconfig_platform_to_cmake_platform_win(
    plat_name: str | None,
) -> str | None:
    """Convert a sysconfig platform string to the corresponding value of
    https://cmake.org/cmake/help/latest/variable/CMAKE_GENERATOR_PLATFORM.html"""
    return {
        None: None,
        "win32": "Win32",
        "win-amd64": "x64",
        "win-arm32": "ARM",
        "win-arm64": "ARM64",
    }.get(plat_name)


def _get_windows_architecture() -> str:
    import sysconfig

    plat = sysconfig.get_platform()
    arch = _python_sysconfig_platform_to_cmake_platform_win(plat)
    if arch is None:
        raise RuntimeError(f"Unknown Windows platform architecture {plat}")
    return arch


def _get_cmake_bin() -> str:
    """
    Get the path to the CMake executable:

    1. The ``ALPAQA_CMAKE_PROGRAM`` environment variable, if set;
    2. The path obtained from ``cmake.CMAKE_BIN_DIR``, if available;
    3. Simply ``"cmake"``.
    """
    cmake_bin = os.getenv("ALPAQA_CMAKE_PROGRAM")
    if not cmake_bin:
        with contextlib.suppress(ImportError, AttributeError):
            import cmake

            cmake_bin = join(cmake.CMAKE_BIN_DIR, "cmake")
    return cmake_bin or "cmake"


def _compile_casadi_problem(cachedir, uid, f, g, second_order, name, **kwargs):
    # Prepare directories
    projdir = join(cachedir, "build")
    builddir = join(projdir, "build")
    os.makedirs(builddir, exist_ok=True)
    probdir = join(cachedir, str(uid))

    # Prepare the necessary CasADi functions
    functions = _prepare_casadi_problem(f, g, second_order, **kwargs)

    # Make code generators for all functions
    def make_codegen(funcname, func):
        codegen = cs.CodeGenerator(f"{name}_{funcname}.c")
        codegen.add(func)
        return codegen

    codegens = {
        funcname: make_codegen(funcname, func) for funcname, func in functions.items()
    }
    # Generate the code
    cfiles = [codegen.generate(join(projdir, "")) for codegen in codegens.values()]

    # CMake configure script
    cmakelists = f"""\
        cmake_minimum_required(VERSION 3.17)
        project(CasADi-{name} LANGUAGES C)
        set(CMAKE_SHARED_LIBRARY_PREFIX "")
        add_library({name} SHARED {" ".join(map(basename, cfiles))})
        install(FILES $<TARGET_FILE:{name}>
                DESTINATION lib)
        install(FILES {" ".join(map(basename, cfiles))}
                DESTINATION src)
        """
    with open(join(projdir, "CMakeLists.txt"), "w") as f:
        f.write(dedent(cmakelists))

    # Run CMake
    build_type = os.getenv("ALPAQA_BUILD_CONFIG", "Release")
    parallel = os.getenv("ALPAQA_BUILD_PARALLEL", "")
    cmake = _get_cmake_bin()
    # Configure
    configure_cmd = [cmake, "-B", builddir, "-S", projdir]
    if platform.system() == "Windows":
        configure_cmd += ["-A", _get_windows_architecture()]
    else:
        configure_cmd += ["-G", "Ninja Multi-Config"]
    # Build
    build_cmd = [
        cmake,
        "--build",
        builddir,
        "--config",
        build_type,
        "-j",
    ]
    if parallel:
        build_cmd += [parallel]
    # Install
    install_cmd = [
        cmake,
        "--install",
        builddir,
        "--config",
        build_type,
        "--prefix",
        probdir,
    ]
    subprocess.run(configure_cmd, check=True)
    subprocess.run(build_cmd, check=True)
    subprocess.run(install_cmd, check=True)
    # Find the resulting binary
    sofile = glob.glob(join(probdir, "lib", name + ".*"))
    if len(sofile) == 0:
        raise RuntimeError(f"Unable to find compiled CasADi problem '{name}'")
    elif len(sofile) > 1:
        warnings.warn(f"Multiple compiled CasADi problem files were found for '{name}'")
    soname = os.path.relpath(sofile[0], cachedir)
    return soname


def generate_and_compile_casadi_problem_no_load(
    f: cs.Function,
    g: cs.Function,
    *,
    C=None,
    D=None,
    param=None,
    l1_reg=None,
    penalty_alm_split=None,
    second_order: SECOND_ORDER_SPEC = "no",
    name: str = "alpaqa_problem",
    **kwargs,
) -> Path:
    """Compile the objective and constraint functions into a alpaqa Problem.

    :param f:            Objective function f(x).
    :param g:            Constraint function g(x).
    :param C:            Bound constraints on x.
    :param D:            Bound constraints on g(x).
    :param param:        Problem parameter values.
    :param l1_reg:       L1-regularization on x.
    :param penalty_alm_split: This many components at the beginning of g(x) are
                              handled using a quadratic penalty method rather
                              than an augmented Lagrangian method.
    :param second_order: Whether to generate functions for evaluating Hessians.
    :param name: Optional string description of the problem (used for filename).
    :param kwargs:       Parameters passed to
                         :py:func:`..casadi_generator.generate_casadi_problem`.

    :return: Path to the shared object file with CasADi functions that can be
             loaded by the solvers.

    .. note::
        If you copy the shared object file, don't forget to also copy the
        accompanying CSV file with problem data.
    """

    cachedir = get_alpaqa_cache_dir()
    cachefile = join(cachedir, "problems")

    encode = lambda x: base64.b64encode(x).decode("ascii")
    key = encode(pickle.dumps((f, g, second_order, name, kwargs)))

    os.makedirs(cachedir, exist_ok=True)
    with shelve.open(cachefile) as cache:
        if key in cache:
            try:
                uid, soname = cache[key]
                sofile = join(cachedir, soname)
                write_casadi_problem_data(
                    sofile,
                    C,
                    D,
                    param,
                    l1_reg,
                    penalty_alm_split,
                    name,
                )
                return Path(sofile)
            except:
                del cache[key]
                # probdir = join(cachedir, str(uid))
                # if os.path.exists(probdir) and os.path.isdir(probdir):
                #     shutil.rmtree(probdir)
                raise
        uid = uuid.uuid1()
        soname = _compile_casadi_problem(
            cachedir,
            uid,
            f,
            g,
            second_order,
            name,
            **kwargs,
        )
        cache[key] = uid, soname
        sofile = join(cachedir, soname)
        write_casadi_problem_data(
            sofile,
            C,
            D,
            param,
            l1_reg,
            penalty_alm_split,
            name,
        )
        return Path(sofile)


def generate_and_compile_casadi_problem(
    *args,
    **kwargs,
) -> pa.CasADiProblem:
    """
    Calls :py:func:`generate_and_compile_casadi_problem_no_load` and loads the
    resulting problem file.

    :return: Problem specification that can be passed to the solvers.
    """
    return _load_casadi_problem(
        generate_and_compile_casadi_problem_no_load(*args, **kwargs)
    )


if pa.with_casadi_ocp:
    from .ocp import generate_and_compile_casadi_control_problem
