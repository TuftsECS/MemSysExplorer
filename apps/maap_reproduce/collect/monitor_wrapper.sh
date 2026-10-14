#!/bin/bash
# SPEC monitor_wrapper for MemSysExplorer collection.
# runcpu calls this once per benchmark binary (see cpu2026.cfg / cpu2017.cfg:
#   monitor_wrapper = <path>/monitor_wrapper.sh $command).
# It re-runs that binary under the selected MAAP profiler backend(s).
#
# Choose the profiler(s) with these env vars (1 = on, 0 = off):
#   RUN_DRIO=1     DynamoRIO   -> per-window WSS time series + total reads/writes
#   RUN_SNIPER=0   Sniper      -> architectural metrics (IPC, cache miss, DRAM)
#   RUN_PERF=0     perf        -> hardware-counter aggregates
# The two figures in this package come from the DynamoRIO backend, so the
# default is DynamoRIO only. Flip the toggles to collect the others.
#
#   MEMSYS_HOME=DIR   MemSysExplorer root (has apps/main.py + apps/config/)
#   LOG_ROOT=DIR      per-benchmark log dir
set -u

BENCH_CMD="$@"
RUN_DIR="$(pwd)"
BENCH=$(echo "$RUN_DIR" | sed -nE 's#.*/benchspec/CPU/([^/]+)/.*#\1#p'); : "${BENCH:=unknown}"
TAG=$(echo "$RUN_DIR" | sed -nE 's#.*/run_base_[^_]+_([A-Za-z0-9]+)\.[0-9]+$#\1#p'); : "${TAG:=notag}"

MEMSYS_HOME="${MEMSYS_HOME:-/path/to/maap/MemSysExplorer}"
MAIN="${MEMSYS_HOME}/apps/main.py"
LOG_ROOT="${MONITOR_LOG_ROOT:-${LOG_ROOT:-$HOME/maap_logs}}"
mkdir -p "$LOG_ROOT"
LOG="${LOG_ROOT}/${BENCH}_${TAG}.log"

RUN_DRIO="${RUN_DRIO:-1}"
RUN_SNIPER="${RUN_SNIPER:-0}"
RUN_PERF="${RUN_PERF:-0}"

echo "[wrapper] $BENCH tag=$TAG drio=$RUN_DRIO sniper=$RUN_SNIPER perf=$RUN_PERF" >> "$LOG"

if [ "$RUN_DRIO" = "1" ]; then
  python3 "$MAIN" -p dynamorio -a both \
    --config "${MEMSYS_HOME}/apps/config/memcount_config.txt" \
    --executable "$BENCH_CMD" >> "$LOG" 2>&1
fi

if [ "$RUN_SNIPER" = "1" ]; then
  python3 "$MAIN" -p sniper -a both --level l3 --results_dir . \
    --config "${MEMSYS_HOME}/apps/config/sunnycove.cfg" \
    --executable "$BENCH_CMD" >> "$LOG" 2>&1
fi

if [ "$RUN_PERF" = "1" ]; then
  python3 "$MAIN" -p perf -a both \
    --executable "$BENCH_CMD" >> "$LOG" 2>&1
fi
