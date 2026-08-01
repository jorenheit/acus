#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <example.cc>" >&2
  exit 2
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(dirname -- "$script_dir")"
build_dir="${ACUS_BUILD_DIR:-$project_dir/build/examples}"
example_name="$(basename -- "$1" .cc)"
source_file="$script_dir/$example_name.cc"

if [[ ! -f "$source_file" ]]; then
  echo "Unknown example: $1" >&2
  exit 2
fi

cmake -S "$project_dir" -B "$build_dir" -DACUS_BUILD_EXAMPLES=ON
cmake --build "$build_dir" --target "acus-example-$example_name"
