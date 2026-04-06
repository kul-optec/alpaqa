import os

from conan import ConanFile
from conan.tools.files import get
from conan.errors import ConanInvalidConfiguration
from conan.tools.files import mkdir, rename, copy


class NVCCConan(ConanFile):
    name = "nvcc"
    package_type = "application"
    description = "NVIDIA nvcc (cross-)compilers and CUDA toolchain."
    homepage = "https://developer.download.nvidia.com/compute/cuda/redist/"
    settings = "os", "arch"

    options = {
        k: [True, False]
        for k in (
            "with_cublas",
            "with_cudla",
            "with_cufft",
            "with_cufile",
            "with_curand",
            "with_cusolver",
            "with_cusparse",
            "with_npp",
            "with_nvidia_nscq",
            "with_nvjitlink",
            "with_nvjpeg",
        )
    }
    default_options = {
        "with_cublas": True,
        "with_cudla": True,
        "with_cufft": True,
        "with_cufile": False,
        "with_curand": True,
        "with_cusolver": True,
        "with_cusparse": True,
        "with_npp": False,
        "with_nvidia_nscq": False,
        "with_nvjitlink": True,
        "with_nvjpeg": False,
    }

    _arch_map = {"x86_64": "x86_64", "armv8": "aarch64"}

    def validate(self):
        if self.settings.arch != "x86_64" or self.settings.os != "Linux":
            msg = f"This toolchain is not compatible with {self.settings.os}-{self.settings.arch}. "
            msg += "It can only run on Linux-x86_64."
            raise ConanInvalidConfiguration(msg)

        invalid_arch = str(self.settings_target.arch) not in self._arch_map
        if self.settings_target.os != "Linux" or invalid_arch:
            msg = f"This toolchain only supports building for Linux-{','.join(self._arch_map)}. "
            msg += f"{self.settings_target.os}-{self.settings_target.arch} is not supported."
            raise ConanInvalidConfiguration(msg)

    def package(self):
        host_arch = str(self.settings.arch)
        tgt_arch = str(self.settings_target.arch)
        host_name = self._arch_map[host_arch] + "-linux"
        host_dir = os.path.join(self.package_folder, "targets", host_name)
        tgt_name = self._arch_map[tgt_arch] + "-linux"
        tgt_dir = os.path.join(self.package_folder, "targets", tgt_name)
        licenses_dir = os.path.join(self.package_folder, "licenses")
        host_inc_dir_rel = os.path.join("targets", host_name, "include")
        host_lib_dir_rel = os.path.join("targets", host_name, "lib")

        def install_license(src_dir, pkg_name):
            rename(
                self,
                os.path.join(src_dir, "LICENSE"),
                os.path.join(licenses_dir, "LICENSE-" + pkg_name),
            )

        # Download the host packages (e.g. nvcc)
        host_packages = self.conan_data["sources"][self.version][host_arch]["host"]
        mkdir(self, licenses_dir)
        mkdir(self, host_dir)
        for pkg_name, pkg in host_packages.items():
            get(self, **pkg, destination=self.package_folder, strip_root=True)
            install_license(self.package_folder, pkg_name)

        # Mimic the layout of the official CUDA toolkit so CMake find module is
        # able to locate CUDA correctly
        lib_dir = os.path.join(self.package_folder, "lib64")
        inc_dir = os.path.join(self.package_folder, "include")
        if tgt_name != host_name:
            copy(self, "*.h", inc_dir, os.path.join(tgt_dir, "include"))
            copy(self, "*.hpp", inc_dir, os.path.join(tgt_dir, "include"))
        rename(self, inc_dir, os.path.join(host_dir, "include"))
        os.symlink(host_inc_dir_rel, inc_dir, target_is_directory=True)
        mkdir(self, os.path.join(host_dir, "lib"))
        os.symlink(host_lib_dir_rel, lib_dir, target_is_directory=True)

        # Download the target packages (e.g. libcudart)
        tgt_packages = self.conan_data["sources"][self.version][host_arch][tgt_arch]
        mkdir(self, tgt_dir)
        for pkg_name, pkg in tgt_packages.items():
            opt_name = f"with_{pkg_name[3:]}"
            if pkg_name.startswith("lib") and not getattr(self.options, opt_name, True):
                continue
            get(self, **pkg, destination=tgt_dir, strip_root=True)
            install_license(tgt_dir, pkg_name)

    def package_id(self):
        self.info.settings_target = self.settings_target
        self.info.settings_target.rm_safe("build_type")
        self.info.settings_target.rm_safe("compiler")

    def package_info(self):
        pkg_dir = self.package_folder
        bin_dir = os.path.join(pkg_dir, "bin")
        tgt_arch = str(self.settings_target.arch)
        tgt_name = self._arch_map[tgt_arch] + "-linux"
        tgt_dir = os.path.join(pkg_dir, "targets", tgt_name)
        tgt_libdir = os.path.join(tgt_dir, "lib")
        tgt_linkflags = "-Wl,-rpath-link," + tgt_libdir
        compilers = {"cuda": os.path.join(bin_dir, "nvcc")}
        cmake_vars = {
            "CUDA_TOOLKIT_ROOT_DIR": pkg_dir,
            # Hint for CMake's FindCUDA.cmake
            "CUDA_TOOLKIT_TARGET_DIR": tgt_dir,
            # Undocumented hint for CMake's FindCUDA.cmake, necessary when cross-compiling
            "CUDA_TOOLKIT_TARGET_NAME": tgt_name,
            # Undocumented hint for pytorch's FindCUDA.cmake, necessary when cross-compiling
            "CUDA_TOOLKIT_TARGET_NAMEs": tgt_name,
            # CMake picks the wrong compiler if unset
            "CMAKE_CUDA_HOST_COMPILER": "${CMAKE_CXX_COMPILER}",
        }
        self.buildenv_info.prepend_path("PATH", bin_dir)
        self.buildenv_info.prepend_path("CUDA_TOOLKIT_ROOT", pkg_dir)
        self.conf_info.update("tools.build:compiler_executables", compilers)
        self.conf_info.update("tools.cmake.cmaketoolchain:extra_variables", cmake_vars)
        self.conf_info.define("tools.cmake.cmaketoolchain:toolset_cuda", pkg_dir)
        self.conf_info.append("tools.build:exelinkflags", tgt_linkflags)
        self.conf_info.append("tools.build:sharedlinkflags", tgt_linkflags)
