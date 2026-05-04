# Configs

This directory contains all configuration files for driving end-to-end pipeline runs. It is organized into subdirectories by use case — each subdirectory contains both the pipeline run YAML and any supporting files (executables, architecture configs, pattern config JSONs, tech result YAMLs, or memory cell files) needed for that run.

```
configs/
├── sniper_run_config_new/          # fresh Sniper + cache run
├── sniper_run_config_existing/     # existing apps and tech results, ready to reuse
├── drio_run_config/                # fresh DynamoRIO + RAM run
└── tech_configs/                   # tech array characterization configs
    ├── sample_configs/             # sample tech YAML configs (e.g. FeFET, PCRAM)
    └── sample_cells/               # sample memory cell input YAML files
```

---

## Provided Configurations

### `sniper_run_config_new/`

A complete setup for running a fresh end-to-end pipeline run using Sniper for cache profiling and a new array characterization. Contains the pipeline run YAML, a Sniper architecture config (`.cfg`), and the target executable. Use this as your starting point for a new Sniper-based run.

### `sniper_run_config_existing/`

A setup that reuses pre-existing results for both the apps and tech stages — no profiling or array characterization is re-run. Contains the pipeline run YAML alongside a pre-generated pattern config JSON (from a previous Sniper run) and a pre-generated NVSim result YAML. This is useful for quickly re-running the analytical model against existing outputs or for testing without needing Sniper or NVSim installed.

### `drio_run_config/`

A complete setup for running a fresh end-to-end pipeline run using DynamoRIO for RAM profiling. Contains the pipeline run YAML and the target executable.

### `tech_configs/`

Stores tech array characterization configs independently of any specific pipeline run, making it easy to pass a single file or an entire directory into the `tech.array_characterization_config` field for a directory sweep.

- **`sample_configs/`** — sample tech YAML configs for different memory technologies and process nodes (e.g. FeFET at 32nm, PCRAM at 130nm). These are the files passed directly to NVSim via the pipeline.
- **`sample_cells/`** — sample memory cell input YAML files referenced by `MemoryCellInputFile` in the tech configs. Each file defines the electrical properties of a specific memory cell technology.

---

## Pipeline Run YAML

A pipeline run YAML has three required sections: `system`, `apps`, and `tech`. All paths are relative to the project root (i.e. the directory where you invoke `python3 run.py`), regardless of where the config file itself lives.

---

### `system`

The `system` section defines the memory design space parameters that are passed into both the tech component (for array characterization) and the analytical model. These values take precedence over any overlapping fields in a tech config.

```yaml
system:
  DesignTarget: cache       # "cache" or "RAM"
  Capacity:
    Value: 128
    Unit: KB
  WordWidth: 128            # bits
  OptimizationTarget: WriteEDP
```

`DesignTarget`, `Capacity`, `WordWidth`, and `OptimizationTarget` are all required.

**Supported `DesignTarget` values:** `cache`, `RAM`

**Supported `OptimizationTarget` values:** `WriteEDP`, `ReadLatency`, `WriteLatency`, `ReadEDP`, `ReadDynamicEnergy`, `WriteDynamicEnergy`

**Sweeping multiple capacities:** `Capacity` can be provided as a list to sweep across multiple values in a single run. NVSim will be invoked once per tech config × capacity combination and all results will land in the same output CSV.

```yaml
system:
  DesignTarget: cache
  Capacity:
    - Value: 8
      Unit: MB
    - Value: 16
      Unit: MB
  WordWidth: 128
  OptimizationTarget: WriteEDP
```

**Sweeping multiple optimization targets:** `OptimizationTarget` can also be provided as a list. NVSim will be invoked once per tech config × capacity × optimization target combination and all results will land in the same output CSV.

```yaml
system:
  DesignTarget: cache
  Capacity:
    Value: 128
    Unit: KB
  WordWidth: 128
  OptimizationTarget:
    - WriteEDP
    - ReadLatency
```

When using `run: existing` for the tech stage, the pipeline validates that each result file's stored optimization target is a member of the list you provide — not an exact match against the full list. A single result file covering one target is valid as long as that target appears somewhere in your `OptimizationTarget` list.

---

### `apps`

The `apps` section controls workload profiling. You can either launch a new profiling run or point the pipeline at output from a previous run.

> **Profiler–target compatibility:** DynamoRIO can only be used when `DesignTarget` is `RAM`. Sniper must be used for `cache` modeling. The pipeline will exit with an error if these are mismatched.

**New profiling run — Sniper (cache):**

```yaml
apps:
  run: new
  profiler: sniper
  config: configs/sniper_run_config_new/skylake.cfg
  level: l2
  multithread: false
  executable: configs/sniper_run_config_new/test_sniper
```

| Field | Required | Description |
|---|---|---|
| `run` | Yes | `"new"` to profile now |
| `profiler` | Yes | `"sniper"` |
| `config` | Yes | Path to Sniper architecture config (`.cfg`) |
| `level` | Yes | Cache level to profile (e.g. `l2`, `l3`) |
| `multithread` | Yes | `true` if the workload is multi-threaded; `false` otherwise |
| `executable` | Yes | Path to the target binary |

**New profiling run — DynamoRIO (RAM):**

```yaml
apps:
  run: new
  profiler: dynamorio
  executable: configs/drio_run_config/hello
```

