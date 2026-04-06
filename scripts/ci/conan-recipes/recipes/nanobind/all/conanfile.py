from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy
import os


class NanobindConan(ConanFile):
    name = "nanobind"
    description = "tiny and efficient C++/Python bindings"
    license = "BSD-3-Clause"
    homepage = "https://github.com/wjakob/nanobind"
    topics = ("nanobind", "python", "binding")
    settings = "os", "arch", "compiler", "build_type"
    package_type = "header-library"

    def layout(self):
        basic_layout(self)

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def requirements(self):
        self.requires("tsl-robin-map/1.4.0", transitive_headers=True)
        self.test_requires("eigen/[~3.4 || ~5.0]")
        self.tool_requires("cmake/[>=3.24 <5]")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["NB_INSTALL_DATADIR"] = "lib/nanobind"
        tc.variables["NB_USE_SUBMODULE_DEPS"] = False
        build_testing = not self.conf.get("tools.build:skip_test", default=False)
        tc.variables["NB_TEST"] = build_testing
        tc.generate()

    generators = "CMakeDeps",

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        # cmake.test()

    def package(self):
        licenses = os.path.join(self.package_folder, "licenses")
        copy(self, "LICENSE", src=self.source_folder, dst=licenses)
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "none")
        self.cpp_info.builddirs.append(os.path.join("lib", "nanobind", "cmake"))

    def package_id(self):
        self.info.clear()
