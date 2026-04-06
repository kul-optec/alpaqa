import contextlib
import os

from conan import ConanFile
from conan.tools.gnu import (
    Autotools,
    AutotoolsToolchain,
    PkgConfig,
    PkgConfigDeps,
    AutotoolsDeps,
)
from conan.tools.files import get, patch, rename


class COINMUMPSRecipe(ConanFile):
    name = "coinmumps"
    package_type = "library"

    # Optional metadata
    license = "EPL-1.0"
    author = "Pieter P <pieter.p.dev@outlook.com>"
    url = "https://github.com/coin-or-tools/ThirdParty-Mumps"
    description = "COIN-OR autotools harness to build Mumps"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "static_fortran_libs": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "static_fortran_libs": False,
    }

    def source(self):
        get(
            self,
            **self.conan_data["sources"][self.version],
            destination=self.source_folder,
            keep_permissions=True,
            strip_root=True
        )
        get(
            self,
            **self.conan_data["mumps-sources"][self.version],
            destination=os.path.join(self.source_folder, "MUMPS"),
            strip_root=True
        )
        patch(self, patch_file="mumps_mpi.patch")
        rename(self, "MUMPS/libseq/mpi.h", "MUMPS/libseq/mumps_mpi.h")

    def requirements(self):
        self.requires("openblas/0.3.30")

    def generate(self):
        pc = PkgConfigDeps(self)
        pc.generate()
        tc = AutotoolsDeps(self)
        tc.generate()
        tc = AutotoolsToolchain(self)
        tc.generate()

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def build(self):
        autotools = Autotools(self)
        # Note: space argument prevents 'configure' from trying to determine
        #       the lapack flags itself (it shouldn't do that, they're already
        #       specified by Conan)
        args = ["--with-lapack= "]
        autotools.configure(args=args)
        autotools.make()

    def package(self):
        autotools = Autotools(self)
        autotools.install()

    def package_info(self):
        pc = PkgConfig(self, "coinmumps", pkg_config_path="lib/pkgconfig")
        pc.fill_cpp_info(
            self.cpp_info,
            is_system=False,
            system_libs={"m", "rt", "pthread", "gfortran", "dl", "quadmath"},
        )
        # The pkg-config version of these variables contain nonsense,
        # overwrite them
        if self.options.get_safe("static_fortran_libs"):
            sys_libs = self.cpp_info.system_libs
            with contextlib.suppress(ValueError):
                sys_libs[sys_libs.index("gfortran")] = ":libgfortran.a"
            with contextlib.suppress(ValueError):
                sys_libs[sys_libs.index("quadmath")] = ":libquadmath.a"
        self.cpp_info.includedirs = ["include/coin-or/mumps"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.libs = ["coinmumps"]
        self.cpp_info.set_property("pkg_config_name", "coinmumps")
