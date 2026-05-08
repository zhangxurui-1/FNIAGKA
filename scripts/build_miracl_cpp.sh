#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
export MIRACL_DIR="$ROOT_DIR/deps/miracl"
export SOURCE_DIR="$MIRACL_DIR/source"
export BUILD_DIR="$ROOT_DIR/build/miracl"
export PAIRING_DIR="$SOURCE_DIR/curve/pairing"

# Copy miracl headers to include/miracl
rm -rf "$ROOT_DIR/include/miracl"
mkdir -p "$ROOT_DIR/include/miracl"
cp "$MIRACL_DIR"/include/* "$ROOT_DIR/include/miracl/"
rm -f "$ROOT_DIR/include/miracl/mirdef.h"
cp "$ROOT_DIR/mirdef.h" "$ROOT_DIR/include/miracl/"

# Build miracl cpp
rm -rf "$BUILD_DIR/cpp"
mkdir -p "$BUILD_DIR/cpp"
cd "$BUILD_DIR/cpp"

g++ -O2 -fPIC -c -I"$ROOT_DIR/include/miracl" \
    "$SOURCE_DIR/big.cpp" "$SOURCE_DIR/brick.cpp" "$SOURCE_DIR/crt.cpp" "$SOURCE_DIR/ebrick.cpp" \
    "$SOURCE_DIR/ebrick2.cpp" "$SOURCE_DIR/ec2.cpp" "$SOURCE_DIR/ecn.cpp" "$SOURCE_DIR/ecnzzn.cpp" \
    "$SOURCE_DIR/flash.cpp" "$SOURCE_DIR/floating.cpp" "$SOURCE_DIR/zzn.cpp" "$SOURCE_DIR/gf2m.cpp" \
    "$PAIRING_DIR/bls_pair.cpp" "$PAIRING_DIR/ecn4.cpp" "$PAIRING_DIR/zzn2.cpp" "$PAIRING_DIR/zzn4.cpp" \
    "$PAIRING_DIR/zzn8.cpp" "$PAIRING_DIR/zzn24.cpp"

ar rcs libMIRACL-cpp.a *.o
mv *.a ../
