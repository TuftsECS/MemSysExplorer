# maap_reproduce

Regenerate the two SPEC CPU2026 figures from the MAAP paper, and the scripts to
collect their data from scratch.

```
maap_reproduce/
├── run_figures.sh          # regenerate both figures -> figures_out/
├── plots/
│   ├── plot_wss.py         # WSS over time (fotonik3d, DynamoRIO)
│   └── plot_boxplot.py     # CPU2026 fprate compiler-sensitivity boxplot
├── data/                   # just-enough CSVs for the two figures
│   ├── fotonik3d_timeseries.csv
│   ├── fotonik3d_summary.csv
│   └── cpu2026_fprate.csv
├── collect/                # collect the data from scratch (CPU2026 + CPU2017)
└── figures_reference/      # the paper's two figures (to diff against)
```

## Regenerate the figures

Requires `python3` with `pandas`, `numpy`, `matplotlib`.

```sh
./run_figures.sh
```

| Figure | Script | Data |
|---|---|---|
| WSS over time (fotonik3d) | `plots/plot_wss.py` | `data/fotonik3d_timeseries.csv`, `_summary.csv` |
| CPU2026 fprate boxplot | `plots/plot_boxplot.py` | `data/cpu2026_fprate.csv` |

Both reproduce the `figures_reference/` copies.

- **WSS over time** — per-window reads, writes, working-set size, and accumulated
  WSS for fotonik3d (each window = 10 M references), resolving a warm-up phase and
  a stable ~27 MB working set.
- **CPU2026 boxplot** — per-workload read/write shift vs the -O2 baseline across
  the -Og/-O1/-O2/-O3 sweep over the 13 fprate workloads; sensitivity runs
  from ~±100% (nest, palm) to near zero (cactus, specrand).

## Collect from scratch

See `collect/README.md` — build the SPEC sweep, run under a chosen profiler
(DynamoRIO / Sniper / perf via `monitor_wrapper.sh`), then `parse_dynamorio.py`
turns the DynamoRIO output into the CSVs in `data/`. It also covers **sweeping
different compiler flags** (the `OPTFLAG` map in `run_sweep.sh`) and **different
configurations** (profiler backend and architectural `--config`).
