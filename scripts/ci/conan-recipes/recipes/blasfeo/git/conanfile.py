import os

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches
from conan.tools.scm import Git

TARGET_LIST = [
    "X64_INTEL_SKYLAKE_X",
    "X64_INTEL_HASWELL",
    "X64_INTEL_SANDY_BRIDGE",
    "X64_INTEL_CORE",
    "X64_AMD_BULLDOZER",
    "X86_AMD_JAGUAR",
    "X86_AMD_BARCELONA",
    "ARMV8A_APPLE_M1",
    "ARMV8A_ARM_CORTEX_A76",
    "ARMV8A_ARM_CORTEX_A73",
    "ARMV8A_ARM_CORTEX_A57",
    "ARMV8A_ARM_CORTEX_A55",
    "ARMV8A_ARM_CORTEX_A53",
    "ARMV7A_ARM_CORTEX_A15",
    "ARMV7A_ARM_CORTEX_A9",
    "ARMV7A_ARM_CORTEX_A7",
    "GENERIC",
]


class BlasfeoRecipe(ConanFile):
    name = "blasfeo"
    package_type = "library"

    # Optional metadata
    author = "Pieter P <pieter.p.dev@outlook.com>"
    url = "https://github.com/giaf/blasfeo"
    description = "Basic linear algebra subroutines for embedded optimization."
    topics = "scientific software"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "target": TARGET_LIST,
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "target": "GENERIC",
        "shared": False,
        "fPIC": True,
    }

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.24 <5]")

    def export_sources(self):
        export_conandata_patches(self)

    def source(self):
        git = Git(self)
        git.clone(url=self.conan_data["sources"][self.version]["url"], target=".")
        git.checkout(self.conan_data["sources"][self.version]["commit"])
        apply_conandata_patches(self)

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        pass

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["TARGET"] = self.options.target
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
        self.cpp_info.builddirs.append(os.path.join("share", "cmake", "blasfeo"))
