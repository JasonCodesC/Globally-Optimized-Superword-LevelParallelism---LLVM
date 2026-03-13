#!/usr/bin/env python3
import csv
import os
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def read_runtime(path: Path):
    rows = []
    with path.open() as f:
        for row in csv.DictReader(f):
            rows.append(
                {
                    "kernel": row["kernel"],
                    "baseline_us": float(row["baseline_us"]),
                    "goslp_us": float(row["goslp_us"]),
                    "speedup": float(row["speedup"]),
                }
            )
    return rows


def read_compile(path: Path):
    rows = []
    with path.open() as f:
        for row in csv.DictReader(f):
            rows.append(
                {
                    "variant": row["variant"],
                    "build_seconds": float(row["build_seconds"]),
                    "opt_seconds": float(row["opt_seconds"]),
                }
            )
    return rows


def plot_runtime(rows, out_dir: Path):
    kernels = [r["kernel"] for r in rows]
    baseline = [r["baseline_us"] for r in rows]
    goslp = [r["goslp_us"] for r in rows]
    speedup = [r["speedup"] for r in rows]

    x = range(len(kernels))

    plt.figure(figsize=(10, 4))
    w = 0.38
    plt.bar([i - w / 2 for i in x], baseline, width=w, label="LLVM SLP", color="#355070")
    plt.bar([i + w / 2 for i in x], goslp, width=w, label="GoSLP", color="#6D597A")
    plt.xticks(list(x), kernels, rotation=20, ha="right")
    plt.ylabel("Median runtime (us)")
    plt.title("GoSLP vs LLVM SLP Runtime")
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "runtime_comparison.png", dpi=180)
    plt.close()

    plt.figure(figsize=(10, 4))
    bars = plt.bar(kernels, speedup, color="#B56576")
    plt.axhline(1.0, color="#333333", linestyle="--", linewidth=1)
    plt.ylabel("Speedup (baseline/goslp)")
    plt.title("GoSLP Speedup per Kernel")
    plt.xticks(rotation=20, ha="right")
    for b, s in zip(bars, speedup):
        plt.text(b.get_x() + b.get_width() / 2, b.get_height(), f"{s:.2f}x", ha="center", va="bottom", fontsize=8)
    plt.tight_layout()
    plt.savefig(out_dir / "runtime_speedup.png", dpi=180)
    plt.close()


def plot_compile(rows, out_dir: Path):
    variants = [r["variant"] for r in rows]
    build = [r["build_seconds"] for r in rows]
    opt = [r["opt_seconds"] for r in rows]

    x = range(len(variants))
    plt.figure(figsize=(7, 4))
    plt.bar(x, build, width=0.5, color="#264653", label="Total build")
    plt.bar(x, opt, width=0.5, color="#2A9D8F", label="opt stage")
    plt.xticks(list(x), variants)
    plt.ylabel("Seconds")
    plt.title("Compile-time Comparison")
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "compile_time.png", dpi=180)
    plt.close()


def main():
    root = Path(__file__).resolve().parent
    out_dir = root / "results"
    runtime_csv = out_dir / "runtime.csv"
    compile_csv = out_dir / "compile.csv"

    if not runtime_csv.exists() or not compile_csv.exists():
      raise SystemExit("Missing runtime.csv or compile.csv in bench/results")

    runtime_rows = read_runtime(runtime_csv)
    compile_rows = read_compile(compile_csv)
    plot_runtime(runtime_rows, out_dir)
    plot_compile(compile_rows, out_dir)


if __name__ == "__main__":
    main()
