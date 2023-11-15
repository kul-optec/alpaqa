import os

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.build import can_run


class AlpaqaRecipe(ConanFile):
    name = "alpaqa"
    version = "1.0.0-alpha.15"

    # Optional metadata
    license = "LGPLv3"
    author = "Pieter P <pieter.p.dev@outlook.com>"
    url = "https://github.com/kul-optec/alpaqa"
    description = (
        "Augmented Lagrangian and PANOC solvers for nonconvex numerical optimization"
    )
    topics = ("optimization", "panoc", "alm", "mpc")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    bool_alpaqa_options = {
        "with_quad_precision": False,
        "with_single_precision": False,
        "with_long_double": False,
        "with_openmp": False,
        "with_drivers": True,
        "with_casadi": False,
        "with_cutest": False,
        "with_qpalm": False,
        "with_lbfgsb": None,
        "with_ocp": False,
        "with_json": False,
        "with_matlab": False,
    }
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    } | {k: [True, False, None] for k in bool_alpaqa_options}
    default_options = {
        "shared": False,
        "fPIC": True,
    } | bool_alpaqa_options

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = (
        "CMakeLists.txt",
        "src/*",
        "cmake/*",
        "interfaces/*",
        "python/*",
        "test/*",
        "LICENSE",
        "README.md",
    )

    generators = ("CMakeDeps",)

    def requirements(self):
        self.requires("eigen/3.4.0")
        self.test_requires("gtest/1.11.0")
        if self.options.with_casadi:
            self.requires("casadi/3.6.4@alpaqa")
        if self.options.with_json:
            self.requires("nlohmann_json/3.11.2")
        if self.options.with_matlab:
            self.requires("utfcpp/4.0.1")

    def config_options(self):
        if self.settings.get_safe("os") == "Windows":
            self.options.rm_safe("fPIC")

    def validate(self):
        if self.options.with_matlab and not self.options.with_json:
            msg = "MATLAB MEX interface requires JSON. Set 'with_json=True'."
            raise ConanInvalidConfiguration(msg)
        if self.options.with_matlab and not self.options.with_casadi:
            msg = "MATLAB MEX interface requires CasADi. Set 'with_casadi=True'."
            raise ConanInvalidConfiguration(msg)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["ALPAQA_WITH_EXAMPLES"] = False
        for k in self.bool_alpaqa_options:
            value = getattr(self.options, k, None)
            if value is not None and value.value is not None:
                tc.variables["ALPAQA_" + k.upper()] = bool(value)
        if can_run(self):
            tc.variables["ALPAQA_FORCE_TEST_DISCOVERY"] = True
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
        self.cpp_info.builddirs.append(os.path.join("lib", "cmake", "alpaqa"))
