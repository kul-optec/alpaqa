import os

from conan import ConanFile
from conan.tools.files import copy
from conan.tools.files import apply_conandata_patches, export_conandata_patches
from conan.tools.scm.git import Git


class SimdRecipe(ConanFile):
    name = "gsi-hpc-simd"
    package_type = "header-library"

    author = "Pieter P <pieter.p.dev@outlook.com>"
    url = "https://github.com/GSI-HPC/simd"
    description = "Implementation of C++26 §29.10 Data-parallel types for GCC"
    license = "GPL-3.0-or-later WITH GCC-exception-3.1, LGPL-3.0-or-later"

    def export_sources(self):
        export_conandata_patches(self)

    def source(self):
        git = Git(self, folder="simd")
        git.clone(url="https://github.com/GSI-HPC/simd.git", target=".")
        git.checkout(self.conan_data["sources"][self.version]["commit"])
        apply_conandata_patches(self)

    def package(self):
        include = os.path.join(self.package_folder, "include")
        copy(self, "simd", os.path.join(self.source_folder, "simd"), include)
        copy(self, "bits/*.h", os.path.join(self.source_folder, "simd"), include)

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.set_property("cmake_target_name", "gsi-hpc-simd::simd")