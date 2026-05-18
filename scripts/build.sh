#!/bin/bash

set -e

export ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

BUILD_MODE="${1:-release}"

case "$BUILD_MODE" in
  debug|Debug)
    CMAKE_BUILD_TYPE="Debug"
    ;;
  release|Release)
    CMAKE_BUILD_TYPE="Release"
    ;;
  *)
    echo "Usage: $0 [debug|release|relwithdebinfo|minsizerel]"
    exit 1
    ;;
esac

cd "$ROOT_DIR"

mkdir -p build/NIAGKA
cd build/NIAGKA

cmake ../.. \
  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build . -j