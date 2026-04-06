import os

from conan import ConanFile
from conan.tools.gnu import (
    Autotools,
    AutotoolsToolchain,
    PkgConfigDeps,
    AutotoolsDeps,
)
from conan.tools.files import (
    apply_conandata_patches,
    export_conandata_patches,
    get,
    copy,
    load,
    save,
)
from conan.tools.scm import Version
from conan.tools.build import cross_building


class CustomAutotoolsToolchain(AutotoolsToolchain):
    def environment(self):
        env = super().environment()
        if cross_building(self._conanfile):
            env.define_path("PKG_CONFIG_LIBDIR", self._conanfile.generators_folder)
        return env


class CPythonRecipe(ConanFile):
    name = "tttapa-python-dev"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    # https://github.com/conan-io/conan/issues/19108
    package_id_non_embed_mode = "full_mode"
    options = {
        "with_bin": [True, False],
        "disable_gil": [True, False],  # affects ABI
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "with_bin": False,
        "disable_gil": False,
        "shared": True,
        "fPIC": True,
    }

    def export_sources(self):
        copy(self, "config.site", self.recipe_folder, self.export_sources_folder)
        copy(self, "FindPython.cmake", self.recipe_folder, self.export_sources_folder)
        export_conandata_patches(self)

    def source(self):
        get(
            self,
            **self.conan_data["sources"][self.version],
            destination=self.source_folder,
            strip_root=True,
        )
        apply_conandata_patches(self)

    def build_requirements(self):
        self.tool_requires(f"tttapa-python-dev-build/{self.version}")

    def requirements(self):
        self.requires("zlib/1.3.1")

    def generate(self):
        pc = PkgConfigDeps(self)
        pc.generate()
        tc = AutotoolsDeps(self)
        tc.generate()
        tc = CustomAutotoolsToolchain(self, prefix="/usr")

        build_python_dep = self.dependencies.direct_build["tttapa-python-dev-build"]
        build_python_v = Version(build_python_dep.ref.version)
        python_majmin = f"python{build_python_v.major}.{build_python_v.minor}"
        build_python_root = os.path.join(build_python_dep.package_folder, "usr")
        build_python_bin = os.path.join(build_python_root, "bin")
        build_python = os.path.join(build_python_bin, python_majmin)
        config_site = os.path.join(self.source_folder, "config.site")
        tc.configure_args.append("--with-build-python=" + build_python)
        tc.configure_args.append("--enable-ipv6")
        tc.configure_args.append("--with-ensurepip=no")
        tc.configure_args.append("--with-openssl=no-i-do-not-want-openssl")
        tc.configure_args.append("CONFIG_SITE=" + config_site)
        tc.extra_ldflags.append("-Wl,-rpath,\\\\$\\$ORIGIN/../lib")
        if not self.options.with_bin:
            tc.configure_args.append("--with-static-libpython=no")
        if self.options.get_safe("disable_gil"):
            tc.configure_args.append("--disable-gil")
        tc.generate()

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.get_safe("shared"):
            self.options.rm_safe("fPIC")
        if not self.options.get_safe("with_bin"):
            self.options.rm_safe("shared")
        if Version(self.version) < "3.13":
            self.options.rm_safe("disable_gil")

    def build(self):
        autotools = Autotools(self)
        autotools.configure()
        if self.options.with_bin:
            autotools.make()

    def _get_find_python_vars(self, pfx):
        root = os.path.join(self.package_folder, "usr")
        python_v = Version(self.ref.version)
        abi = "t" if self.options.get_safe("disable_gil") else ""
        python_maj = f"python{python_v.major}"
        python_majmin = f"{python_maj}.{python_v.minor}"
        inc = os.path.join(root, "include", python_majmin + abi)
        static = bin = lib = slib = None
        if self.options.with_bin:
            bin = os.path.join(root, "bin", python_majmin + abi)
            if self.options.shared:
                static = "FALSE"
                lib = os.path.join(root, "lib", f"lib{python_majmin + abi}.so")
                slib = os.path.join(root, "lib", f"lib{python_maj + abi}.so")
            else:
                static = "TRUE"
                lib = os.path.join(root, "lib", f"lib{python_majmin + abi}.a")
        vars = {
            f"{pfx}_ROOT_DIR": root,
            f"{pfx}_USE_STATIC_LIBS": static or "",
            f"{pfx}_FIND_STRATEGY": "LOCATION",
            f"{pfx}_FIND_IMPLEMENTATIONS": "CPython",
            # Setting FindPython artifacts breaks SOABI
            # f"{pfx}_EXECUTABLE": bin or f"{pfx}_EXECUTABLE-NOTFOUND",
            # f"{pfx}_INCLUDE_DIR": inc,
            # f"{pfx}_LIBRARY": lib or f"{pfx}_LIBRARY-NOTFOUND",
            # f"{pfx}_SABI_LIBRARY": slib or f"{pfx}_SABI_LIBRARY-NOTFOUND",
        }
        if self.options.get_safe("disable_gil"):
            vars[f"{pfx}_FIND_ABI"] = "OFF;OFF;OFF;ON"
        return "\n".join(f'set({k} "{v}")' for k, v in vars.items())

    def package(self):
        autotools = Autotools(self)
        autotools.install(target="libainstall")  # python3.x-config
        autotools.install(target="inclinstall")  # Python.h
        if self.options.with_bin:
            autotools.install(target="altbininstall")  # python3.x
        find_python = os.path.join(self.source_folder, "FindPython.cmake")
        root = os.path.join(self.package_folder, "usr")
        cmake_dir = os.path.join(root, "share", "cmake", "Modules")
        for pfx in "Python", "Python3":
            find_python_hints = self._get_find_python_vars(pfx)
            content = load(self, find_python)
            content = content.replace("@FIND_PYTHON_HINTS@", find_python_hints)
            save(self, os.path.join(cmake_dir, f"Find{pfx}.cmake"), content)

    def package_info(self):
        root = os.path.join(self.package_folder, "usr")
        cmake_dir = os.path.join(root, "share", "cmake", "Modules")
        self.cpp_info.builddirs = [cmake_dir]
