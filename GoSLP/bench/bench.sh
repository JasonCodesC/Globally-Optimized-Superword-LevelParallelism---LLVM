#!/usr/bin/env bash
set -euo pipefail

if [ $# -gt 2 ]; then
  echo "Usage: $0 [kernel (vec4_add|saxpy3|dot64|nested)] [iters]" >&2
  exit 1
fi

kernel="${1:-vec4_add}"
iters="${2:-0}"

export PATH="/opt/homebrew/opt/llvm/bin:${PATH}"
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/llvm:${CMAKE_PREFIX_PATH:-}"
export SDKROOT="${SDKROOT:-$(xcrun --show-sdk-path 2>/dev/null || true)}"
sysroot_args=()
if [ -n "${SDKROOT}" ]; then
  sysroot_args+=(-isysroot "${SDKROOT}")
fi

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${root}"

# Build the GoSLP pass (can be skipped if SKIP_BUILD=1 and artifacts exist)
if [ "${SKIP_BUILD:-0}" != "1" ]; then
  mkdir -p build && cd build
  cmake -DPASS_DIR=GoSLPPass ..
  cmake --build . --target GoSLPPass
  cd "${root}"
fi

bench_src="${root}/bench/bench.cpp"
clangxx_flags=(-std=c++17 -O3 -fno-slp-vectorize -ffp-contract=off "${sysroot_args[@]}")

run_bin() {
  local exe="$1"
  shift
  echo "---- $exe ----"
  "./${exe}" "$@"
}

# Baseline (no GoSLP)
clang++ "${clangxx_flags[@]}" "${bench_src}" -o bench_baseline

# With GoSLP
clang++ "${clangxx_flags[@]}" -emit-llvm -S "${bench_src}" -o bench.ll
opt -load-pass-plugin="${root}/build/GoSLPPass/GoSLPPass.dylib" -passes=GoSLPPass -S bench.ll -o bench.goslp.ll
clang++ "${clangxx_flags[@]}" bench.goslp.ll -o bench_goslp

# Run both variants
args=("$kernel")
if [ "${iters}" != "0" ]; then
  args+=("${iters}")
fi
run_bin bench_baseline "${args[@]}"
run_bin bench_goslp "${args[@]}"
