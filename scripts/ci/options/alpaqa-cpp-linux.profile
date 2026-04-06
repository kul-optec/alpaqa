[options]
alpaqa/*:with_ipopt=True
alpaqa/*:with_external_casadi=True
alpaqa/*:with_qpalm=True
alpaqa/*:with_cutest=True
alpaqa/*:with_json=True
alpaqa/*:with_python=False
alpaqa/*:with_examples=True
&:shared=True
openblas/*:no_fortran=False

[settings]
casadi/*:build_type=Release

[replace_requires]
openblas/*:openblas/tttapa.0.3.32
