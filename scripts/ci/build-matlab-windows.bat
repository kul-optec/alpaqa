@echo off
setlocal enabledelayedexpansion

:: Change to the script directory's parent directory
cd /d "%~dp0\..\.."

:: Set default values for parameters
set "matlab_dir=%~1"
if "%matlab_dir%"=="" set "matlab_dir=C:\Program Files\MATLAB"

set "triple=%~2"
if "%triple%"=="" set "triple=amd64"

set "pkg_dir=%~3"
if "%pkg_dir%"=="" set "pkg_dir=."

set "out_dir=%~4"
if "%out_dir%"=="" set "out_dir=staging\matlab"

:: Source the Visual Studio environment
set "vcvarsall_path=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
call "%vcvarsall_path%" %triple% || exit /b 1

:: Create Conan profiles
set "matlab_profile=%CD%\profile-matlab.local.conan"
> "%matlab_profile%" (
    @echo.include^(%CD%\scripts\ci\profiles\windows-%triple%.profile^)
    @echo.include^(%CD%\scripts\ci\profiles\alpaqa-matlab.profile^)
)

:: Remove temporary folders
for %%C in (debug release) do (
    if exist "%pkg_dir%\build\matlab-%%C\generators" (
        rmdir /s /q "%pkg_dir%\build\matlab-%%C\generators"
    )
    if exist "%pkg_dir%\build\matlab-%%C\CMakeCache.txt" (
        del /f /q "%pkg_dir%\build\matlab-%%C\CMakeCache.txt"
    )
)

:: Install dependencies
for %%C in (Release) do (
    conan install "%pkg_dir%" --build=missing ^
        -pr:h "%matlab_profile%" ^
        -s build_type=%%C ^
        || exit /b 1
)

:: Build MATLAB bindings
pushd "%pkg_dir%"
cmake --preset conan-matlab-release ^
    -D CMAKE_FIND_ROOT_PATH="%matlab_dir%" ^
    -D CMAKE_C_COMPILER_LAUNCHER=sccache ^
    -D CMAKE_CXX_COMPILER_LAUNCHER=sccache ^
    || exit /b 1
cmake --build --preset conan-matlab-release ^
    -t alpaqa_mex -v ^
    || exit /b 1
cmake --install build\matlab-release ^
    --component mex_interface --prefix "%out_dir%" ^
    || exit /b 1
popd
