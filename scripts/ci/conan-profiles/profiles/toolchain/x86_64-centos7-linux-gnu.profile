include({{ os.path.join(profile_dir, "_cross-linux.profile") }})

[settings]
arch=x86_64
os.toolchain-vendor=centos7

[conf]
tools.cmake.cmaketoolchain:extra_variables*={"CPACK_DEBIAN_PACKAGE_ARCHITECTURE": "amd64"}
