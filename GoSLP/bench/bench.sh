#!/usr/bin/env bash
set -euo pipefail

kernel_arg="${1:-all}"
iters="${2:-2000}"
n="${3:-8192}"
repeats="${4:-3}"

if [[ "${repeats}" -lt 1 ]]; then
  repeats=1
fi

export PATH="/opt/homebrew/opt/llvm/bin:${PATH}"
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/llvm:${CMAKE_PREFIX_PATH:-}"
export SDKROOT="${SDKROOT:-$(xcrun --show-sdk-path 2>/dev/null || true)}"
sysroot_args=()
if [[ -n "${SDKROOT}" ]]; then
  sysroot_args+=( -isysroot "${SDKROOT}" )
fi

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${root}"

if [[ "${SKIP_BUILD:-0}" != "1" ]]; then
  cmake -S . -B build -DPASS_DIR=GoSLPPass
  cmake --build build --target GoSLPPass -j8
fi

plugin="${root}/build/GoSLPPass/GoSLPPass.dylib"
bench_src="${root}/bench/suite.cpp"
results_dir="${root}/bench/results"
mkdir -p "${results_dir}"

common_flags=( -std=c++17 -O3 -march=native -ffp-contract=off "${sysroot_args[@]}" )
baseline_exe="${results_dir}/bench_baseline"
goslp_exe="${results_dir}/bench_goslp"

measure_real_seconds() {
  local log_file="$1"
  awk '/^real /{print $2}' "${log_file}"
}

# Baseline build + asm
baseline_time_log="${results_dir}/compile_baseline.time"
/usr/bin/time -p -o "${baseline_time_log}" \
  clang++ "${common_flags[@]}" "${bench_src}" -o "${baseline_exe}"

clang++ "${common_flags[@]}" -S "${bench_src}" -o "${results_dir}/baseline.s"

# GoSLP build pipeline + asm
ll_in="${results_dir}/suite.ll"
ll_out="${results_dir}/suite.goslp.ll"

front_time_log="${results_dir}/compile_goslp_front.time"
opt_time_log="${results_dir}/compile_goslp_opt.time"
link_time_log="${results_dir}/compile_goslp_link.time"

/usr/bin/time -p -o "${front_time_log}" \
  clang++ "${common_flags[@]}" -fno-slp-vectorize -emit-llvm -S "${bench_src}" -o "${ll_in}"

/usr/bin/time -p -o "${opt_time_log}" \
  opt -load-pass-plugin="${plugin}" -passes="GoSLPPass(func:block8)" -S "${ll_in}" -o "${ll_out}" >/dev/null 2>&1

/usr/bin/time -p -o "${link_time_log}" \
  clang++ "${common_flags[@]}" "${ll_out}" -o "${goslp_exe}"

clang++ "${common_flags[@]}" -S "${ll_out}" -o "${results_dir}/goslp.s"

baseline_build_s="$(measure_real_seconds "${baseline_time_log}")"
front_s="$(measure_real_seconds "${front_time_log}")"
opt_s="$(measure_real_seconds "${opt_time_log}")"
link_s="$(measure_real_seconds "${link_time_log}")"
goslp_build_s="$(python3 - <<PY
print(float("${front_s}") + float("${opt_s}") + float("${link_s}"))
PY
)"

asm_baseline_inst="$(grep -E '^[[:space:]]+[a-zA-Z]' "${results_dir}/baseline.s" | wc -l | tr -d ' ')"
asm_goslp_inst="$(grep -E '^[[:space:]]+[a-zA-Z]' "${results_dir}/goslp.s" | wc -l | tr -d ' ')"

printf "variant,build_seconds,opt_seconds,asm_instruction_lines\n" > "${results_dir}/compile.csv"
printf "llvm_slp,%s,0,%s\n" "${baseline_build_s}" "${asm_baseline_inst}" >> "${results_dir}/compile.csv"
printf "goslp,%s,%s,%s\n" "${goslp_build_s}" "${opt_s}" "${asm_goslp_inst}" >> "${results_dir}/compile.csv"

kernels=(array_sum dot_product rolling_sum vwap variance covariance)
if [[ "${kernel_arg}" != "all" ]]; then
  kernels=("${kernel_arg}")
fi

printf "kernel,baseline_us,goslp_us,speedup,baseline_checksum,goslp_checksum\n" > "${results_dir}/runtime.csv"

run_median() {
  local exe="$1"
  local kernel="$2"
  local times=()
  local checksum=""

  for ((r=0; r<repeats; ++r)); do
    local out
    out="$(${exe} "${kernel}" "${iters}" "${n}")"
    local t
    t="$(echo "${out}" | sed -n 's/.*time_us=\([0-9]*\).*/\1/p')"
    local c
    c="$(echo "${out}" | sed -n 's/.*checksum=\([^ ]*\).*/\1/p')"
    if [[ -z "${t}" || -z "${c}" ]]; then
      echo "failed_to_parse_output ${exe} ${kernel}: ${out}" >&2
      exit 1
    fi
    times+=("${t}")
    checksum="${c}"
  done

  IFS=$'\n' read -r -d '' -a sorted < <(printf "%s\n" "${times[@]}" | sort -n && printf '\0')
  local mid=$(( ${#sorted[@]} / 2 ))
  local median="${sorted[$mid]}"
  printf "%s;%s" "${median}" "${checksum}"
}

echo "kernel, baseline_us, goslp_us, speedup"
for k in "${kernels[@]}"; do
  base_pair="$(run_median "${baseline_exe}" "${k}")"
  goslp_pair="$(run_median "${goslp_exe}" "${k}")"

  base_us="${base_pair%%;*}"
  base_checksum="${base_pair##*;}"
  goslp_us="${goslp_pair%%;*}"
  goslp_checksum="${goslp_pair##*;}"

  python3 - <<PY
import math
b=float("${base_checksum}")
g=float("${goslp_checksum}")
if not math.isclose(b, g, rel_tol=1e-6, abs_tol=1e-4):
    raise SystemExit(f"checksum mismatch for ${k}: baseline={b} goslp={g}")
PY

  speedup="$(python3 - <<PY
b=float("${base_us}")
g=float("${goslp_us}")
print(0.0 if g == 0 else b/g)
PY
)"

  printf "%s,%s,%s,%s,%s,%s\n" "${k}" "${base_us}" "${goslp_us}" "${speedup}" "${base_checksum}" "${goslp_checksum}" >> "${results_dir}/runtime.csv"
  printf "%s, %s, %s, %.4fx\n" "${k}" "${base_us}" "${goslp_us}" "${speedup}"
done

MPLCONFIGDIR=/tmp/mpl-go-slp python3 "${root}/bench/plot_results.py"

echo "Wrote ${results_dir}/runtime.csv and ${results_dir}/compile.csv"
