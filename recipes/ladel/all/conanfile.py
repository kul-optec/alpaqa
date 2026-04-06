import os

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get


class LADELRecipe(ConanFile):
    name = "ladel"
    package_type = "library"

    # Optional metadata
    license = "LGPL-3.0-or-later"
    author = "Pieter P <pieter.p.dev@outlook.com>"
    url = "https://github.com/kul-optec/LADEL"
    description = "Quasidefinite LDL factorization package with (symmetric) row/column adds and deletes"
    topics = ("LDL", "LDLT", "linear-algebra")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_mex": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_mex": False,
    }

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def export_sources(self):
        export_conandata_patches(self)

    def source(self):
        get(
            self,
            **self.conan_data["sources"][self.version],
            destination=self.source_folder,
            strip_root=True
        )
        apply_conandata_patches(self)

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        self.test_requires("gtest/1.17.0")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.23 <4.2]")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        if self.options.get_safe("with_mex"):
            tc.variables["LADEL_WITH_MEX"] = True
        if can_run(self):
            tc.variables["LADEL_FORCE_TEST_DISCOVERY"] = True
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.test()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "none")
        self.cpp_info.set_property("cmake_file_name", "LADEL")
        self.cpp_info.builddirs.append(os.path.join("lib", "cmake", "LADEL"))
