#!/usr/bin/env python3
"""WSS over time: per-window reads, writes, WSS, and accumulated WSS for
SPEC CPU2026 fotonik3d (DynamoRIO backend). Output: figures_out/wss_over_time.pdf"""
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data"
OUT = ROOT / "figures_out" / "wss_over_time"
BASELINE = "maap"
LINE_BYTES = 64
C = {"reads": "#4C78A8", "writes": "#F58518", "wss": "#54A24B", "cum": "#E45756"}


def panel(ax, x, y, color, ylabel, alpha=0.20, lw=0.9):
    ax.plot(x, y, color=color, lw=lw)
    ax.fill_between(x, 0, y, color=color, alpha=alpha)
    ax.set_ylabel(ylabel, fontsize=10)


def annotate(ax, text, color, y=0.92):
    ax.text(0.99, y, text, transform=ax.transAxes, ha="right", va="top",
            fontsize=9, color=color,
            bbox=dict(facecolor="white", edgecolor="none", alpha=0.7, pad=1.5))


ts = pd.read_csv(DATA / "fotonik3d_timeseries.csv")
ts = ts[ts["compiler_tag"] == BASELINE].sort_values("window_idx").reset_index(drop=True)
summ = pd.read_csv(DATA / "fotonik3d_summary.csv")
summ = summ[summ["compiler_tag"] == BASELINE].iloc[0]

x = ts["window_idx"].values
wss_win = ts["wss_lines_window"] * LINE_BYTES / 1024 / 1024
wss_cum = ts["wss_lines_cumulative"] * LINE_BYTES / 1024 / 1024
peak = wss_cum.max()

fig, (ax_r, ax_w, ax_ws, ax_wc) = plt.subplots(
    4, 1, figsize=(3.5, 5.0), sharex=True, gridspec_kw=dict(hspace=0.25))

panel(ax_r, x, ts["n_reads"] / 1e6, C["reads"], "per-window\nreads (M)")
annotate(ax_r, f"$\\Sigma = {summ['total_reads'] / 1e6:.0f}$ M", C["reads"])
panel(ax_w, x, ts["n_writes"] / 1e6, C["writes"], "per-window\nwrites (M)")
annotate(ax_w, f"$\\Sigma = {summ['total_writes'] / 1e6:.0f}$ M", C["writes"])
panel(ax_ws, x, wss_win, C["wss"], "per-window\nWSS (MB)", alpha=0.25)
panel(ax_wc, x, wss_cum, C["cum"], "accumulated\nWSS (MB)", lw=1.6)
ax_wc.axhline(peak, color=C["cum"], ls=":", lw=1.0, alpha=0.8)
annotate(ax_wc, f"peak = {peak:.0f} MB", C["cum"], y=0.30)
ax_wc.set_xlabel("window index", fontsize=11)

for ax in (ax_r, ax_w, ax_ws, ax_wc):
    ax.tick_params(axis="both", labelsize=9)
    ax.grid(True, alpha=0.3)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.set_ylim(bottom=0)

OUT.parent.mkdir(exist_ok=True)
fig.savefig(f"{OUT}.pdf", bbox_inches="tight")
fig.savefig(f"{OUT}.png", bbox_inches="tight", dpi=150)
print(f"wrote {OUT}.pdf")
