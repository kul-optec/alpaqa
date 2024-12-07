[conf]
tools.cmake.cmake_layout:build_folder_vars=['const.python', 'settings.build_type']

[options]
alpaqa/*:with_ipopt=True
alpaqa/*:with_external_casadi=True
alpaqa/*:with_qpalm=True
alpaqa/*:with_cutest=True
alpaqa/*:with_json=False
alpaqa/*:with_python=True

[settings]
casadi/*:build_type=Release
