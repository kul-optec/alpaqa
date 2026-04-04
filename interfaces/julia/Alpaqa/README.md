# Alpaqa.jl

To build the JLL package locally, use
```sh
alpaqa_dir="$PWD"/alpaqa
alpaqa_jl_dir="$alpaqa_dir"/interfaces/julia
[ -d "$alpaqa_dir" ] || git clone https://github.com/kul-optec/alpaqa.git "$alpaqa_dir"
pushd /tmp
julia "$alpaqa_jl_dir"/build_tarballs.jl $(gcc -print-multiarch) --deploy=local --verbose
popd
julia --project="$alpaqa_jl_dir/Alpaqa" -e "
using Pkg
jll = joinpath(DEPOT_PATH[1], \"dev\", \"Alpaqa_jll\")
Pkg.develop(path=jll)
Pkg.instantiate()
"
julia --project="$alpaqa_jl_dir/Alpaqa" -e "
using Pkg
Pkg.test()
"
```
