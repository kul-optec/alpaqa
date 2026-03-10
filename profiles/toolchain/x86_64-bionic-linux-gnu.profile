include({{ os.path.join(profile_dir, "_cross-linux.profile") }})

[settings]
arch=x86_64
os.toolchain-vendor=bionic

[conf]
tools.cmake.cmaketoolchain:extra_variables*={"CPACK_DEBIAN_PACKAGE_ARCHITECTURE": "amd64"}
