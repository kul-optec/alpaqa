include({{ os.path.join(profile_dir, "_cross-linux.profile") }})

[settings]
arch=armv7hf
os.toolchain-vendor=neon

[conf]
tools.cmake.cmaketoolchain:extra_variables*={"CPACK_DEBIAN_PACKAGE_ARCHITECTURE": "armhf"}
