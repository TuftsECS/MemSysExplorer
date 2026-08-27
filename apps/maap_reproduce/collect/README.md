# Collection — SPEC CPU2026 / CPU2017 with MemSysExplorer

How the two figures' data is collected from scratch. Both figures come from the
**DynamoRIO** backend; the wrapper can also run Sniper and perf.

```
collect/
├── monitor_wrapper.sh     # profiler switch (DynamoRIO / Sniper / perf)
├── parse_dynamorio.py     # DynamoRIO .pb output -> the figure CSVs
├── cpu2026/{cpu2026.cfg, run_sweep.sh}
└── cpu2017/{cpu2017.cfg, run_sweep.sh}
```

Before running: edit the `monitor_wrapper` line in `cpu2026.cfg` / `cpu2017.cfg`
to the absolute path of `monitor_wrapper.sh`, and set `MEMSYS_HOME` (in the
wrapper) to your MemSysExplorer checkout.

## Flow

1. **Build + run the sweep.** `runcpu` builds each compiler variant and runs the
   benchmarks; for every benchmark invocation it calls `monitor_wrapper.sh`,
   which re-runs the binary under the chosen profiler.

   ```sh
   source /path/to/cpu2026/shrc          # puts runcpu on PATH
   cd cpu2026
   ./run_sweep.sh ref                     # all 8 compiler tags
   ```

   `run_sweep.sh` runs one `runcpu` per compiler tag. The tags map to flags:

   | tag | flag | | tag | flag |
   |---|---|---|---|---|
   | `maap` | -O3 (base) | | `maapOfast` | -Ofast |
   | `maapO1` | -O1 | | `maapOg` | -Og |
   | `maapO2` | -O2 | | `maapFAST` | -O3 -ffast-math |
   | `maapO3` | -O3 | | `maapLTO` | -O3 -flto |

2. **Pick the profiler** with env vars on `monitor_wrapper.sh` (default: DynamoRIO):

   | var | backend | output used for |
   |---|---|---|
   | `RUN_DRIO=1` | DynamoRIO | per-window WSS time series + total reads/writes (**both figures**) |
   | `RUN_SNIPER=1` | Sniper | IPC / cache-miss / DRAM (not used here) |
   | `RUN_PERF=1` | perf | hardware-counter aggregates (not used here) |

   Each calls `MemSysExplorer/apps/main.py -p <backend>`. Point the wrapper at
   your MemSysExplorer with `MEMSYS_HOME=...`.

3. **Parse** the DynamoRIO protobuf output into CSVs:

   ```sh
   python3 parse_dynamorio.py --root $MAAP_OUT --out-dir out --master out/all_summaries.csv
   ```

   This writes per-workload `out/<N>.<wl>_r.{timeseries,summary,hotregions}.csv`
   (per-window reads/writes/WSS) and the master `out/all_summaries.csv` (per-tag
   `total_reads`/`total_writes`). Copy and rename the three the figures use into
   `../data/`:

   ```sh
   cp out/749.fotonik3d_r.timeseries.csv ../data/fotonik3d_timeseries.csv
   cp out/749.fotonik3d_r.summary.csv    ../data/fotonik3d_summary.csv
   cp out/all_summaries.csv              ../data/cpu2026_fprate.csv
   ```

   Then run `../run_figures.sh`. Freshly parsed CSVs carry `cell_dir` and `host`
   columns (run paths and hostname); the plots ignore them, but drop those two
   columns before releasing the data.

## Sweeping different compiler flags

The flag sweep is the `OPTFLAG` map in `cpu2026/run_sweep.sh`. Each key is a tag
(it becomes the SPEC label and the `compiler_tag` column in the output CSVs), and
its value is the `OPTIMIZE` flag injected into the build:

```sh
declare -A OPTFLAG=(
  [maap]="-O3"            [maapOfast]="-Ofast"
  [maapO1]="-O1"         [maapOg]="-Og"
  [maapO2]="-O2"         [maapFAST]="-O3 -ffast-math"
  [maapO3]="-O3"         [maapLTO]="-O3 -flto"
)
```

- **Add a flag:** add a line, e.g. `[maapO2native]="-O2 -march=native"`, then run
  just that tag: `./run_sweep.sh ref maapO2native`.
- **Run a subset:** pass tags as arguments — `./run_sweep.sh ref maapO2 maapO3`.
- **Run everything:** `./run_sweep.sh ref` (all tags in the map).

A new tag also has to be taught to the parser: add it to `KNOWN_TAGS`,
`COMPILER_DECOMPOSITION` (its `opt_level_numeric` / fastmath / lto / debug
fields), and `_RUN_DIR_RE` in `parse_dynamorio.py`. The boxplot then groups it by
`opt_level_numeric` and `plot_wss.py` filters the base tag via its `BASELINE`
constant (`maap`), so keep the `maap*` prefix or update those two spots.

## Sweeping different configurations

Configuration = the profiler and its architectural model, set in
`monitor_wrapper.sh`:

- **Profiler backend** — toggle `RUN_DRIO` / `RUN_SNIPER` / `RUN_PERF`.
- **Architectural config** — the `--config <file>` passed to `main.py`
  (`apps/config/sunnycove.cfg` for Sniper, `memcount_config.txt` for DynamoRIO).
  To sweep a hardware knob (cache size, L3 latency, replacement policy, DRAM
  type), point `--config` at a different config file per run. For example, a
  cache/technology sweep loops over several `apps/config/*.cfg` files and runs
  the same binary under each, tagging every result with that config — the same
  pattern used for the CPU2026 × L3-technology study.

## CPU2017

`cpu2017/{cpu2017.cfg, run_sweep.sh}` collect CPU2017 the same way — same
`monitor_wrapper.sh`, same `OPTFLAG`-style sweep. One caveat: `parse_dynamorio.py`
is tuned for the CPU2026 runs (it matches `refrate` run dirs and stamps
`source=cpu2026`), so adjust its run-dir regex / source label if you parse a
CPU2017 tree.
