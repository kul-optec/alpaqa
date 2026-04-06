import os

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches
from conan.tools.scm.git import Git


class HpipmRecipe(ConanFile):
    name = "hpipm"
    package_type = "library"

    # Optional metadata
    author = "Pieter P <pieter.p.dev@outlook.com>"
    url = "https://github.com/giaf/hpipm"
    description = "High-performance interior-point-method QP and QCQP solvers."
    topics = "scientific software"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
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
        git = Git(self)
        git.clone(url="https://github.com/giaf/hpipm.git", target=".")
        git.checkout(self.conan_data["sources"][self.version]["commit"])
        apply_conandata_patches(self)

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        self.requires("blasfeo/tttapa.20260119", transitive_headers=True)

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.24 <5]")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["HPIPM_FIND_BLASFEO"] = True
        if self.settings.arch not in ["x86", "x86_64"]:
            tc.cache_variables["TARGET"] = "GENERIC"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "none")
        self.cpp_info.builddirs.append(os.path.join("lib", "cmake", "hpipm"))
