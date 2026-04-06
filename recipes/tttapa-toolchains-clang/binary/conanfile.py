from __future__ import annotations

"""
libstdc++ cross-compilation toolchains with specific versions of glibc for
maximal compatibility.

You can select specific target triplets using the custom `os.toolchain-vendor`
and `arch.toolchain-cpu` settings. Below is an example `settings_user.yml` Conan
configuration file:

```yaml
os:
  Linux:
    toolchain-vendor: [null, focal, bionic, centos7, neon, rpi, rpi3]
arch:
  armv7hf:
    toolchain-cpu: [null, armv8, armv7]
```

To select a specific version of libstdc++, add the following lines to
`settings_user.yml` as well:

```yaml
compiler:
  clang:
    libcxx:
      libstdc++11:
        gcc_version: [null, ANY]
```

You can then configure the version using e.g. `compiler.libcxx.gcc_version=15.1`
in your profile.
"""

import glob
import os
import shlex

from conan import ConanFile
from conan.tools.files import get
from conan.errors import ConanInvalidConfiguration
from conan.tools.scm import Version


class ToolchainsConan(ConanFile):
    name = "tttapa-toolchains-clang"
    package_type = "application"
    description = "Clang cross-compilers using GCC toolchains."
    homepage = "https://github.com/tttapa/toolchains"
    settings = "os", "arch"

    def _get_target_triplet(self) -> tuple[str, str, str]:
        default_cpu = {
            "x86_64": "x86_64",
            "armv8": "aarch64",
            "armv7hf": "armv7",
            "armv6": "armv6",
        }
        default_vendor = {
            "x86_64": "bionic",
            "aarch64": "rpi3",
            "armv8": "rpi3",
            "armv7": "neon",
            "armv6": "rpi",
        }
        cpu = self.settings_target.get_safe("arch.toolchain-cpu", default=None)
        cpu = cpu or default_cpu.get(str(self.settings_target.arch))
        if cpu is None:
            msg = "Unsupported toolchain CPU type. "
            msg += "Please make sure that settings.arch is one of the "
            msg += "supported values (" + ", ".join(default_cpu) + "), or "
            msg += "set settings.arch.toolchain-cpu explicitly."
            raise ConanInvalidConfiguration(msg)
        vendor = self.settings_target.get_safe("os.toolchain-vendor", default=None)
        vendor = vendor or default_vendor.get(cpu)
        if vendor is None:
            msg = "Unsupported toolchain vendor type. "
            msg += "Please make sure that settings.arch.toolchain-cpu is valid, "
            msg += "or set settings.os.toolchain-vendor explicitly."
            raise ConanInvalidConfiguration(msg)
        triplet = (cpu, vendor)
        os = {
            ("armv8", "rpi3"): "linux-gnueabihf",
            ("armv7", "neon"): "linux-gnueabihf",
            ("armv6", "rpi"): "linux-gnueabihf",
        }.get(triplet, "linux-gnu")
        triplet += (os,)
        return triplet

    def _get_target_libstdcxx_version(self) -> str | None:
        return self.settings_target.get_safe(
            "compiler.libcxx.gcc_version", default=None
        )

    def _get_gcc_version(self, tgt_gcc_version) -> str:
        possible_versions = self.conan_data["sources"][self.version]
        if tgt_gcc_version is None:
            return max(possible_versions, key=Version)
        if tgt_gcc_version in possible_versions:
            return tgt_gcc_version
        for p in possible_versions:
            v = Version(p)
            tgt_v = Version(tgt_gcc_version)
            if (tgt_v.major or "") == v.major and (tgt_v.minor or v.minor) == v.minor:
                return p

        msg = f"Invalid GCC version '{tgt_gcc_version}'. "
        msg += f"Only the following versions are supported for the compiler: {', '.join(possible_versions)}"
        raise ConanInvalidConfiguration(msg)

    def _get_gcc_dir(self, toolchain_dir, triple, version, bitness):
        lib_dirs = ["lib64", "lib"] if bitness == 64 else ["lib32", "lib"]
        gcc_dirs = ["gcc", "gcc-cross"]
        matches = []
        for lib in lib_dirs:
            for gcc in gcc_dirs:
                pattern = os.path.join(toolchain_dir, lib, gcc, triple, f"{version}*")
                found = glob.glob(pattern)
                matches.extend(found)
        return matches

    def _get_tgt_lib_dir(self, toolchain_dir, triple, bitness):
        lib_dirs = ["lib64", "lib"] if bitness == 64 else ["lib32", "lib"]
        return [os.path.join(toolchain_dir, triple, lib) for lib in lib_dirs]

    def validate(self):
        if self.settings.arch != "x86_64" or self.settings.os != "Linux":
            msg = f"This toolchain is not compatible with {self.settings.os}-{self.settings.arch}. "
            msg += "It can only run on Linux-x86_64."
            raise ConanInvalidConfiguration(msg)

        if self.settings_target.compiler != "clang":
            msg = f"The compiler is set to '{self.settings_target.compiler}', but this "
            msg += "toolchain only supports building with Clang."
            raise ConanInvalidConfiguration(msg)

        if self.settings_target.compiler.libcxx != "libstdc++11":
            msg = f"The C++ standard library is set to '{self.settings_target.compiler.libcxx}', but this "
            msg += "toolchain only supports libstdc++11."
            raise ConanInvalidConfiguration(msg)

        gcc_version = self._get_gcc_version(self._get_target_libstdcxx_version())
        target_trip = self._get_target_triplet()
        target_trip_str = "-".join(target_trip)
        sources = self.conan_data["sources"][self.version]
        try:
            sources[gcc_version]["Linux-x86_64"][target_trip_str]
        except KeyError as e:
            msg = f"Unsupported toolchain target triplet '{target_trip_str}' "
            msg += f"for GCC version {gcc_version}."
            raise ConanInvalidConfiguration(msg) from e

    def package(self):
        host = f"{self.settings.os}-{self.settings.arch}"
        gcc_version = self._get_gcc_version(self._get_target_libstdcxx_version())
        target_trip_str = "-".join(self._get_target_triplet())
        sources = self.conan_data["sources"][self.version]
        get(
            self,
            **sources[gcc_version][host][target_trip_str],
            destination=self.package_folder,
            strip_root=True,
        )
        self.run(f"chmod -R +w {shlex.quote(self.package_folder)}")

    def package_id(self):
        self.info.settings_target = self.settings_target.copy()
        self.info.settings_target.rm_safe("build_type")
        self.info.settings_target.rm_safe("compiler.cppstd")
        self.info.settings_target.rm_safe("compiler.cstd")

    def package_info(self):
        target = "-".join(self._get_target_triplet())
        toolchain_dir = os.path.join(self.package_folder, target)
        processor = {
            "x86_64-focal-linux-gnu": "x86_64",
            "x86_64-bionic-linux-gnu": "x86_64",
            "x86_64-centos7-linux-gnu": "x86_64",
            "aarch64-rpi3-linux-gnu": "aarch64",
            "armv8-rpi3-linux-gnueabihf": "armv7l",  # armv8l does not exist
            "armv7-neon-linux-gnueabihf": "armv7l",
            "armv6-rpi-linux-gnueabihf": "armv6l",
        }[target]
        bitness = {
            "x86_64-focal-linux-gnu": 64,
            "x86_64-bionic-linux-gnu": 64,
            "x86_64-centos7-linux-gnu": 64,
            "aarch64-rpi3-linux-gnu": 64,
            "armv8-rpi3-linux-gnueabihf": 32,
            "armv7-neon-linux-gnueabihf": 32,
            "armv6-rpi-linux-gnueabihf": 32,
        }[target]
        self.conf_info.define("tools.cmake.cmaketoolchain:system_name", "Linux")
        self.conf_info.define("tools.cmake.cmaketoolchain:system_processor", processor)
        self.conf_info.define("tools.gnu:host_triplet", target)
        link_flags = []
        toolchain_flags = []
        if Version(self.settings_target.compiler).major >= 16:
            gcc_version = self._get_gcc_version(self._get_target_libstdcxx_version())
            toolchain_install_dir = self._get_gcc_dir(
                toolchain_dir, target, gcc_version, bitness
            )[0]
            toolchain_flags += ["--gcc-install-dir=" + toolchain_install_dir]
            link_flags += ["-L" + toolchain_install_dir]
        else:
            toolchain_flags += [
                "--gcc-toolchain=" + toolchain_dir,
                "--gcc-triple=" + target,
            ]
        sysroot = os.path.join(toolchain_dir, target, "sysroot")
        self.conf_info.define("tools.build:sysroot", sysroot)
        self.conf_info.append("tools.build:cflags", toolchain_flags)
        self.conf_info.append("tools.build:cxxflags", toolchain_flags)
        self.conf_info.append("tools.build:exelinkflags", link_flags)
        self.conf_info.append("tools.build:sharedlinkflags", link_flags)
        self.conf_info.define("tools.build.cross_building:cross_build", True)
