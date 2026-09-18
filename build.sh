#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 {debug|release}" >&2
    exit 1
fi

case "$1" in
    debug)
        build_type="Debug"
        build_directory="build/debug"
        ;;
    release)
        build_type="Release"
        build_directory="build/release"
        ;;
    *)
        echo "Unknown build type: $1" >&2
        echo "Usage: $0 {debug|release}" >&2
        exit 1
        ;;
esac

cmake -S . -B "$build_directory" -DCMAKE_BUILD_TYPE="$build_type"
cmake --build "$build_directory" --parallel

echo "Build complete: $build_directory"