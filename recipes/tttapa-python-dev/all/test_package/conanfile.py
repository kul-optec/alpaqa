import re

from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.scm import Version


class PythonTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires(self.tested_reference_str)

    def build_requirements(self):
        self.build_requires("cmake/[>=4.1.2 <5]")

    def layout(self):
        cmake_layout(self)

    def test(self):
        tokens = re.split("[@#]", self.tested_reference_str)
        python_dev_name, required_version = tokens[0].split("/", 1)
        python_dev = self.dependencies.direct_host[python_dev_name]
        cmake = CMake(self)
        version = Version(required_version)
        with_gil = python_dev.options.get_safe("disable_gil", False)
        vars = {
            "PYTHON_DEV_PACKAGE_FOLDER": python_dev.package_folder,
            "PYTHON_DEV_VERSION": f"{version.major}.{version.minor}.{version.patch}",
            "PYTHON_DEV_WITH_BIN": "On" if python_dev.options.with_bin else "Off",
            "PYTHON_DEV_DISABLE_GIL": "On" if with_gil else "Off",
        }
        if python_dev.options.with_bin:
            vars["PYTHON_DEV_LIB_EXT"] = "so" if python_dev.options.shared else "a"
        cmake.configure(variables=vars)
