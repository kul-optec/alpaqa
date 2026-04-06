import os

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, save


class HyhoundRecipe(ConanFile):
    name = "hyhound"
    package_type = "library"

    # Optional metadata
    license = "LGPL-3.0-or-later"
    author = "Pieter P <pieter.p.dev@outlook.com>"
    url = "https://github.com/kul-optec/hyhound"
    description = "Hyperbolic Householder transformations for Cholesky factorization up- and downdates."
    topics = "scientific software"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    # https://github.com/conan-io/conan/issues/19108
    package_id_non_embed_mode = "full_mode"
    bool_hyhound_options = {
        "with_ocp": False,  # affects ABI
        "with_benchmarks": False,
    }
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "real_type": ["double;float", "float;double", "double", "float"],  # affects ABI
    } | {k: [True, False] for k in bool_hyhound_options}
    default_options = {
        "shared": False,
        "fPIC": True,
        "real_type": "double;float",
    } | bool_hyhound_options

    def config_options(self):
        if self.settings.get_safe("os") == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.options["guanaqo/*"].with_blas = True

    def export_sources(self):
        commit = self.conan_data["commits"][self.version]
        save(self, os.path.join(self.export_sources_folder, "commit.txt"), commit)
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
        cmake_layout(self)

    def requirements(self):
        self.requires(
            "guanaqo/[>=1.0.0-alpha.22 <1.0.0-alpha.28, include_prerelease]",
            transitive_headers=True,
            transitive_libs=True,
        )
        if self.options.with_ocp:
            self.requires("eigen/[~3.4 || ~5.0]", transitive_headers=True)
        elif self.options.with_benchmarks:
            self.requires("eigen/[~3.4 || ~5.0]")
        else:
            self.test_requires("eigen/[~3.4 || ~5.0]")
        if self.options.with_benchmarks:
            self.requires("benchmark/1.9.4")

    def build_requirements(self):
        self.test_requires("gtest/1.17.0")
        self.tool_requires("cmake/[>=3.24 <5]")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        for k in self.bool_hyhound_options:
            value = self.options.get_safe(k)
            if value is not None and value.value is not None:
                tc.variables["HYHOUND_" + k.upper()] = bool(value)
        guanaqo = self.dependencies["guanaqo"]
        index_type = guanaqo.options.get_safe("blas_index_type", default="int")
        real_type = str(self.options.real_type)
        tc.variables["HYHOUND_DENSE_INDEX_TYPE"] = index_type
        tc.variables["HYHOUND_DENSE_REAL_TYPE"] = real_type
        if can_run(self):
            tc.variables["HYHOUND_FORCE_TEST_DISCOVERY"] = True
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
        self.cpp_info.builddirs.append(os.path.join("lib", "cmake", "hyhound"))
