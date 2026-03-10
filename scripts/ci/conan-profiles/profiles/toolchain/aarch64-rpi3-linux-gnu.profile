include({{ os.path.join(profile_dir, "_cross-linux.profile") }})

[settings]
arch=armv8
os.toolchain-vendor=rpi3

[conf]
tools.cmake.cmaketoolchain:extra_variables*={"CPACK_DEBIAN_PACKAGE_ARCHITECTURE": "arm64"}
