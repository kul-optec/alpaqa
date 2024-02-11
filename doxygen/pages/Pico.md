# Raspberry Pi Pico support {#rpi-pico}

The alpaqa solvers can be used in embedded systems. This guide covers how to
build the alpaqa examples for the Raspberry Pi Pico microcontroller, which is
based on a dual-core ARM Cortex-M0+.

## Dependencies

The only required dependency is the Eigen linear algebra library. It can be
installed using the included `install-eigen.sh` script. Eigen is installed into
the `staging` directory.

```sh
./scripts/install-eigen.sh "$PWD/staging RelWithDebInfo"
```

## SDK and cross-compiler

You'll also need to install the Pico SDK and a suitable compiler:

```sh
mkdir -p ~/pico
git clone https://github.com/raspberrypi/pico-sdk ~/pico/pico-sdk --single-branch --branch 1.5.1
```

```sh
mkdir -p ~/opt
wget -O- https://github.com/tttapa/docker-arm-cross-toolchain/releases/latest/download/x-tools-arm-pico-eabi.tar.xz | tar xJ -C ~/opt
```

## Configure and build the examples

A CMake toolchain file is included in `cmake/toolchain/pico.cmake`, it selects
the correct options (disabling unsupported features such as loading of shared
libraries), and loads the Pico SDK with the right compiler.

```sh
export PICO_SDK_PATH="$HOME/pico/pico-sdk"
export PICO_GCC_TRIPLE="arm-pico-eabi"
export PICO_TOOLCHAIN_PATH="$HOME/opt/x-tools/arm-pico-eabi/bin"
cmake -S . -B build -G Ninja \
    --toolchain cmake/toolchain/pico.cmake \
    -D Eigen3_DIR="$PWD/staging/share/eigen3/cmake" \
    -D CMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Copy the example to the Pico

Connect the Pico over USB while pressing the BOOTSEL button to enter
programming mode. Then copy the UF2 file to the storage device that appears.

```sh
cp build/examples/C++/CustomCppProblem/custom-cpp-problem-example.uf2 /media/$USER/RPI-RP2
```

## Open the serial port output

```sh
screen /dev/ttyACM0 115200
```

You should see the solver progress and the solution printed.

## VSCode support

When using the CMake Tools extension in VSCode, you can select the
`RPi Pico ─ tttapa/arm-pico-eabi` kit to compile for the Pico. You can edit the
`.vscode/cmake-kits.json` file to set the paths to the compiler and the Pico SDK.
