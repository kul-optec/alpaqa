{% set clang_suffix = os.getenv("TTTAPA_CONAN_PROFILES_CLANG_SUFFIX", "") %}
{% set _, clang_version, _ = detect_api.detect_clang_compiler(compiler_exe="clang" + clang_suffix) %}

[settings]
compiler=clang
compiler.version={{ detect_api.default_compiler_version("clang", clang_version) }}
compiler.cppstd=23
compiler.libcxx=libstdc++11
compiler.libcxx.gcc_version=15.2

[replace_requires]
llvm-openmp/*: llvm-openmp/[~{{ clang_version }}]

[conf]
tools.build:compiler_executables*={"c": "{{ "clang" + clang_suffix }}", "cpp": "{{ "clang++" + clang_suffix }}" }
llvm-openmp/*:tools.build:cflags+=["-fvisibility=default"]
llvm-openmp/*:tools.build:cxxflags+=["-fvisibility=default"]

[options]
llvm-openmp/*:shared=True
