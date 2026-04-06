import os

from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.build import can_run


class guanaqoTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def build_requirements(self):
        self.tool_requires(self.tested_reference_str)

    def requirements(self):
        if self.settings.compiler == "clang":
            self.requires(f"llvm-openmp/[~{self.settings.compiler.version}]")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build(cli_args=["--verbose"])

    def layout(self):
        cmake_layout(self)

    def test(self):
        if can_run(self):
            cmd = os.path.join(self.cpp.build.bindir, "example")
            self.run(cmd, env="conanrun")
            cmd = os.path.join(self.cpp.build.bindir, "example_c")
            self.run(cmd, env="conanrun")
