import os

from conan import ConanFile
from conan.tools.files import get
from conan.errors import ConanInvalidConfiguration


class TorchConan(ConanFile):
    name = "libtorch"
    package_type = "shared-library"
    settings = "os", "compiler", "arch"

    options = {"cuda_version": [None, "12.4"]}
    default_options = {"cuda_version": None}

    def validate(self):
        supported_os = ("Linux", "Windows")
        if self.settings.arch != "x86_64" or self.settings.os not in supported_os:
            msg = f"This package is not compatible with {self.settings.os}-{self.settings.arch}. "
            msg += "It can only run on Linux-x86_64 and Windows-x86_64."
            raise ConanInvalidConfiguration(msg)
        cxx11 = self.settings.get_safe("compiler.libcxx")
        if self.settings.os == "Linux" and cxx11 and cxx11 != "libstdc++11":
            msg = f"This package currently only supports libstdc++11, not {cxx11}"
            raise ConanInvalidConfiguration(msg)

    def package(self):
        cuda_version = self.options.get_safe("cuda_version")
        cuda_version = f"cuda-{cuda_version}" if cuda_version else "cpu"
        win_lnx = str(self.settings.os)
        pkg = self.conan_data["sources"][self.version][win_lnx][cuda_version]
        get(self, **pkg, destination=self.package_folder, strip_root=True)

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "none")
        self.cpp_info.builddirs.append(os.path.join("share", "cmake", "ATen"))
        self.cpp_info.builddirs.append(os.path.join("share", "cmake", "Caffe2"))
        self.cpp_info.builddirs.append(os.path.join("share", "cmake", "Tensorpipe"))
        self.cpp_info.builddirs.append(os.path.join("share", "cmake", "Torch"))
        self.cpp_info.builddirs.append(os.path.join("share", "cmake", "fbgemm"))
        self.cpp_info.builddirs.append(os.path.join("share", "cmake", "kineto"))
