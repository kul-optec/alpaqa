from __future__ import annotations

"""
GCC cross-compilation toolchains with specific versions of glibc for maximal
compatibility.

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

For Clang or Intel icx with a specific libstdc++ version, add the following to
`settings_user.yml`:

```yaml
compiler:
  clang:
    libcxx:
      libstdc++11:
        gcc_version: [null, ANY]
      libstdc++:
        gcc_version: [null, ANY]
  intel-cc:
    libcxx:
      libstdc++11:
        gcc_version: [null, ANY]
      libstdc++:
        gcc_version: [null, ANY]
```

You can then configure the version using e.g. `compiler.libcxx.gcc_version=15`
in your profile.
"""

import glob
import os
import shlex
from contextlib import suppress

from conan import ConanFile
from conan.tools.files import get
from conan.errors import ConanInvalidConfiguration, ConanException
from conan.tools.scm import Version


class ToolchainsConan(ConanFile):
    name = "tttapa-toolchains"
    package_type = "application"
    description = "GCC cross-compilers."
    homepage = "https://github.com/tttapa/toolchains"
    settings = "os", "arch"

    # Target triplet metadata
    _TARGET_METADATA = {
        "x86_64-focal-linux-gnu": {"processor": "x86_64", "bitness": 64},
        "x86_64-bionic-linux-gnu": {"processor": "x86_64", "bitness": 64},
        "x86_64-centos7-linux-gnu": {"processor": "x86_64", "bitness": 64},
        "aarch64-rpi3-linux-gnu": {"processor": "aarch64", "bitness": 64},
        "armv8-rpi3-linux-gnueabihf": {
            "processor": "armv7l",
            "bitness": 32,
        },  # armv8l does not exist
        "armv7-neon-linux-gnueabihf": {"processor": "armv7l", "bitness": 32},
        "armv6-rpi-linux-gnueabihf": {"processor": "armv6l", "bitness": 32},
    }

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

    def _resolve_gcc_version(self, tgt_gcc_version: str | None) -> str:
        """Resolve GCC version from exact or partial version string."""
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

    def _get_gcc_version(self) -> str:
        """Get GCC version based on target compiler."""
        if self.settings_target.compiler == "gcc":
            tgt_version = str(self.settings_target.compiler.version)
        elif self.settings_target.compiler in ("clang", "intel-cc"):
            tgt_version = self.settings_target.get_safe("compiler.libcxx.gcc_version", default=None)
        else:
            msg = f"Unsupported compiler '{self.settings_target.compiler}'. "
            msg += "This toolchain only supports GCC, Clang, and Intel icx."
            raise ConanInvalidConfiguration(msg)
        return self._resolve_gcc_version(tgt_version)

    def _get_gcc_dir(self, toolchain_dir, triple, version, bitness):
        """Find GCC installation directory (for Clang)."""
        lib_dirs = ["lib64", "lib"] if bitness == 64 else ["lib32", "lib"]
        gcc_dirs = ["gcc", "gcc-cross"]
        matches = []
        for lib in lib_dirs:
            for gcc in gcc_dirs:
                pattern = os.path.join(toolchain_dir, lib, gcc, triple, f"{version}*")
                found = glob.glob(pattern)
                matches.extend(found)
        return matches

    def _validate_triplet_availability(self, gcc_version: str):
        """Check if the target triplet is available for the given GCC version."""
        target_trip_str = "-".join(self._get_target_triplet())
        sources = self.conan_data["sources"][self.version]
        try:
            sources[gcc_version]["Linux-x86_64"][target_trip_str]
        except KeyError as e:
            msg = f"Unsupported toolchain target triplet '{target_trip_str}' "
            msg += f"for GCC version {gcc_version}."
            raise ConanInvalidConfiguration(msg) from e

    def _validate_gcc(self):
        """Validate settings when using GCC compiler."""
        gcc_version = self._get_gcc_version()
        self._validate_triplet_availability(gcc_version)

    def _validate_clang(self):
        """Validate settings when using Clang compiler."""
        libcxx = self.settings_target.get_safe("compiler.libcxx")
        if libcxx is None or str(libcxx) not in ("libstdc++11", "libstdc++"):
            msg = f"The C++ standard library is set to '{libcxx}', but this "
            msg += "toolchain only supports libstdc++11 or libstdc++."
            raise ConanInvalidConfiguration(msg)

        gcc_version = self._get_gcc_version()
        self._validate_triplet_availability(gcc_version)

    def _validate_icx(self):
        mode = self.settings_target.get_safe("compiler.mode")
        if mode != "icx":
            msg = f"The Intel compiler mode is set to '{mode}', but this "
            msg += "toolchain only supports 'icx' mode."
            raise ConanInvalidConfiguration(msg)
        self._validate_clang()

    def validate(self):
        if self.settings.arch != "x86_64" or self.settings.os != "Linux":
            msg = f"This toolchain is not compatible with {self.settings.os}-{self.settings.arch}. "
            msg += "It can only run on Linux-x86_64."
            raise ConanInvalidConfiguration(msg)

        if self.settings_target.compiler == "gcc":
            self._validate_gcc()
        elif self.settings_target.compiler == "clang":
            self._validate_clang()
        elif self.settings_target.compiler == "intel-cc":
            self._validate_icx()
        else:
            msg = f"The compiler is set to '{self.settings_target.compiler}', but this "
            msg += "toolchain only supports GCC, Clang, and Intel icx."
            raise ConanInvalidConfiguration(msg)

    def package(self):
        host = f"{self.settings.os}-{self.settings.arch}"
        gcc_version = self._get_gcc_version()
        target_trip_str = "-".join(self._get_target_triplet())
        sources = self.conan_data["sources"][self.version]
        get(
            self,
            **sources[gcc_version][host][target_trip_str],
            destination=self.package_folder,
            strip_root=True,
        )
        self.run(f"chmod -R +w {shlex.quote(self.package_folder)}")

    def _del_settings_except(self, obj, *keep_fields):
        """Remove all fields from obj except those in keep_fields."""
        fields = None
        with suppress(ConanException):
            fields = obj.fields  # even hasattr(obj, "fields", None) raises ConanException
        if fields is not None:
            for f in tuple(fields):
                if f not in keep_fields:
                    delattr(obj, f)

    def package_id(self):
        # Only include settings that actually affect the package
        # Use a whitelist approach to ignore any custom sub-settings
        self.info.settings_target = self.settings_target.copy()
        assert hasattr(self.info.settings_target, "fields")
        self._del_settings_except(self.info.settings_target, "os", "arch", "compiler")
        self._del_settings_except(self.info.settings_target.arch, "toolchain-cpu")
        self._del_settings_except(self.info.settings_target.os, "toolchain-vendor")
        if self.info.settings_target.compiler == "gcc":
            self._del_settings_except(self.info.settings_target.compiler, "version")
        elif self.info.settings_target.compiler in ("clang", "intel-cc"):
            self._del_settings_except(self.info.settings_target.compiler, "libcxx")
            if self.info.settings_target.compiler.get_safe("libcxx") is not None:
                self._del_settings_except(self.info.settings_target.compiler.libcxx, "gcc_version")

    def _get_target_processor(self, target: str) -> str:
        """Get CMake system processor for target."""
        return self._TARGET_METADATA[target]["processor"]

    def _get_target_bitness(self, target: str) -> int:
        """Get target bitness (32 or 64)."""
        return self._TARGET_METADATA[target]["bitness"]

    def _configure_common_package_info(self, target: str):
        """Configure common package_info settings for both GCC and Clang."""
        processor = self._get_target_processor(target)
        self.conf_info.define("tools.cmake.cmaketoolchain:system_name", "Linux")
        self.conf_info.define("tools.cmake.cmaketoolchain:system_processor", processor)
        self.conf_info.define("tools.gnu:host_triplet", target)
        self.conf_info.define("tools.build.cross_building:cross_build", True)

    def _package_info_gcc(self):
        """Configure package_info for GCC compiler."""
        target = "-".join(self._get_target_triplet())
        self._configure_common_package_info(target)
        bindir = os.path.join(self.package_folder, target, "bin")
        self.buildenv_info.prepend_path("PATH", bindir)
        compilers = {
            "c": f"{target}-gcc",
            "cpp": f"{target}-g++",
            "fortran": f"{target}-gfortran",
        }
        self.conf_info.update("tools.build:compiler_executables", compilers)

    def _package_info_clang(self):
        """Configure package_info for Clang compiler."""
        target = "-".join(self._get_target_triplet())
        self._configure_common_package_info(target)
        toolchain_dir = os.path.join(self.package_folder, target)
        bitness = self._get_target_bitness(target)
        link_flags = []
        toolchain_flags = []
        if Version(self.settings_target.compiler.version).major >= 16:
            gcc_version = self._get_gcc_version()
            toolchain_install_dirs = self._get_gcc_dir(toolchain_dir, target, gcc_version, bitness)
            toolchain_flags += ["--gcc-install-dir=" + toolchain_install_dirs[0]]
            link_flags += ["-L" + toolchain_install_dirs[0]]
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
        self.conf_info.update(
            "tools.cmake.cmaketoolchain:extra_variables",
            {
                "CMAKE_C_COMPILER_TARGET": target,
                "CMAKE_CXX_COMPILER_TARGET": target,
                "CMAKE_ASM_COMPILER_TARGET": target,
            },
        )

    def package_info(self):
        if self.settings_target.compiler == "gcc":
            self._package_info_gcc()
        elif self.settings_target.compiler in ("clang", "intel-cc"):
            self._package_info_clang()
        else:
            msg = f"Unsupported compiler '{self.settings_target.compiler}'. "
            msg += "This toolchain only supports GCC, Clang, and Intel icx."
            raise ConanInvalidConfiguration(msg)
