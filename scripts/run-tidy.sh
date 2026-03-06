#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug >/dev/null
files=$(find src tests -type f \( -name '*.cpp' -o -name '*.hpp' \) | sort)
if [[ -z "${files}" ]]; then
    exit 0
fi

clang-tidy -p build ${files}
