#!/usr/bin/env bash
set -euo pipefail

# compile_dataHandler.sh
# Small helper to compile just the current file: Embedded-dev/dataHandler.cpp
# Usage:
#   ./compile_dataHandler.sh         # compile to build/dataHandler.o
#   ./compile_dataHandler.sh link    # also link into build/dataHandler (executable)
#
# It prefers the compiler in $CC (you can export CC=avr-g++ or another toolchain).

SRC="Embedded-dev/dataHandler.cpp"
OUT_DIR="build"
OBJ="$OUT_DIR/dataHandler.o"
BIN="$OUT_DIR/dataHandler"

mkdir -p "$OUT_DIR"

# Allow overriding compiler via environment (e.g. CC=avr-g++)
: ${CC:=g++}
echo "Using compiler: $CC"

echo "Compiling $SRC -> $OBJ"
$CC -c "$SRC" -o "$OBJ" -Wall -Wextra -O2 -std=c++17

if [ "${1:-}" = "link" ]; then
  echo "Linking $OBJ -> $BIN"
  $CC "$OBJ" -o "$BIN"
  echo "Built executable: $BIN"
fi

echo "Done. Object: $OBJ"
