import os

from conan import ConanFile
from conan.tools.files import copy
from conan.tools.scm.git import Git


class SkaSortRecipe(ConanFile):
    name = "ska-sort"
    package_type = "header-library"

    author = "Pieter P <pieter.p.dev@outlook.com>"
    url = "https://github.com/skarupke/ska_sort"
    description = "Radix sort"
    license = "BSL-1.0"

    def source(self):
        git = Git(self)
        git.clone(url="https://github.com/skarupke/ska_sort.git", target=".")
        git.checkout(self.conan_data["sources"][self.version]["commit"])

    def package(self):
        include = os.path.join(self.package_folder, "include")
        copy(self, "ska_sort.hpp", self.source_folder, include)
