import os

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import (
    apply_conandata_patches,
    copy,
    export_conandata_patches,
    get,
    replace_in_file,
)
from conan.tools.scm import Version


class LLVMOpenMpConan(ConanFile):
    name = "llvm-openmp"
    description = (
        "The OpenMP (Open Multi-Processing) specification is a standard for a set of compiler directives, "
        "library routines, and environment variables that can be used to specify shared memory parallelism "
        "in Fortran and C/C++ programs. This is the LLVM implementation."
    )
    license = "Apache-2.0 WITH LLVM-exception"
    homepage = "https://github.com/llvm/llvm-project/blob/main/openmp"
    topics = ("llvm", "openmp", "parallelism")

    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "build_libomptarget": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "build_libomptarget": False,
    }
    options_description = {
        "build_libomptarget": (
            "Build the LLVM OpenMP Offloading Runtime Library (libomptarget) "
            "in addition to the OpenMP Runtime Library (libomp)."
        )
    }

    def export_sources(self):
        export_conandata_patches(self)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def requirements(self):
        if self.options.build_libomptarget:
            self.requires(f"llvm-core/{self.version}")

    def layout(self):
        if self.version < "22.0.0":
            cmake_layout(self, src_folder="src")
        else:
            cmake_layout(self, src_folder="src/openmp")

    def validate(self):
        if self.settings.compiler not in ["apple-clang", "clang", "gcc", "intel-cc"]:
            msg = f"{self.settings.compiler} is not supported by this recipe. Contributions are welcome!"
            raise ConanInvalidConfiguration(msg)
        if Version(self.version).major >= 17:
            check_min_cppstd(self, 17)

    def build_requirements(self):
        if Version(self.version).major >= 17:
            self.tool_requires("cmake/[>=3.20 <4]")

    def source(self):
        data = self.conan_data["sources"][self.version]
        if self.version < "22.0.0":
            get(self, **data["openmp"], strip_root=True)
            get(
                self,
                **data["cmake"],
                strip_root=True,
                destination=self.export_sources_folder,
            )
            copy(
                self,
                "*.cmake",
                src=os.path.join(self.export_sources_folder, "Modules"),
                dst=os.path.join(self.source_folder, "cmake"),
            )
        else:
            get(
                self,
                **data["llvm"],
                strip_root=True,
                destination=os.path.dirname(self.source_folder),
            )
        apply_conandata_patches(self)
        replace_in_file(
            self,
            os.path.join(self.source_folder, "runtime", "CMakeLists.txt"),
            "add_subdirectory(test)",
            "",
        )

    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()
        tc = CMakeToolchain(self)
        tc.variables["OPENMP_STANDALONE_BUILD"] = True
        tc.variables["LIBOMP_ENABLE_SHARED"] = self.options.shared
        tc.variables["OPENMP_ENABLE_LIBOMPTARGET"] = self.options.build_libomptarget
        # Do not build OpenMP Tools Interface (OMPT)
        tc.variables["LIBOMP_OMPT_SUPPORT"] = False
        tc.variables["LIBOMP_OMPD_SUPPORT"] = False
        tc.variables["OPENMP_ENABLE_OMPT_TOOLS"] = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(
            self,
            "LICENSE.txt",
            src=self.source_folder,
            dst=os.path.join(self.package_folder, "licenses"),
        )
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "OpenMP")
        self.cpp_info.set_property("cmake_target_name", "OpenMP::OpenMP")
        aliases = ["OpenMP::OpenMP_C", "OpenMP::OpenMP_CXX"]
        self.cpp_info.set_property("cmake_target_aliases", aliases)
        self.cpp_info.libs = ["omp"]
        self.cpp_info.includedirs.append("include")
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs = ["dl", "m", "pthread", "rt"]
        if self.settings.compiler == "clang":
            self.cpp_info.cxxflags = ["-fopenmp"]
        elif self.settings.compiler == "apple-clang":
            self.cpp_info.cxxflags = ["-Xpreprocessor", "-fopenmp"]
        elif self.settings.compiler == "gcc":
            self.cpp_info.cxxflags = ["-fopenmp"]
        elif self.settings.compiler == "intel-cc":
            self.cpp_info.cxxflags = ["/Qopenmp"] if self.settings.os == "Windows" else ["-Qopenmp"]
        self.cpp_info.cflags = self.cpp_info.cxxflags
