[conf]
tools.cmake.cmake_layout:build_folder_vars=['const.python', 'settings.build_type']
[options]
alpaqa/*:with_ipopt=False
alpaqa/*:with_external_casadi=False
alpaqa/*:with_qpalm=False
alpaqa/*:with_cutest=False
alpaqa/*:with_python=True
[settings]
casadi/*:build_type=Release
