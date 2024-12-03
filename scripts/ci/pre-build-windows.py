import configparser
import os
from pathlib import Path
from subprocess import run
import sysconfig

project_dir = Path(__file__).parent.parent.parent
os.chdir(project_dir)

native_platform = sysconfig.get_platform()
plat_name = ""
dist_extra_conf = os.getenv("DIST_EXTRA_CONFIG")
if dist_extra_conf is not None:
    distcfg = configparser.ConfigParser()
    distcfg.read(dist_extra_conf)
    plat_name = distcfg.get("build_ext", "plat_name", fallback="")

print("build_ext.plat_name:", plat_name)
print("native platform:    ", native_platform)

if not plat_name:
    plat_name = native_platform

can_run = (plat_name, native_platform) in {
    ("win32", "win32"),
    ("win32", "win-amd64"),
    ("win-amd64", "win-amd64"),
    ("win-arm32", "win-arm32"),
    ("win-arm64", "win-arm64"),
}
cmake_processor = {
    "win32": "x86",
    "win-amd64": "AMD64",
    "win-arm32": "ARM",
    "win-arm64": "ARM64",
}[plat_name]
conan_arch = {
    "win32": "x86",
    "win-amd64": "x86_64",
    "win-arm32": "armv7",
    "win-arm64": "armv8",
}[plat_name]
cpu_flags = {
    "win32": ["/arch:SSE2"],
    "win-amd64": ["/arch:AVX2"],
    "win-arm32": [],
    "win-arm64": [],
}[plat_name]

native_profile = f"""\
include(default)
include({project_dir.as_posix()}/scripts/ci/alpaqa-python-windows.profile)
[settings]
arch={conan_arch}
build_type=Release
[conf]
tools.build:skip_test=True
tools.build:cflags+={cpu_flags}
tools.build:cxxflags+={cpu_flags}
"""
cross_profile = native_profile
cross_profile += f"""\
tools.build.cross_building:can_run={can_run}
tools.cmake.cmaketoolchain:system_name=Windows
tools.cmake.cmaketoolchain:system_processor={cmake_processor}
"""

cross = not can_run
profile = cross_profile if cross else native_profile
Path("cibw.profile").write_text(profile)
print(profile)

opts = dict(shell=True, check=True)
if not Path("recipes").exists():
    run(f"git clone https://github.com/tttapa/conan-recipes recipes", **opts)
    run(f"conan remote add tttapa-conan-recipes recipes --force", **opts)
for c in ("RelWithDebInfo", "Release"):
    run(
        "conan install . -pr:h ./cibw.profile --build=missing -s build_type=" + c,
        **opts,
    )
