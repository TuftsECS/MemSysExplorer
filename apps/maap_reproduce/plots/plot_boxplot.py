#!/usr/bin/env python3
"""CPU2026 fprate compiler-sensitivity boxplot: per-workload read/write %
shift vs the -O2 baseline across the -Og/-O1/-O2/-O3 sweep.
Output: figures_out/cpu2026_boxplot.pdf"""
import csv
from pathlib import Path
from statistics import mean
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Patch

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data" / "cpu2026_fprate.csv"
OUT = ROOT / "figures_out" / "cpu2026_boxplot.pdf"
LIMIT = 100.0
BLUE, ORANGE = "#1f77b4", "#ff7f0e"


def num(x):
    try:
        return float(x)
    except (TypeError, ValueError):
        return None


def variance(rows, baseline="2"):
    by_wl = {}
    for r in rows:
        re, wr = num(r["total_reads"]), num(r["total_writes"])
        if re is None or wr is None or (re == 0 and wr == 0):
            continue
        by_wl.setdefault(r["workload"], {}).setdefault(r["opt_level_numeric"], []).append((re, wr))

    out = []
    for wl, tags in by_wl.items():
        if baseline not in tags:
            continue
        avg = {t: (mean(v[0] for v in vs), mean(v[1] for v in vs)) for t, vs in tags.items()}
        base_r, base_w = avg[baseline]
        others = [v for t, v in avg.items() if t != baseline]
        if not others:
            continue
        reads = [100 * (v[0] - base_r) / base_r for v in others if base_r > 0]
        writes = [100 * (v[1] - base_w) / base_w for v in others if base_w > 0]
        spread = (max((abs(v) for v in reads), default=0)
                  + max((abs(v) for v in writes), default=0))
        clip = lambda vals: [max(min(v, LIMIT), -LIMIT) for v in vals]
        out.append({"wl": wl, "reads": clip(reads), "writes": clip(writes), "spread": spread})
    out.sort(key=lambda r: r["spread"], reverse=True)
    return out


def box(ax, data, pos, color):
    bp = ax.boxplot(data, positions=pos, widths=0.36, patch_artist=True,
                    showfliers=False, medianprops=dict(color="black", linewidth=1.6),
                    whiskerprops=dict(color="black", linewidth=1.1),
                    capprops=dict(color="black", linewidth=1.1),
                    boxprops=dict(linewidth=1.1))
    for p in bp["boxes"]:
        p.set_facecolor(color)
        p.set_alpha(0.55)


with open(DATA, newline="") as f:
    rows = [r for r in csv.DictReader(f)
            if r["input_class"] == "refrate" and r["opt_level_numeric"] in ("1", "2", "3", "4")
            and r["has_fastmath"] == "0" and r["has_lto"] == "0"]
data = variance(rows)
if not data:
    raise SystemExit("no rows after filtering data/cpu2026_fprate.csv")

fig, ax = plt.subplots(figsize=(9.0, 5.6))
pos = np.arange(len(data))
box(ax, [r["reads"] for r in data], pos - 0.20, BLUE)
box(ax, [r["writes"] for r in data], pos + 0.20, ORANGE)
ax.axhline(0.0, color="black", linestyle=":", linewidth=1.2, alpha=0.85)
ax.set_title("SPEC CPU2026 fprate", fontsize=16)
ax.set_xticks(pos)
ax.set_xticklabels([r["wl"] for r in data], rotation=45, ha="right", fontsize=14)
ax.tick_params(axis="y", labelsize=13)
ax.set_ylim(-LIMIT - 5, LIMIT + 5)
ax.set_ylabel("Compiler shift (\\%)\n" + r"$(v - v_{\mathrm{O2}}) / v_{\mathrm{O2}}$", fontsize=15)
ax.grid(axis="y", linestyle=":", alpha=0.4)
ax.set_axisbelow(True)
ax.legend(handles=[Patch(facecolor=BLUE, alpha=0.55, label="Reads"),
                   Patch(facecolor=ORANGE, alpha=0.55, label="Writes")],
          loc="upper right", fontsize=14)
fig.tight_layout()
OUT.parent.mkdir(exist_ok=True)
fig.savefig(OUT, bbox_inches="tight")
print(f"wrote {OUT}")
