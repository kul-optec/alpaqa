#!/usr/bin/env pwsh
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Set-Location (Join-Path $PSScriptRoot "..\..")

$env:CTEST_OUTPUT_ON_FAILURE = "1"

# Select architecture
$triple = if ($args.Count -ge 1) { $args[0] } else { "windows-amd64" }
$tests  = if ($args.Count -ge 2) { $args[1] } else { "" }

$test_flags = if ($tests) { "-DALPAQA_FORCE_TEST_DISCOVERY=On" } else { "" }

# Create Conan profile
$cpp_profile = Join-Path (Get-Location) "profile-cpp.conan"
@"
include($(Get-Location)/scripts/ci/profiles/$triple.profile)
include($(Get-Location)/scripts/ci/profiles/alpaqa-cpp-windows.profile)
[conf]
tools.cmake.cmake_layout:build_folder_vars=['const.pkg']
!&:tools.build:skip_test=True
&:tools.build:skip_test=False
&:tools.env.virtualenv:powershell=pwsh
&:tools.cmake.cmaketoolchain:extra_variables*={"CMAKE_MSVC_DEBUG_INFORMATION_FORMAT": "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"}
[buildenv]
&:CMAKE_C_COMPILER_LAUNCHER=sccache
&:CMAKE_CXX_COMPILER_LAUNCHER=sccache
[replace_requires]
eigen/*: eigen/3.4.0
"@ | Set-Content -NoNewline $cpp_profile

foreach ($cfg in @("Debug", "Release")) {
    # Dependencies
    conan install . --build=missing -pr:h "$cpp_profile" -s build_type=$cfg
    . ./build/pkg/generators/conanbuild.ps1
    # Configure
    cmake --preset conan-pkg --fresh $test_flags
    # Build
    cmake --build build/pkg -j --config $cfg
    # Test
    if ($tests) {
        ctest -C $cfg --test-dir build/pkg -D $tests
    }
    . ./build/pkg/generators/deactivate_conanbuild.ps1
}

# Package
Push-Location build/pkg
. ./generators/conanbuild.ps1
cpack -G 'ZIP' -C "Release;Debug"
Pop-Location
