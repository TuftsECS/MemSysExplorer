#!/bin/bash
# Compiler-flag sweep for CPU2026 fprate, parallelized with GNU parallel.
# Mirrors the CPU2017 maap/maapO1/maapO2/maapO3/maapOfast/maapOg tagging so
# scripts/extract_spec_binary.py picks up the result tree without changes.
#
# Each tag becomes the SPEC label (which names the result dir) and selects
# the OPTIMIZE flag via --define optflag=...
#
# Usage:
#   source /path/to/cpu2026/shrc
#   ./run_sweep.sh [size] [tag ...]
#   size defaults to ref. With no tags, all eight are run.
#
# Examples:
#   ./run_sweep.sh                       # ref, all 8 tags
#   ./run_sweep.sh test                  # test size, all 8 tags
#   ./run_sweep.sh ref maap maapO2         # ref, just two tags
#
# Output layout. Each tag gets its own subtree so concurrent runcpu
# invocations never touch the same file:
#   $MAAP_OUT/<tag>/spec/             SPEC output_root (build/run/result)
#   $MAAP_OUT/<tag>/monitor_logs/     base_execute.sh per-bench logs
#   $MAAP_OUT/<tag>/runcpu.log        per-tag runcpu stdout+stderr
#   $MAAP_OUT/sweep_logs/joblog.tsv   GNU parallel joblog (resume-friendly)
#   $MAAP_OUT/sweep_logs/sweep.log    top-level sweep script log
# Override the root with:
#   MAAP_OUT=$HOME/maap_out ./run_sweep.sh
#
# Stdout/stderr policy: SPEC's runcpu relies on stdout/stderr for benchmark
# output, so nothing in this script reads or rewrites those streams. Each
# runcpu invocation redirects its own stdout+stderr straight into
# $tag_root/runcpu.log inside run_one_tag, and the sweep script itself
# redirects its own progress prints into $SWEEP_LOG_DIR/sweep.log. GNU
# parallel is used purely as a scheduler with a joblog. It is passed
# --ungroup so it does NOT spool worker output to $TMPDIR buffer files,
# which both keeps /tmp clean and avoids the "Cannot append to buffer
# file in /tmp" failure when /tmp is small or full. Workers have already
# self-redirected, so there is nothing for parallel to spool.
#
# Parallelism: MAX_JOBS controls how many tags run concurrently (default 2).
#   MAX_JOBS=4 ./run_sweep.sh
# Sniper is heavy and largely single-threaded, so MAX_JOBS ~= nproc/2 is
# a safe starting point. Also check RAM and disk before going higher.
#
# Resume after crash/kill: rerun the same command; GNU parallel reads
# joblog.tsv and skips tags that already exited 0.

set -euo pipefail

# Take down the whole script process group on Ctrl-C / SIGTERM so all
# backgrounded tags (runcpu / specperl / cc1plus / sniper / base_execute)
# die with us instead of orphaning to init.
trap 'echo "[run_sweep] signal caught, killing process group..."; kill -TERM 0 2>/dev/null; sleep 3; kill -KILL 0 2>/dev/null; exit 130' INT TERM

# Resolve cfg next to this script so it works on any host the folder
# is copied to.
CFG="$(dirname "$(readlink -f "$0")")/cpu2026.cfg"

MAAP_OUT="${MAAP_OUT:-$HOME/maap_out}"
SWEEP_LOG_DIR="$MAAP_OUT/sweep_logs"
# GNU parallel's bookkeeping (joblog spill, semaphore, etc.) lives under
# MAAP_OUT so it stays on a large volume and never fills a small tmpfs.
PARALLEL_TMPDIR="$MAAP_OUT/parallel_tmp"
mkdir -p "$MAAP_OUT" "$SWEEP_LOG_DIR" "$PARALLEL_TMPDIR"

# Detach the script from the controlling terminal's stdout/stderr. Every
# print from here on (including anything GNU parallel writes on its own)
# lands in sweep.log, so the terminal stays untouched and runcpu's own
# stdio is never multiplexed with ours.
exec >>"$SWEEP_LOG_DIR/sweep.log" 2>&1

MAX_JOBS="${MAX_JOBS:-2}"

SIZE="${1:-ref}"
[ $# -gt 0 ] && shift

# tag -> OPTIMIZE flag
# Six -O baseline tags + two single-flag variants around the O3 baseline
# (factorial-with-base, not full cross). FAST and LTO isolate the
# fast-math and link-time-optimization metadata fields the paper names
# explicitly. extract_spec_binary.py regex was extended to accept them.
declare -A OPTFLAG=(
  [maap]="-O3"
  [maapO1]="-O1"
  [maapO2]="-O2"
  [maapO3]="-O3"
  [maapOfast]="-Ofast"
  [maapOg]="-Og"
  [maapFAST]="-O3 -ffast-math"
  [maapLTO]="-O3 -flto"
)

if [ $# -eq 0 ]; then
  TAGS=(maap maapO1 maapO2 maapO3 maapOfast maapOg maapFAST maapLTO)
else
  TAGS=("$@")
fi

for tag in "${TAGS[@]}"; do
  if [ -z "${OPTFLAG[$tag]+x}" ]; then
    echo "[run_sweep] unknown tag '$tag' — known: ${!OPTFLAG[*]}" >&2
    exit 1
  fi
done

if ! command -v parallel >/dev/null 2>&1; then
  echo "[run_sweep] GNU parallel not found. Install with:" >&2
  echo "    sudo apt install parallel    # Debian/Ubuntu" >&2
  echo "    sudo dnf install parallel    # RHEL/Fedora" >&2
  exit 1
fi

# Per-tag worker. parallel spawns this in a subshell, so the function
# and the env vars it reads must be exported below.
run_one_tag() {
  local tag="$1" flag="$2"
  local tag_root="$MAAP_OUT/$tag"
  local log="$tag_root/runcpu.log"
  mkdir -p "$tag_root/spec" "$tag_root/monitor_logs"
  # runcpu owns its own stdout/stderr, but we point them at a file before
  # exec so neither the parent shell nor GNU parallel ever sees the bytes.
  # stdin is closed so nothing inside SPEC can block on a tty read.
  MONITOR_LOG_ROOT="$tag_root/monitor_logs" \
    runcpu --config="$CFG" \
           --define maap_out="$tag_root" \
           --define label="$tag" \
           --define optflag="$flag" \
           --action=run \
           --noreportable \
           --size="$SIZE" \
           --iterations=1 \
           fprate \
           >>"$log" 2>&1 </dev/null
}
export -f run_one_tag
export MAAP_OUT CFG SIZE

echo "[run_sweep] tags     : ${TAGS[*]}"
echo "[run_sweep] size     : $SIZE"
echo "[run_sweep] max jobs : $MAX_JOBS"
echo "[run_sweep] output   : $MAAP_OUT"
echo "[run_sweep] joblog   : $SWEEP_LOG_DIR/joblog.tsv"
echo "[run_sweep] launching..."

{
  for tag in "${TAGS[@]}"; do
    printf '%s\t%s\n' "$tag" "${OPTFLAG[$tag]}"
  done
} | parallel \
      --colsep '\t' \
      -j "$MAX_JOBS" \
      --joblog "$SWEEP_LOG_DIR/joblog.tsv" \
      --tmpdir "$PARALLEL_TMPDIR" \
      --ungroup \
      run_one_tag {1} {2}

echo "[run_sweep] sweep complete."
echo "  joblog   : $SWEEP_LOG_DIR/joblog.tsv"
echo "  per-tag  : $MAAP_OUT/<tag>/runcpu.log"
