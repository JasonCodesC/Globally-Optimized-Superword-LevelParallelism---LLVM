#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLUGIN="${ROOT}/build/GoSLPPass/GoSLPPass.dylib"

if [[ ! -f "${PLUGIN}" ]]; then
  echo "Missing pass plugin at ${PLUGIN}" >&2
  echo "Build first: cmake --build ${ROOT}/build --target GoSLPPass" >&2
  exit 1
fi

TMP_DIR="$(mktemp -d /tmp/goslp-validation.XXXXXX)"
trap 'rm -rf "${TMP_DIR}"' EXIT

run_case() {
  local name="$1"
  local func="$2"
  local expect_pattern="$3"
  local reject_pattern="$4"

  local src="${ROOT}/tests/kernels/${name}.c"
  local ll="${TMP_DIR}/${name}.ll"
  local goslp_ll="${TMP_DIR}/${name}.goslp.ll"
  local base_exe="${TMP_DIR}/${name}.base.exe"
  local goslp_exe="${TMP_DIR}/${name}.goslp.exe"

  clang -O3 -fno-slp-vectorize -fno-vectorize -ffp-contract=off \
    -emit-llvm -S "${src}" -o "${ll}"

  opt -load-pass-plugin="${PLUGIN}" \
    -passes="GoSLPPass(func:${func})" \
    -S "${ll}" -o "${goslp_ll}" >/dev/null 2>&1

  if [[ -n "${expect_pattern}" ]]; then
    if ! rg -q "${expect_pattern}" "${goslp_ll}"; then
      echo "[FAIL] ${name}: expected IR pattern not found: ${expect_pattern}" >&2
      exit 1
    fi
  fi

  if [[ -n "${reject_pattern}" ]]; then
    if rg -q "${reject_pattern}" "${goslp_ll}"; then
      echo "[FAIL] ${name}: unexpected IR pattern found: ${reject_pattern}" >&2
      exit 1
    fi
  fi

  clang -O3 -fno-slp-vectorize -fno-vectorize -ffp-contract=off \
    "${src}" -o "${base_exe}"
  "${base_exe}"

  clang -O3 -fno-slp-vectorize -fno-vectorize -ffp-contract=off \
    "${goslp_ll}" -o "${goslp_exe}"
  "${goslp_exe}"

  echo "[PASS] ${name}"
}

run_case pair_add_store foo_add2 "add <2 x i32>" ""
run_case pair_add4_store foo_add4 "store <2 x i32>" ""
run_case mismatch_ops foo_mismatch2 "" "(add|sub) <2 x i32>"

echo "All GoSLP validation cases passed."
