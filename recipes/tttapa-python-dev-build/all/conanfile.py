import os

from conan import ConanFile
from conan.tools.gnu import (
    Autotools,
    AutotoolsToolchain,
    PkgConfigDeps,
    AutotoolsDeps,
)
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get
from conan.tools.build import cross_building


class CustomAutotoolsToolchain(AutotoolsToolchain):
    def environment(self):
        env = super().environment()
        if cross_building(self._conanfile):
            env.define_path("PKG_CONFIG_LIBDIR", self._conanfile.generators_folder)
        return env


class CPythonRecipe(ConanFile):
    name = "tttapa-python-dev-build"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
    }

    def export_sources(self):
        export_conandata_patches(self)

    def source(self):
        get(
            self,
            **self.conan_data["sources"][self.version],
            destination=self.source_folder,
            strip_root=True
        )
        apply_conandata_patches(self)

    def requirements(self):
        self.requires("zlib/1.3.1")

    def generate(self):
        pc = PkgConfigDeps(self)
        pc.generate()
        tc = AutotoolsDeps(self)
        tc.generate()
        # Python 3.8, 3.9 don't support prefix="/"
        tc = CustomAutotoolsToolchain(self, prefix="/usr")
        tc.configure_args.append("--enable-ipv6")
        tc.configure_args.append("--disable-test-modules")
        tc.configure_args.append("--with-ensurepip=no")
        tc.extra_ldflags.append("-Wl,-rpath,\\\\$\\$ORIGIN/../lib")
        tc.generate()

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def build(self):
        autotools = Autotools(self)
        autotools.configure()
        autotools.make()

    def package(self):
        autotools = Autotools(self)
        autotools.install(target="altinstall")

    def package_info(self):
        bindir = os.path.join(self.package_folder, "usr", "bin")
        self.buildenv_info.prepend_path("PATH", bindir)
        libdir = os.path.join(self.package_folder, "usr", "lib")
        self.runenv_info.prepend_path("LD_LIBRARY_PATH", libdir)
