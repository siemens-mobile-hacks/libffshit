#!/bin/bash
set -e
set -x
cd "$(dirname $0)/../../"

emcmake cmake -B build_wasm -DUSE_BUILTIN_LIBFMT=TRUE
cmake --build build_wasm
pnpm run build
