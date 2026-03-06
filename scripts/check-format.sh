#!/usr/bin/env bash
set -euo pipefail

files=$(find src tests -type f \( -name '*.cpp' -o -name '*.hpp' \) | sort)
if [[ -z "${files}" ]]; then
    exit 0
fi

clang-format -n --Werror ${files}