| Field | Required | Description |
|---|---|---|
| `run` | Yes | `"new"` to profile now |
| `profiler` | Yes | `"dynamorio"` |
| `executable` | Yes | Path to the target binary |

**Reusing a previous profiling run:**

```yaml
apps:
  run: existing
  profiler: sniper
  multithread: false
  patternconfig_path: configs/sniper_run_config_existing/memsyspatternconfig_test_sniper.json
```

| Field | Required | Description |
|---|---|---|
| `run` | Yes | `"existing"` to reuse prior output |
| `profiler` | Yes | Profiler that generated the original output |
| `multithread` | Sniper only | `true` if the original workload was multi-threaded |
| `patternconfig_path` | Yes | Path to a single pattern config JSON or a directory of JSONs |

`patternconfig_path` can be either a single pattern config JSON or a directory. When a directory is provided, the pipeline picks up all JSON files whose names contain `"pattern"` and processes them in sequence — one row per pattern config will appear in the output CSV.

---

### `tech`

The `tech` section controls array characterization via NVSim. As with `apps`, you can run new characterizations or reuse existing results.

**New array characterization run:**

```yaml
tech:
  run: new
  array_characterization_config: configs/tech_configs/sample_configs/sample_FeFET_32nm_tech_config.yaml
```

| Field | Required | Description |
|---|---|---|
| `run` | Yes | `"new"` to run array characterization |
| `array_characterization_config` | Yes | Path to a single tech YAML or a directory of tech YAMLs |

`array_characterization_config` can be a single YAML file or a directory of YAML files. When a directory is given, NVSim is run for every tech config × capacity combination it finds and all results land in the same output CSV. `system` config values take precedence over any overlapping fields in a tech config.

> **Default tech config:** If no `array_characterization_config` is specified, the pipeline will select a default based on your `system` and `apps` inputs. Currently the default is `tech/ArrayCharacterization/sample_configs/sample_FeFET_32nm.yaml` for cache runs — a more robust selection system based on passed-in system and workload parameters is planned.

**Reusing existing NVSim results:**

```yaml
tech:
  run: existing
  array_characterization_result_path: configs/sniper_run_config_existing/sample_FeFET_32nm_result_128KB_WriteEDP.yaml
```

| Field | Required | Description |
|---|---|---|
| `run` | Yes | `"existing"` to reuse prior NVSim results |
| `array_characterization_result_path` | Yes | Path to a single result YAML or a directory of result YAMLs |

When reusing existing results, the pipeline validates that the `Capacity` and `OptimizationTarget` stored in each result file match what your `system` config specifies. If there is a mismatch, the pipeline exits with a clear message identifying the conflicting file.

---

## Tech YAML

A tech YAML defines the array characterization parameters passed to NVSim. The basic structure is:

```yaml
MemoryCellInputFile: configs/tech_configs/sample_cells/sample_FeFET_cell.yaml

ProcessNode: 32
DeviceRoadmap: LOP
CacheAccessMode: Normal
Associativity: 8           # for cache only

OutputDirectory:
OutputFilePrefix: sample_FeFET_32nm_result
EnablePruning: Yes

LocalWire:
  Type: LocalAggressive
  RepeaterType: RepeatedNone
  LowSwing: No

GlobalWire:
  Type: GlobalAggressive
  RepeaterType: RepeatedNone
  UseLowSwing: No

Routing: H-tree
InternalSensing: true
Temperature: 370           # Kelvin
RetentionTime: 40          # microseconds
BufferDesignOptimization: latency
```

> **Labeling outputs:** `OutputFilePrefix` controls the base name used for the NVSim result file generated for this tech config. If you are running multiple tech configs or a capacity sweep and want results to be clearly identifiable, make sure each tech YAML has a distinct `OutputFilePrefix`. The pipeline will automatically append the capacity and optimization target to the prefix (e.g. `sample_FeFET_32nm_result_128KB_WriteEDP`).

Note that `Capacity`, `WordWidth`, and `OptimizationTarget` do not need to be set in the tech YAML — they are passed in automatically from your `system` config at runtime and will override any values present here.

---

## Example Run YAMLs

### Sniper + Cache (new run)

```yaml
system:
  DesignTarget: cache
  Capacity:
    Value: 128
    Unit: KB
  WordWidth: 128
  OptimizationTarget: WriteEDP

apps:
  run: new
  profiler: sniper
  config: configs/sniper_run_config_new/skylake.cfg
  level: l2
  multithread: false
  executable: configs/sniper_run_config_new/test_sniper

tech:
  run: new
  array_characterization_config: configs/tech_configs/sample_configs/sample_FeFET_32nm_tech_config.yaml
```

### Sniper + Cache (existing runs)

```yaml
system:
  DesignTarget: cache
  Capacity:
    Value: 128
    Unit: KB
  WordWidth: 128
  OptimizationTarget: WriteEDP

apps:
  run: existing
  profiler: sniper
  multithread: false
  patternconfig_path: configs/sniper_run_config_existing/memsyspatternconfig_test_sniper.json

tech:
  run: existing
  array_characterization_result_path: configs/sniper_run_config_existing/sample_FeFET_32nm_result_128KB_WriteEDP.yaml
```

---

Send us any issues or suggestions!