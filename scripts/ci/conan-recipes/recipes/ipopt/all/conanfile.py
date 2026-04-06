from conan import ConanFile
from conan.tools.gnu import (
    Autotools,
    AutotoolsToolchain,
    PkgConfigDeps,
    PkgConfig,
    AutotoolsDeps,
)
from conan.tools.files import get


class IpoptRecipe(ConanFile):
    name = "ipopt"
    package_type = "library"

    # Optional metadata
    license = "EPL-2.0"
    author = "Pieter P <pieter.p.dev@outlook.com>"
    url = "https://github.com/coin-or/Ipopt"
    description = "COIN-OR Interior Point Optimizer IPOPT"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_mumps": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_mumps": True,
    }

    def source(self):
        get(
            self,
            **self.conan_data["sources"][self.version],
            destination=self.source_folder,
            keep_permissions=True,
            strip_root=True,
        )

    def requirements(self):
        self.requires("openblas/0.3.30")
        if self.options.with_mumps:
            self.requires("coinmumps/3.0.7")

    def generate(self):
        tc = AutotoolsDeps(self)
        tc.generate()
        tc = AutotoolsToolchain(self)
        tc.generate()
        pc = PkgConfigDeps(self)
        pc.generate()

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def build(self):
        autotools = Autotools(self)
        # Note: space argument prevents 'configure' from trying to determine
        #       the lapack flags itself (it shouldn't do that, they're already
        #       specified by Conan)
        args = [
            "--with-lapack= ",
            "--with-mumps=" + ("yes" if self.options.with_mumps else "no"),
        ]
        autotools.configure(args=args)
        autotools.make()

    def package(self):
        autotools = Autotools(self)
        autotools.install()

    def package_info(self):
        if False:
            pc = PkgConfig(self, "ipopt", pkg_config_path="lib/pkgconfig")
            pc.fill_cpp_info(
                self.cpp_info,
                is_system=False,
                system_libs={"m", "rt", "pthread", "gfortran", "dl", "quadmath"},
            )
            """
            Package coinmumps was not found in the pkg-config search path.
            Perhaps you should add the directory containing `coinmumps.pc'
            to the PKG_CONFIG_PATH environment variable
            Package 'coinmumps', required by 'ipopt', not found
            ERROR: ipopt/3.14.16: Error in package_info() method, line 80
                    pc.fill_cpp_info(
                    ConanException: Error 1 while executing
            """
        else:
            self.cpp_info.system_libs = ["dl"]
        # The pkg-config version of these variables contain nonsense,
        # overwrite them
        self.cpp_info.includedirs = ["include/coin-or"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.libs = ["ipopt"]
        self.cpp_info.set_property("pkg_config_name", "ipopt")
        self.cpp_info.set_property("cmake_file_name", "Ipopt")
        self.cpp_info.set_property("cmake_target_name", "Ipopt::Ipopt")
