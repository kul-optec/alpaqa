import os

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, save
from conan.tools.scm import Version


class guanaqoRecipe(ConanFile):
    name = "guanaqo"
    package_type = "library"

    # Optional metadata
    license = "LGPL-3.0-or-later"
    author = "Pieter P <pieter.p.dev@outlook.com>"
    url = "https://github.com/tttapa/guanaqo"
    description = "Utilities for scientific software."
    topics = "scientific software"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    # https://github.com/conan-io/conan/issues/19108
    package_id_non_embed_mode = "full_mode"
    bool_guanaqo_options = {
        "with_quad_precision": False,  # affects ABI
        "with_itt": False,  # affects ABI
        "with_tracing": False,  # affects ABI
        "with_perfetto": False,  # affects ABI
        "with_pcm": False,  # affects ABI
        "with_pcm_tracing": False,  # affects ABI
        "with_hl_blas_tracing": True,  # affects ABI
        "with_openmp": False,  # affects ABI
        "with_blas": False,  # affects ABI
        "with_mkl": False,  # affects ABI
    }
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "blas_index_type": ["int", "long", "long long"],  # affects ABI
    } | {k: [True, False] for k in bool_guanaqo_options}
    default_options = {
        "shared": False,
        "fPIC": True,
        "blas_index_type": "long long",
    } | bool_guanaqo_options

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")
        if Version(self.version) < "1.0.0-alpha.8":
            self.options.rm_safe("with_itt")
            self.options.rm_safe("with_tracing")
        if Version(self.version) < "1.0.0-alpha.9":
            self.options.rm_safe("with_blas")
            self.options.rm_safe("with_mkl")
            self.options.rm_safe("blas_index_type")
        if Version(self.version) < "1.0.0-alpha.10":
            self.options.rm_safe("with_openmp")
        if Version(self.version) < "1.0.0-alpha.13":
            self.options.rm_safe("with_hl_blas_tracing")
        if Version(self.version) < "1.0.0-alpha.24":
            self.options.rm_safe("with_perfetto")
            self.options.rm_safe("with_pcm")
            self.options.rm_safe("with_pcm_tracing")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        with_blas = self.options.get_safe("with_blas")
        if with_blas:
            if not self.options.with_mkl:
                # OpenBLAS does not allow configuring the index type
                self.options.rm_safe("blas_index_type")
        else:
            self.options.rm_safe("with_mkl")
            self.options.rm_safe("blas_index_type")
        if not self.options.get_safe("with_tracing") or not with_blas:
            self.options.rm_safe("with_hl_blas_tracing")
        if not self.options.get_safe("with_perfetto") or not self.options.get_safe("with_pcm"):
            self.options.rm_safe("with_pcm_tracing")

    def export_sources(self):
        commit = self.conan_data["commits"][self.version]
        save(self, os.path.join(self.export_sources_folder, "commit.txt"), commit)
        export_conandata_patches(self)

    def source(self):
        get(
            self,
            **self.conan_data["sources"][self.version],
            destination=self.source_folder,
            strip_root=True,
        )
        apply_conandata_patches(self)

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        if self.options.get_safe("with_itt"):
            self.requires("ittapi/3.25.5", transitive_headers=True)
        if self.options.get_safe("with_perfetto"):
            self.requires("perfetto/52.0", transitive_headers=True)
        if self.options.get_safe("with_pcm"):
            self.requires("intel-pcm/tttapa.20260207")
        if self.options.get_safe("with_blas") and not self.options.get_safe("with_mkl"):
            self.requires("openblas/0.3.30", transitive_headers=True)
        if self.options.get_safe("with_openmp") and self.settings.compiler == "clang":
            self.requires(
                f"llvm-openmp/[~{self.settings.compiler.version}]",
                transitive_headers=True,
            )

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.24 <5]")
        self.test_requires("gtest/1.17.0")
        self.test_requires("eigen/[~3.4 || ~5.0]")
        if self.conf.get("user.guanaqo:with_python_tests", default=False, check_type=bool):
            self.test_requires("nanobind/2.10.2")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        for k in self.bool_guanaqo_options:
            value = self.options.get_safe(k)
            if value is not None and value.value is not None:
                tc.variables["GUANAQO_" + k.upper()] = bool(value)
        if self.options.get_safe("with_blas"):
            tc.variables["GUANAQO_WITH_OPENBLAS"] = not self.options.with_mkl
            tc.variables["GUANAQO_BLAS_INDEX_TYPE"] = self.options.get_safe(
                "blas_index_type", default="int"
            )
        if can_run(self):
            tc.variables["GUANAQO_FORCE_TEST_DISCOVERY"] = True
        if self.conf.get("user.guanaqo:with_python_tests", default=False, check_type=bool):
            tc.variables["GUANAQO_WITH_PYTHON_TESTS"] = True
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
        self.cpp_info.builddirs.append(os.path.join("lib", "cmake", "guanaqo"))
