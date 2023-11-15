# Profiling {#page-profiling}

[TOC]

## Google gperftools

### Installation

https://github.com/gperftools/gperftools

```sh
sudo apt install libgoogle-perftools-dev gv
wget https://github.com/gperftools/gperftools/raw/master/src/pprof -O "$VIRTUAL_ENV/bin/pprof"
chmod +x "$_"
```

### Preparation

Compile alpaqa with `ALPAQA_NO_DLCLOSE=On` (see [gperftools/gperftools#787](https://github.com/gperftools/gperftools/issues/787)).

### Gather profile

Recommended to run many experiments to get representative samples.

```sh
CPUPROFILE=alpaqa.prof \
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so \
./build/src/RelWithDebInfo/alpaqa-driver \
    cs:"$(alpaqa cache path)/bcd8a11e-7e4b-11ee-9892-0f8255cce6e0/lib/alpaqa_problem.so" \
    alm.print_interval=0 num_exp=5000
```

```sh
CPUPROFILE=alpaqa.prof \
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so \
./build/src/RelWithDebInfo/alpaqa-driver \
    ./build/examples/problems/RelWithDebInfo/sparse-logistic-regression.so \
    problem.datafile=breast_cancer.csv num_exp=20000
```

### Visualize profile

```sh
pprof --gv ./build/src/RelWithDebInfo/alpaqa-driver alpaqa.prof
```

## Callgrind

### Installation

```sh
sudo apt install valgrind kcachegrind
```

### Gather profile

```sh
valgrind --tool=callgrind --dump-instr=yes --simulate-cache=yes --collect-jumps=yes \
./build/src/RelWithDebInfo/alpaqa-driver \
    cs:"$(alpaqa cache path)/bcd8a11e-7e4b-11ee-9892-0f8255cce6e0/lib/alpaqa_problem.so" \
    alm.print_interval=0 num_exp=10
```

### Visualize profile

```sh
kcachegrind callgrind.out.xxxxx
```

## VTune

### Installation

- https://www.intel.com/content/www/us/en/docs/vtune-profiler/get-started-guide/2023-1/overview.html
- https://www.intel.com/content/www/us/en/developer/articles/code-sample/vtune-profiler-sampling-driver-downloads.html
- https://www.intel.com/content/www/us/en/docs/vtune-profiler/user-guide/2023-2/build-install-sampling-drivers-for-linux-targets.html

### Gather profile

```sh
source ~/intel/oneapi/setvars.sh
vtune -collect hotspots -- ./build/src/RelWithDebInfo/alpaqa-driver \
    cs:"$(alpaqa cache path)/bcd8a11e-7e4b-11ee-9892-0f8255cce6e0/lib/alpaqa_problem.so" \
    alm.print_interval=0 num_exp=50000
```

### Visualize profile

```sh
vtune-gui r000hs
```

## Perf + Flamescope

- https://www.brendangregg.com/flamegraphs.html
- https://github.com/Netflix/flamescope

### Installation

```sh
sudo apt install linux-tools-$(uname -r)  # install perf
mkdir -p ~/opt
git clone https://github.com/Netflix/flamescope --single-branch --depth=1 ~/opt/flamescope
docker build -t flamescope ~/opt/flamescope
```

### Gather profile 

LBR mode for Intel CPUs (but does not seem to be 100% accurate).
```sh
echo 0 | sudo tee /proc/sys/kernel/perf_event_paranoid
perf record --call-graph lbr -- ./build/src/Release/alpaqa-driver \
    cs:"$(alpaqa cache path)/bcd8a11e-7e4b-11ee-9892-0f8255cce6e0/lib/alpaqa_problem.so" \
    alm.print_interval=0 num_exp=2000
t="$(stat -c %y perf.data)"; perf script --header > "profile/alpaqa-driver.${t// /-}"
```

DWARF mode (`--call-graph dwarf`) with default options does not produce
correct results. Increase the stack dump size to fix this. This increased dump
size may require decreasing the sampling rate (using the `-F` flag).
```sh
echo 0 | sudo tee /proc/sys/kernel/perf_event_paranoid
perf record --call-graph dwarf,65528 -F 100 -- ./build/src/Release/alpaqa-driver \
    cs:"$(alpaqa cache path)/bcd8a11e-7e4b-11ee-9892-0f8255cce6e0/lib/alpaqa_problem.so" \
    alm.print_interval=0 num_exp=2000
t="$(stat -c %y perf.data)"; perf script --header > "profile/alpaqa-driver.${t// /-}"
```

Alternatively, compile everything using `-fno-omit-frame-pointer` and use
`--call-graph fp`.

### Visualize profile

```sh
docker run --rm -it -v "$PWD/profile":/profiles:ro -p 5000:5000 flamescope
firefox http://localhost:5000
```

## Gprofng

- https://blogs.oracle.com/linux/post/gprofng-the-next-generation-gnu-profiling-tool
- https://sourceware.org/binutils/docs-2.41/gprofng.html
- https://www.gnu.org/software/binutils/

### Installation

```sh
wget https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.xz -O- | tar xJ -C ~/Downloads
pushd ~/Downloads/binutils-2.41
./configure --prefix="$HOME/opt/binutils-2.41"
make -j$(nproc)
make install
popd
```

### Gather profile

```sh
~/opt/binutils-2.41/bin/gprofng collect app \
./build/src/Release/alpaqa-driver \
    cs:"$(alpaqa cache path)/bcd8a11e-7e4b-11ee-9892-0f8255cce6e0/lib/alpaqa_problem.so" \
    alm.print_interval=0 num_exp=2000
```

### Analyze profile

```sh
~/opt/binutils-2.41/bin/gprofng display text -calltree test.1.er
```

To avoid line wrapping, pipe into [`bat`](https://github.com/sharkdp/bat), for example.
```sh
~/opt/binutils-2.41/bin/gprofng display text -calltree test.5.er | bat --wrap=never
```

<!--

---

## Results

### CUTEst "FEEDLOC"


#### PANOC
![](profiles/PANOC-CUTEst-FEEDLOC-2023-11-09_12-47-53.png)
#### Structured PANOC
![](profiles/StrucPANOC-CUTEst-FEEDLOC-2023-11-09_12-53-27.png)
#### ZeroFPR
![](profiles/ZeroFPR-CUTEst-FEEDLOC-2023-11-09_12-13-20.png)
#### PANTR
![](profiles/PANTR-CUTEst-FEEDLOC-2023-11-09_11-03-42.png)
#### Ipopt
![](profiles/Ipopt-CUTEst-FEEDLOC-2023-11-09_11-57-12.png)

### PANOC + CasADi "Hanging Chain" (SX)

#### PANOC
![](profiles/PANOC-CasADi-Hanging-Chain-SX-2023-11-09_11-18-48.png)
#### Structured PANOC
![](profiles/StrucPANOC-CasADi-Hanging-Chain-SX-2023-11-09_12-58-42)
#### LBFGSB
![](LBFGSB-CasADi-Hanging-Chain-SX-2023-11-09_11-48-02.png)
![](LBFGSB-CasADi-Hanging-Chain-SX-2023-11-09_11-48-02-Fortran.png)

-->
