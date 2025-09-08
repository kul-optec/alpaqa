include({{ os.path.join(profile_dir, "_harden.profile") }})
[settings]
build_type=Release
[conf]
tools.build:cflags+=["-U_FORTIFY_SOURCE", "-D_FORTIFY_SOURCE=3"]
tools.build:cxxflags+=["-U_FORTIFY_SOURCE", "-D_FORTIFY_SOURCE=3"]
