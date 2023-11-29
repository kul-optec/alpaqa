#!/usr/bin/env bash
cd "$( dirname "${BASH_SOURCE[0]}" )"/../..

if [ -z "${VIRTUAL_ENV+x}" ]; then
    echo "No active virtual environment, refusing to install."
    exit 1
fi

export CUTEST_ROOT="${VIRTUAL_ENV}/opt/CUTEst"
mkdir -p "$CUTEST_ROOT"
cd "$CUTEST_ROOT"

rm -rf archdefs
rm -rf sifdecode
rm -rf cutest
[ -d archdefs ] || git clone https://github.com/ralna/ARCHDefs --depth 1 --single-branch ./archdefs
[ -d sifdecode ] || git clone https://github.com/ralna/SIFDecode --depth 1 --single-branch ./sifdecode --branch v2.0.6
[ -d cutest ] || git clone https://github.com/ralna/CUTEst --depth 1 --single-branch ./cutest
# [ -d sif ] || git clone https://bitbucket.org/optrove/sif --depth 1 --single-branch ./sif
[ -d sif ] || {
    wget https://bitbucket.org/optrove/sif/get/master.tar.bz2 -O- | \
    tar xj && ln -s optrove-sif-* sif ; }
[ -d maros-meszaros ] || {
    wget https://bitbucket.org/optrove/maros-meszaros/get/master.tar.bz2 -O- | \
    tar xj && ln -s optrove-maros-meszaros-* maros-meszaros ; }
[ -d netlib-lp ] || {
    wget https://bitbucket.org/optrove/netlib-lp/get/master.tar.bz2 -O- | \
    tar xj && ln -s optrove-netlib-lp-* netlib-lp ; }

export ARCH="$CUTEST_ROOT/archdefs"
export ARCHDEFS="$CUTEST_ROOT/archdefs"
export CCOMPILER="ccompiler.pc64.lnx.gcc"
export COMPILER="compiler.pc64.lnx.gfo"
export CMP="gfo"
export CUTEST="$CUTEST_ROOT/cutest"
export GALAHAD_REQPREC="D"
export MACHINE="Intel-like PC with a 64-bit processor"
export MCH="pc64"
export OPSYS="Linux"
export OS="lnx"
export SIFDECODE="$CUTEST_ROOT/sifdecode"
export VERSION="${MCH}.${OS}.${CMP}"

source $ARCH/system.$OS
source $ARCH/$COMPILER
source $ARCH/$CCOMPILER

OPTIMIZATION='-O3 -march=native'
CXXOPT='-O3 -march=native'

if [ -n "$CROSS_COMPILE" ]; then
    AR="$(which ${CROSS_COMPILE}ar)"
    RANLIB="$(which ${CROSS_COMPILE}ranlib)"
    FORTRAN="$(which ${CROSS_COMPILE}gfortran)"
    CC="$(which ${CROSS_COMPILE}gcc)"
    CXX="$(which ${CROSS_COMPILE}g++)"
    echo "Cross-compiling using $FORTRAN"
fi

yesno_default_no() {
    echo "$1 (y/N)? "
    return 0
}
yesno_default_yes() {
    echo "$1 (Y/n)? "
    return 1
}
warning() {
    tput bold
    echo -e "\n WARNING: $1\n"
    tput sgr0
}
error() {
    tput bold
    echo -e "\n ERROR: $1\n"
    tput sgr0
}
success() {
    tput bold
    echo -e "\n SUCCESS: $1\n"
    tput sgr0
}
message() {
    tput bold
    echo -e "\n MESSAGE: $1\n"
    tput sgr0
}

COMPUSED=`$LS $ARCH/compiler.${MCH}.${OS}.gfo 2>/dev/null`
CCOMPUSED=`$LS $ARCH/ccompiler.${MCH}.${OS}.gcc 2>/dev/null`

dirs -c

echo ' Installing SIFDecode ...'
cd "$SIFDECODE"
source ./bin/install_sifdecode_main
status=$?
if [[ $status -ne 0 ]]; then
    error 'An error occurred while installing SIFDecode.'
    exit $status
fi

echo ' Installing CUTEst ...'
cd "$CUTEST"
source ./bin/install_cutest_main
status=$?
if [[ $status -ne 0 ]]; then
    error 'An error occurred while installing CUTEst.'
    exit $status
fi

echo 'Creating CUTEst shared library ...'
OBJ_DIR="$CUTEST/objects/$VERSION/double"
$FORTRAN -fPIC -rdynamic -shared -Wl,--whole-archive "$OBJ_DIR/libcutest.a" -Wl,--no-whole-archive -Wl,-soname,libcutest.so -o "$OBJ_DIR/libcutest.so"
status=$?
if [[ $status -ne 0 ]]; then
    error 'An error occurred creating the CUTEst shared library.'
    exit $status
fi
