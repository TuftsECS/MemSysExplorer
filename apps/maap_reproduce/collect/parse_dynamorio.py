#!/usr/bin/env python3
"""Stream-parse DynamoRIO memcount memtrace_<pid>.pb files and emit
per-workload summary/timeseries/hotregions CSVs.

Wire format (length-prefixed delimited, see
MemSysExplorer/apps/profilers/common/src/protobuf_writer.c):

  for each event:
      uint32_t  msg_size              (little-endian on x86)
      <msg_size bytes>                MemoryEvent (proto3, packed)

  MemoryEvent fields (all varint, wire type 0):
      1  uint64 timestamp_us
      2  uint32 thread_id
      3  uint64 address
      4  enum   mem_op       (0=READ, 1=WRITE)
      5  enum   hit_miss     (0=HIT,  1=MISS)

  proto3 omits default-value fields; missing field 4 means READ,
  missing field 5 means HIT.

Outputs (default under collect/scripts/out/per_workload/):
  <workload>.summary.csv       8 rows x ~100 cols, scalar metrics +
                               compiler decomp + input meta + sniper
                               config + optional binary metadata
  <workload>.timeseries.csv    8 tags x ~target_windows rows, time-binned
                               reads/writes/hit_rate/per-window WSS/
                               cumulative WSS
  <workload>.hotregions.csv    8 tags x top_k_regions rows, per-1MB
                               region (addr>>20) read/write counts
  ../all_summaries.csv         Concat of all *.summary.csv (104 rows)

All trace stats use:
  - Adaptive fine-bin merging during streaming (bin size doubles when
    bin count exceeds max_fine_bins) so memory stays bounded
  - Per-bin and cumulative HyperLogLog (p=12, ~1.6% std error) for
    unique cache-line counts (WSS)
  - Hand-written protobuf parser, zero external dependencies

Usage:
  python3 extract_drio_pb.py
  python3 extract_drio_pb.py --workers 8 --target-windows 1000 \\
                             --top-regions 500
  python3 extract_drio_pb.py --binary-metadata out/cpu2026_binary_metadata.csv
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import re
import struct
import sys
import time
from concurrent.futures import ProcessPoolExecutor, as_completed
from pathlib import Path

HERE = Path(__file__).resolve().parent
DEFAULT_ROOT    = Path("/path/to/output")
DEFAULT_OUT_DIR = HERE / "out" / "per_workload"
DEFAULT_MASTER  = HERE / "out" / "all_summaries.csv"
DEFAULT_META    = HERE / "out" / "_run_meta.json"

KNOWN_TAGS = {"maap", "maapO1", "maapO2", "maapO3", "maapOfast",
              "maapOg", "maapFAST", "maapLTO"}

_RUN_DIR_RE = re.compile(r"^run_base_refrate_maap(O[1-9]|Ofast|Og|FAST|LTO)?\.(\d{4})$")
_PB_RE      = re.compile(r"^memtrace_\d+\.pb$")

# tag -> (opt_level_numeric, has_fastmath, has_vectorize, has_unroll,
#         has_debug_friendly, has_lto)
COMPILER_DECOMPOSITION = {
    "maap":      (3, 0, 1, 1, 0, 0),
    "maapO1":    (1, 0, 0, 0, 0, 0),
    "maapO2":    (2, 0, 1, 0, 0, 0),
    "maapO3":    (3, 0, 1, 1, 0, 0),
    "maapOfast": (4, 1, 1, 1, 0, 0),
    "maapOg":    (1, 0, 0, 0, 1, 0),
    "maapFAST":  (3, 1, 1, 1, 0, 0),
    "maapLTO":   (3, 0, 1, 1, 0, 1),
}


def decompose(tag: str) -> dict:
    opt, fm, vec, un, dbg, lto = COMPILER_DECOMPOSITION[tag]
    return dict(opt_level_numeric=opt, has_fastmath=fm,
                has_vectorize=vec, has_unroll=un,
                has_debug_friendly=dbg, has_lto=lto)


def parse_run_dir_name(name: str) -> dict | None:
    m = _RUN_DIR_RE.match(name)
    if not m:
        return None
    suffix = m.group(1) or ""
    return dict(compiler_tag="maap" + suffix, iteration=int(m.group(2)))


# ---------------------------------------------------------------------
# HyperLogLog (p=12 default = 4 KB registers, std error ~1.6 %).
# Stable Knuth multiplicative hash applied to the input; PYTHONHASHSEED
# does not affect results across workers.
# ---------------------------------------------------------------------

_HLL_MASK64 = (1 << 64) - 1


def _splitmix64(x: int) -> int:
    """SplitMix64 finalizer. Strong bit diffusion, stable across runs,
    no external dependency. Used by Java, C++ std, and others."""
    x = (x + 0x9E3779B97F4A7C15) & _HLL_MASK64
    x = ((x ^ (x >> 30)) * 0xBF58476D1CE4E5B9) & _HLL_MASK64
    x = ((x ^ (x >> 27)) * 0x94D049BB133111EB) & _HLL_MASK64
    return x ^ (x >> 31)


class HLL:
    __slots__ = ("p", "m", "mask", "registers")

    def __init__(self, p: int = 12):
        self.p = p
        self.m = 1 << p
        self.mask = self.m - 1
        self.registers = bytearray(self.m)

    def add(self, x: int) -> None:
        h = _splitmix64(x)
        idx = h & self.mask
        w = h >> self.p
        if w == 0:
            lz = 64 - self.p + 1
        else:
            # Leading zeros in (64-p)-bit w = (64-p) - bit_length(w);
            # the register stores that count + 1.
            lz = (64 - self.p) - w.bit_length() + 1
        if lz > self.registers[idx]:
            self.registers[idx] = lz

    def merge(self, other: "HLL") -> None:
        """Union: take elementwise max of registers."""
        if other.m != self.m:
            raise ValueError("HLL precision mismatch")
        a = self.registers
        b = other.registers
        for i, bv in enumerate(b):
            if bv > a[i]:
                a[i] = bv

    def cardinality(self) -> int:
        m = self.m
        alpha = 0.7213 / (1 + 1.079 / m) if m >= 128 else 0.673
        Z = sum(2.0 ** -r for r in self.registers)
        if Z == 0:
            return 0
        E = alpha * m * m / Z
        V = sum(1 for r in self.registers if r == 0)
        if E <= 2.5 * m and V > 0:
            # Linear counting small-range correction
            E = m * (math.log(m) - math.log(V))
        return int(round(E))

    def snapshot(self) -> bytes:
        return bytes(self.registers)

    @classmethod
    def from_bytes(cls, buf: bytes, p: int) -> "HLL":
        h = cls.__new__(cls)
        h.p = p
        h.m = 1 << p
        h.mask = h.m - 1
        h.registers = bytearray(buf)
        return h


# ---------------------------------------------------------------------
# Protobuf varint reader and MemoryEvent decoder.
# ---------------------------------------------------------------------

def _read_varint(buf: bytes, pos: int) -> tuple[int, int]:
    result = 0
    shift = 0
    while True:
        b = buf[pos]
        pos += 1
        result |= (b & 0x7f) << shift
        if not (b & 0x80):
            return result, pos
        shift += 7
        if shift > 70:
            return result, pos


def _parse_memory_event(buf: bytes, pos: int, end: int) \
        -> tuple[int, int, int, int, int]:
    """Decode a single MemoryEvent message between buf[pos:end]."""
    timestamp = 0
    thread_id = 0
    address   = 0
    mem_op    = 0   # READ default
    hit_miss  = 0   # HIT default
    while pos < end:
        tag = buf[pos]; pos += 1
        field     = tag >> 3
        wire_type = tag & 0x7
        if wire_type == 0:
            value, pos = _read_varint(buf, pos)
            if   field == 1: timestamp = value
            elif field == 2: thread_id = value
            elif field == 3: address   = value
            elif field == 4: mem_op    = value
            elif field == 5: hit_miss  = value
        elif wire_type == 1:
            pos += 8
        elif wire_type == 2:
            length, pos = _read_varint(buf, pos)
            pos += length
        elif wire_type == 5:
            pos += 4
        else:
            break
    return timestamp, thread_id, address, mem_op, hit_miss


def parse_event_bytes(buf: bytes) -> tuple[int, int, int, int, int]:
    """Decode the outer MemoryTrace wrapper and return the first
    contained MemoryEvent's fields. The writer in
    MemSysExplorer/.../protobuf_writer.c:42 always wraps a single event
    in a MemoryTrace {repeated MemoryEvent events = 1} before writing,
    so the bytes between length-prefixes are a wrapper, not a bare event."""
    pos = 0
    n = len(buf)
    while pos < n:
        tag = buf[pos]; pos += 1
        field     = tag >> 3
        wire_type = tag & 0x7
        if wire_type == 2:
            length, pos = _read_varint(buf, pos)
            if field == 1:
                # `events` sub-message; parse it as MemoryEvent.
                return _parse_memory_event(buf, pos, pos + length)
            pos += length
        elif wire_type == 0:
            _, pos = _read_varint(buf, pos)
        elif wire_type == 1:
            pos += 8
        elif wire_type == 5:
            pos += 4
        else:
            break
    return 0, 0, 0, 0, 0


# ---------------------------------------------------------------------
# Streaming worker: one .pb -> aggregated tally with adaptive bin merging.
# ---------------------------------------------------------------------

def stream_pb(path_str: str,
              fine_bin_us_initial: int = 1000,
              max_fine_bins: int = 10_000,
              region_size_log2: int = 20,
              hll_precision: int = 12) -> dict:
    """Pass-through .pb extractor (one pass, bounded memory).

    Algorithm:
      1. Start with fine bin = fine_bin_us_initial (default 1 ms).
      2. For each event, increment the bin's read/write/hit/miss
         counters, add its cache-line to (cumulative HLL, current
         bin's HLL), and bump its 1 MB region counter.
      3. If bin count exceeds max_fine_bins, halve resolution: merge
         pairs of adjacent bins (sum counters, union HLLs) and double
         the fine bin size.
      4. Return a serialisable dict so the main process can reduce
         across PIDs.
    """
    path  = Path(path_str)
    size  = path.stat().st_size
    t0    = time.time()
    print(f"  start {path}  ({size/1024/1024/1024:.2f} GB)",
          file=sys.stderr, flush=True)

    unpack4 = struct.Struct("<I").unpack
    cline_shift = 6                       # 64-byte cache lines
    region_shift = region_size_log2       # 1 MB regions default
    fine_bin_us = fine_bin_us_initial

    bins: list[dict] = []                 # each: {n_reads, n_writes, n_hits, n_misses, hll}
    cumulative_hll = HLL(hll_precision)
    region_hist: dict[int, list[int]] = {}  # region -> [reads, writes]
    threads: set[int] = set()

    n_events = n_reads = n_writes = n_hits = n_misses = 0
    min_ts = None
    max_ts = None
    first_ts = None

    def new_bin() -> dict:
        return {"n_reads": 0, "n_writes": 0, "n_hits": 0, "n_misses": 0,
                "hll": HLL(hll_precision)}

    def merge_bin_pairs() -> int:
        nonlocal fine_bin_us
        new_bins = []
        for i in range(0, len(bins), 2):
            a = bins[i]
            if i + 1 < len(bins):
                b = bins[i + 1]
                a["n_reads"]  += b["n_reads"]
                a["n_writes"] += b["n_writes"]
                a["n_hits"]   += b["n_hits"]
                a["n_misses"] += b["n_misses"]
                a["hll"].merge(b["hll"])
            new_bins.append(a)
        bins[:] = new_bins
        fine_bin_us *= 2
        return fine_bin_us

    last_report = t0
    with open(path, "rb", buffering=16 * 1024 * 1024) as f:
        while True:
            hdr = f.read(4)
            if len(hdr) < 4:
                break
            (msg_size,) = unpack4(hdr)
            if msg_size == 0 or msg_size > 1024:
                # Bad framing or zero record; assume truncation and stop.
                print(f"  warn {path}: bad msg_size={msg_size} at byte "
                      f"{f.tell()-4}; stopping", file=sys.stderr, flush=True)
                break
            msg = f.read(msg_size)
            if len(msg) < msg_size:
                break

            ts, tid, addr, mop, hm = parse_event_bytes(msg)
            n_events += 1
            if mop == 0:
                n_reads += 1
            else:
                n_writes += 1
            if hm == 0:
                n_hits += 1
            else:
                n_misses += 1
            if tid:
                threads.add(tid)

            if ts:
                if first_ts is None:
                    first_ts = ts
                if min_ts is None or ts < min_ts:
                    min_ts = ts
                if max_ts is None or ts > max_ts:
                    max_ts = ts
                rel_us = ts - first_ts
                bin_idx = rel_us // fine_bin_us
            else:
                bin_idx = 0

            # Grow bins list to bin_idx, merging when too big.
            while bin_idx >= len(bins):
                bins.append(new_bin())
                if len(bins) > max_fine_bins:
                    merge_bin_pairs()
                    if first_ts is not None:
                        bin_idx = (ts - first_ts) // fine_bin_us

            b = bins[bin_idx]
            if mop == 0:
                b["n_reads"] += 1
            else:
                b["n_writes"] += 1
            if hm == 0:
                b["n_hits"] += 1
            else:
                b["n_misses"] += 1

            if addr:
                cline = addr >> cline_shift
                cumulative_hll.add(cline)
                b["hll"].add(cline)
                region = addr >> region_shift
                rh = region_hist.get(region)
                if rh is None:
                    region_hist[region] = [1, 0] if mop == 0 else [0, 1]
                else:
                    if mop == 0:
                        rh[0] += 1
                    else:
                        rh[1] += 1

            if n_events & 0x3FFFFFF == 0:   # every ~67M events
                now = time.time()
                if now - last_report >= 5.0:
                    pos = f.tell()
                    pct = 100.0 * pos / size if size else 0.0
                    rate = pos / (now - t0) if now > t0 else 0.0
                    eta  = (size - pos) / rate if rate > 0 else 0.0
                    print(f"  prog {path}: {pct:5.1f}%  "
                          f"{rate/1024/1024:.0f} MB/s  events={n_events:,}  "
                          f"bins={len(bins)} bin={fine_bin_us}us  "
                          f"eta={eta:.0f}s", file=sys.stderr, flush=True)
                    last_report = now

    elapsed = time.time() - t0
    print(f"  done  {path}  events={n_events:,} reads={n_reads:,} "
          f"writes={n_writes:,} bins={len(bins)} bin={fine_bin_us}us "
          f"WSS~{cumulative_hll.cardinality():,} lines  "
          f"{elapsed:.1f}s ({(size/1024/1024)/elapsed if elapsed else 0:.0f} MB/s)",
          file=sys.stderr, flush=True)

    # Serialise bins to a transportable form (no HLL objects across pickle).
    serial_bins = [{"n_reads": b["n_reads"], "n_writes": b["n_writes"],
                    "n_hits":  b["n_hits"],  "n_misses": b["n_misses"],
                    "hll_bytes": b["hll"].snapshot()} for b in bins]

    return dict(
        pb_path=str(path),
        pb_size_bytes=size,
        n_events=n_events,
        total_reads=n_reads,
        total_writes=n_writes,
        total_hits=n_hits,
        total_misses=n_misses,
        n_threads=len(threads),
        first_timestamp_us=first_ts,
        min_timestamp_us=min_ts,
        max_timestamp_us=max_ts,
        fine_bin_us=fine_bin_us,
        bins=serial_bins,
        cumulative_hll_bytes=cumulative_hll.snapshot(),
        hll_precision=hll_precision,
        region_hist=region_hist,          # dict[int region] -> [reads, writes]
        region_size_log2=region_size_log2,
        elapsed_parse_s=elapsed,
    )


# ---------------------------------------------------------------------
# Optional C-helper bridge. pb_helper.c streams the .pb in compiled C
# (~50-100x faster than CPython) and writes a binary blob that we
# parse back into the same dict shape as stream_pb() above.
# ---------------------------------------------------------------------

import os
import subprocess


def stream_pb_via_c(path_str: str,
                    helper_path: str,
                    fine_bin_us_initial: int = 1000,
                    max_fine_bins: int = 10_000,
                    region_size_log2: int = 20,
                    hll_precision: int = 12) -> dict:
    """Invoke pb_helper to extract one .pb, then reconstruct the same
    dict shape as stream_pb(). Output binary lands next to the .pb."""
    pb_path  = Path(path_str)
    out_path = Path(path_str + ".stats.bin")

    cmd = [helper_path, path_str, str(out_path),
           "--hll-p",            str(hll_precision),
           "--max-bins",         str(max_fine_bins),
           "--fine-bin-us",      str(fine_bin_us_initial),
           "--region-size-log2", str(region_size_log2)]
    t0 = time.time()
    print(f"  start {path_str}  ({pb_path.stat().st_size/1024/1024/1024:.2f} GB; C)",
          file=sys.stderr, flush=True)
    try:
        proc = subprocess.run(cmd, check=False,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
    except OSError as e:
        raise RuntimeError(f"pb_helper exec failed for {path_str}: {e}")
    if proc.returncode != 0:
        raise RuntimeError(f"pb_helper failed for {path_str}: "
                           f"{proc.stderr.decode(errors='replace')[:500]}")
    # Forward helper's own log line(s) to our stderr stream.
    for line in proc.stderr.splitlines():
        if line.strip():
            print(line.decode(errors="replace"), file=sys.stderr, flush=True)

    if not out_path.is_file():
        raise RuntimeError(f"pb_helper produced no output for {path_str}")

    with open(out_path, "rb") as f:
        data = f.read()
    try:
        out_path.unlink()
    except OSError:
        pass

    if data[:8] != b"DRIOPB01":
        raise RuntimeError(f"bad magic in {out_path}: {data[:8]!r}")

    pos = 8
    (hll_p, fine_bin_us, n_bins, n_regions, region_size_log2_out,
     n_threads, _r1, _r2) = struct.unpack_from("<8I", data, pos)
    pos += 32

    (n_events, n_reads, n_writes, n_hits, n_misses,
     first_ts, min_ts, max_ts, pb_size) = struct.unpack_from("<9Q", data, pos)
    pos += 72

    (elapsed,) = struct.unpack_from("<d", data, pos)
    pos += 8

    hll_m = 1 << hll_p
    cumulative_hll_bytes = data[pos:pos + hll_m]
    pos += hll_m

    bins = []
    for _ in range(n_bins):
        nr, nw, nh, nms = struct.unpack_from("<4Q", data, pos); pos += 32
        hll_bytes = data[pos:pos + hll_m]; pos += hll_m
        bins.append({"n_reads": nr, "n_writes": nw,
                     "n_hits":  nh, "n_misses": nms,
                     "hll_bytes": hll_bytes})

    region_hist: dict[int, list[int]] = {}
    for _ in range(n_regions):
        region, nr, nw = struct.unpack_from("<3Q", data, pos); pos += 24
        region_hist[region] = [nr, nw]

    wall = time.time() - t0
    print(f"  done  {path_str}  events={n_events:,} reads={n_reads:,} "
          f"writes={n_writes:,} bins={n_bins} bin={fine_bin_us}us  "
          f"helper={elapsed:.1f}s wall={wall:.1f}s",
          file=sys.stderr, flush=True)

    return dict(
        pb_path=path_str,
        pb_size_bytes=pb_size,
        n_events=n_events,
        total_reads=n_reads,
        total_writes=n_writes,
        total_hits=n_hits,
        total_misses=n_misses,
        n_threads=n_threads,
        first_timestamp_us=first_ts if first_ts else None,
        min_timestamp_us=min_ts   if min_ts   else None,
        max_timestamp_us=max_ts   if max_ts   else None,
        fine_bin_us=fine_bin_us,
        bins=bins,
        cumulative_hll_bytes=cumulative_hll_bytes,
        hll_precision=hll_p,
        region_hist=region_hist,
        region_size_log2=region_size_log2_out,
        elapsed_parse_s=elapsed,
    )


def pb_worker(path_str: str,
              fine_bin_us_initial: int,
              max_fine_bins: int,
              region_size_log2: int,
              hll_precision: int,
              c_helper: str | None) -> dict:
    """Dispatch one .pb to either the C helper or the pure-Python parser."""
    if c_helper:
        return stream_pb_via_c(path_str, c_helper,
                               fine_bin_us_initial=fine_bin_us_initial,
                               max_fine_bins=max_fine_bins,
                               region_size_log2=region_size_log2,
                               hll_precision=hll_precision)
    return stream_pb(path_str,
                     fine_bin_us_initial=fine_bin_us_initial,
                     max_fine_bins=max_fine_bins,
                     region_size_log2=region_size_log2,
                     hll_precision=hll_precision)


# ---------------------------------------------------------------------
# sim.cfg / sim.info / speccmds.cmd parsers (text, optional, cheap).
# ---------------------------------------------------------------------

def parse_sim_cfg(path: Path) -> dict:
    out = {"l1_size": None, "l2_size": None, "l3_size": None,
           "l3_associativity": None, "l3_replacement": None,
           "branch_predictor": None, "prefetcher_type": None,
           "dram_type": None, "core_frequency_ghz": None}
    if not path.is_file():
        return out
    try:
        text = path.read_text()
    except OSError:
        return out

    def section(name: str) -> str:
        m = re.search(rf"^\[{re.escape(name)}\](.*?)(?=^\[|\Z)", text,
                      flags=re.MULTILINE | re.DOTALL)
        return m.group(1) if m else ""

    def kv(section_name: str, key: str) -> str | None:
        sec = section(section_name)
        m = re.search(rf"^\s*{re.escape(key)}\s*=\s*\"?([^\"\n]+)\"?",
                      sec, flags=re.MULTILINE)
        return m.group(1).strip() if m else None

    out["l1_size"]          = kv("perf_model/l1_dcache", "cache_size")
    out["l2_size"]          = kv("perf_model/l2_cache",  "cache_size")
    out["l3_size"]          = kv("perf_model/l3_cache",  "cache_size")
    out["l3_associativity"] = kv("perf_model/l3_cache",  "associativity")
    out["l3_replacement"]   = kv("perf_model/l3_cache",  "replacement_policy")
    out["branch_predictor"] = kv("perf_model/branch_predictor", "type")
    out["prefetcher_type"]  = (kv("perf_model/dram",  "prefetcher")
                               or kv("perf_model/cache", "prefetcher"))
    out["dram_type"]        = (kv("perf_model/dram",  "type")
                               or kv("perf_model/dram", "model"))
    freq = (kv("perf_model/core", "frequency")
            or kv("perf_model/core/static", "frequency"))
    if freq:
        try:
            out["core_frequency_ghz"] = float(freq)
        except ValueError:
            pass
    return out


def parse_sim_info(path: Path) -> dict:
    out = {"host": None, "simulator_rev": None, "core_model": None}
    if not path.is_file():
        return out
    try:
        text = path.read_text()
    except OSError:
        return out
    m = re.search(r"'host':\s*'([^']+)'", text)
    if m: out["host"] = m.group(1)
    m = re.search(r"'git_revision':\s*'([^']+)'", text)
    if m: out["simulator_rev"] = m.group(1)[:12]
    m = re.search(r"'(?:--config=)?[^']*?(\w+)\.cfg'", text)
    if m: out["core_model"] = m.group(1)
    return out


def parse_speccmds(path: Path) -> dict:
    """Read speccmds.cmd; capture input class (refrate/test/train), the
    command line, and the input file sizes inferred from the command."""
    out = {"input_class": None, "input_command": None,
           "input_args": None, "input_files": None,
           "input_total_bytes": None}
    if not path.is_file():
        return out
    try:
        text = path.read_text(errors="replace")
    except OSError:
        return out

    # SPEC commands look like:
    #   -i input.file -o stdout -e stderr -- /path/to/exec arg1 arg2
    # We collect the trailing tokens after `--`.
    last_cmd = None
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        m = re.search(r"--\s+(.*)$", line)
        if m:
            last_cmd = m.group(1).strip()
    if last_cmd:
        toks = last_cmd.split()
        out["input_command"] = last_cmd
        if len(toks) > 1:
            out["input_args"] = " ".join(toks[1:])

    # Pull input filenames from the parent run dir's files (everything
    # that is not a SPEC infra file and not produced by Sniper/DRIO).
    run_dir = path.parent
    blocked_suffix = (".out", ".err", ".cmd", ".log", ".pb", ".sqlite3",
                      ".cfg", ".info", ".py", ".json")
    blocked_prefix = ("sim.", "memsys", "memtrace_", "L3_trace_",
                      "speccmds.", "compare.", "monitor.")
    inputs = []
    total = 0
    for p in run_dir.iterdir():
        if not p.is_file():
            continue
        n = p.name
        if any(n.startswith(b) for b in blocked_prefix):
            continue
        if any(n.endswith(b) for b in blocked_suffix):
            continue
        if "_base." in n:  # the SPEC binary itself
            continue
        try:
            total += p.stat().st_size
        except OSError:
            continue
        inputs.append(n)
    if inputs:
        out["input_files"] = "|".join(sorted(inputs)[:10])
        out["input_total_bytes"] = total
    # input class from the dir name (refrate/test/train).
    m = re.search(r"run_(base|peak)_(\w+?)_maap", run_dir.name)
    if m:
        out["input_class"] = m.group(2)
    return out


# ---------------------------------------------------------------------
# Per-cell aggregation: combine multi-PID .pb results into one cell.
# ---------------------------------------------------------------------

def aggregate_cell(pb_results: list[dict],
                   target_windows: int,
                   top_k_regions: int,
                   hll_precision: int) -> dict:
    """Reduce per-PID stream_pb outputs into one cell's metrics.

    All HLL unions are computed in the parent process here; bins are
    aligned by re-binning to a common coarse grid of `target_windows`
    spanning the cell's earliest first-event to latest last-event."""
    if not pb_results:
        return {}

    # Scalar sums.
    n_events   = sum(r["n_events"]      for r in pb_results)
    n_reads    = sum(r["total_reads"]   for r in pb_results)
    n_writes   = sum(r["total_writes"]  for r in pb_results)
    n_hits     = sum(r["total_hits"]    for r in pb_results)
    n_misses   = sum(r["total_misses"]  for r in pb_results)
    n_threads  = sum(r["n_threads"]     for r in pb_results)
    n_pb_files = len(pb_results)
    pb_bytes   = sum(r["pb_size_bytes"] for r in pb_results)
    parse_s    = sum(r["elapsed_parse_s"] for r in pb_results)

    # Cumulative HLL: union across PIDs.
    cum_hll = HLL(hll_precision)
    for r in pb_results:
        cum_hll.merge(HLL.from_bytes(r["cumulative_hll_bytes"], hll_precision))
    wss_lines_total = cum_hll.cardinality()

    # Time range across PIDs.
    starts = [r["first_timestamp_us"] for r in pb_results if r["first_timestamp_us"] is not None]
    ends   = [r["max_timestamp_us"]   for r in pb_results if r["max_timestamp_us"]   is not None]
    cell_start = min(starts) if starts else 0
    cell_end   = max(ends)   if ends   else 0
    exec_us    = max(cell_end - cell_start, 0)
    exec_s     = exec_us / 1e6
    read_freq  = (n_reads  / exec_s) if exec_s > 0 else 0.0
    write_freq = (n_writes / exec_s) if exec_s > 0 else 0.0
    hit_rate   = (n_hits   / (n_hits + n_misses)) if (n_hits + n_misses) > 0 else 0.0

    # ----- Re-bin per-PID timeseries to a common coarse grid ---------
    # target coarse bin = exec / target_windows (at least 1 us)
    coarse_bin_us = max(1, exec_us // target_windows) if exec_us > 0 else 1
    n_coarse = (exec_us // coarse_bin_us + 1) if exec_us > 0 else 1
    n_coarse = max(min(int(n_coarse), target_windows * 2), 1)

    coarse_bins = [{"n_reads": 0, "n_writes": 0,
                    "n_hits":  0, "n_misses": 0,
                    "hll": HLL(hll_precision)} for _ in range(n_coarse)]

    for r in pb_results:
        first = r["first_timestamp_us"] or 0
        fine_us = r["fine_bin_us"]
        for i, b in enumerate(r["bins"]):
            # Bin midpoint timestamp (absolute, microseconds since unix-ish).
            bin_mid_us = first + i * fine_us + fine_us // 2
            rel_us = bin_mid_us - cell_start
            ci = int(rel_us // coarse_bin_us)
            if ci < 0 or ci >= n_coarse:
                continue
            cb = coarse_bins[ci]
            cb["n_reads"]  += b["n_reads"]
            cb["n_writes"] += b["n_writes"]
            cb["n_hits"]   += b["n_hits"]
            cb["n_misses"] += b["n_misses"]
            cb["hll"].merge(HLL.from_bytes(b["hll_bytes"], hll_precision))

    # Emit final timeseries rows with cumulative WSS computed via
    # incremental union of coarse-bin HLLs.
    timeseries_rows = []
    running_hll = HLL(hll_precision)
    for ci, cb in enumerate(coarse_bins):
        ts_start_us = cell_start + ci * coarse_bin_us
        ts_end_us   = ts_start_us + coarse_bin_us
        running_hll.merge(cb["hll"])
        accesses = cb["n_hits"] + cb["n_misses"]
        timeseries_rows.append(dict(
            window_idx=ci,
            ts_start_us=ts_start_us,
            ts_end_us=ts_end_us,
            window_bin_us=coarse_bin_us,
            n_reads=cb["n_reads"],
            n_writes=cb["n_writes"],
            n_hits=cb["n_hits"],
            n_misses=cb["n_misses"],
            hit_rate=(cb["n_hits"] / accesses) if accesses > 0 else 0.0,
            wss_lines_window=cb["hll"].cardinality(),
            wss_lines_cumulative=running_hll.cardinality(),
        ))

    # ----- Region histogram: sum across PIDs, keep top-K -------------
    merged_region: dict[int, list[int]] = {}
    for r in pb_results:
        for region, (rd, wr) in r["region_hist"].items():
            slot = merged_region.get(region)
            if slot is None:
                merged_region[region] = [rd, wr]
            else:
                slot[0] += rd
                slot[1] += wr
    region_size_bytes = 1 << pb_results[0]["region_size_log2"]
    region_rows_all = [
        dict(region_addr=region,
             region_addr_hex=hex(region << pb_results[0]["region_size_log2"]),
             region_size_bytes=region_size_bytes,
             n_reads=rd, n_writes=wr, n_total=rd + wr)
        for region, (rd, wr) in merged_region.items()
    ]
    region_rows_all.sort(key=lambda x: -x["n_total"])
    region_rows = region_rows_all[:top_k_regions]

    summary = dict(
        n_pb_files=n_pb_files,
        pb_total_bytes=pb_bytes,
        n_events=n_events,
        total_reads=n_reads,
        total_writes=n_writes,
        total_hits=n_hits,
        total_misses=n_misses,
        n_threads=n_threads,
        cell_start_us=cell_start,
        cell_end_us=cell_end,
        execution_time_us=exec_us,
        execution_time_s=exec_s,
        read_freq=read_freq,
        write_freq=write_freq,
        hit_rate=hit_rate,
        wss_lines_total=wss_lines_total,
        wss_bytes_total=wss_lines_total * 64,
        coarse_bin_us=coarse_bin_us,
        n_coarse_bins=n_coarse,
        n_unique_regions_total=len(merged_region),
        elapsed_parse_s=parse_s,
    )

    return dict(summary=summary,
                timeseries=timeseries_rows,
                regions=region_rows)


# ---------------------------------------------------------------------
# Walker + main.
# ---------------------------------------------------------------------

def find_pb_files(cell_dir: Path) -> list[Path]:
    return sorted(p for p in cell_dir.iterdir()
                  if p.is_file() and _PB_RE.match(p.name))


def latest_cell_with_pb(run_root: Path, tag: str) -> tuple[Path, dict] | None:
    candidates = []
    for cfg_dir in run_root.iterdir():
        if not cfg_dir.is_dir():
            continue
        meta = parse_run_dir_name(cfg_dir.name)
        if meta is None or meta["compiler_tag"] != tag:
            continue
        if any(_PB_RE.match(p.name) for p in cfg_dir.iterdir() if p.is_file()):
            candidates.append((meta["iteration"], cfg_dir, meta))
    if not candidates:
        return None
    candidates.sort(key=lambda t: t[0], reverse=True)
    _, cfg_dir, meta = candidates[0]
    return cfg_dir, meta


def enumerate_cells(root: Path, tag_filter: set[str] | None) -> list:
    tags = (tag_filter & KNOWN_TAGS) if tag_filter else KNOWN_TAGS
    cells = []
    for tag in sorted(tags):
        cpu_root = root / tag / "spec" / "benchspec" / "CPU"
        if not cpu_root.is_dir():
            continue
        for wl_dir in sorted(cpu_root.iterdir()):
            if not wl_dir.is_dir() or "." not in wl_dir.name:
                continue
            run_root = wl_dir / "run"
            if not run_root.is_dir():
                continue
            workload = wl_dir.name.split(".", 1)[1].replace("_r", "")
            cell = latest_cell_with_pb(run_root, tag)
            if cell is None:
                continue
            cfg_dir, meta = cell
            cells.append(dict(tag=tag, workload=workload,
                              workload_id=wl_dir.name,
                              cfg_dir=cfg_dir, meta=meta))
    return cells


def write_csv(rows: list[dict], path: Path) -> None:
    if not rows:
        print(f"  (no rows to write for {path})", file=sys.stderr)
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    fields: list[str] = []
    for r in rows:
        for k in r:
            if k not in fields:
                fields.append(k)
    with path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for r in rows:
            w.writerow(r)
    print(f"  wrote {path}  ({len(rows)} rows, {len(fields)} cols)",
          file=sys.stderr)


def load_binary_metadata(path: Path | None) -> dict:
    """Return dict keyed by (workload, compiler_tag) -> {col: value}."""
    if path is None or not path.is_file():
        return {}
    by_key: dict[tuple, dict] = {}
    with path.open() as f:
        reader = csv.DictReader(f)
        for row in reader:
            wl  = row.get("workload")
            tag = row.get("compiler_tag")
            if wl and tag:
                by_key[(wl, tag)] = row
    return by_key


def main() -> None:
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--root", default=str(DEFAULT_ROOT))
    ap.add_argument("--out-dir", default=str(DEFAULT_OUT_DIR),
                    help=f"per-workload CSV dir (default: {DEFAULT_OUT_DIR})")
    ap.add_argument("--master", default=str(DEFAULT_MASTER),
                    help=f"master concat summary CSV (default: {DEFAULT_MASTER})")
    ap.add_argument("--meta", default=str(DEFAULT_META),
                    help=f"run-metadata JSON (default: {DEFAULT_META})")
    ap.add_argument("--tags", nargs="*", default=None,
                    help="restrict to these compiler tags")
    ap.add_argument("--workers", type=int, default=4,
                    help="parallel .pb stream-parsers (default: 4)")
    ap.add_argument("--target-windows", type=int, default=1000,
                    help="approximate per-cell time-series rows (default: 1000)")
    ap.add_argument("--max-fine-bins", type=int, default=10_000,
                    help="bin count cap during streaming (default: 10000)")
    ap.add_argument("--top-regions", type=int, default=1000,
                    help="top-K 1MB regions per cell (default: 1000)")
    ap.add_argument("--region-size-log2", type=int, default=20,
                    help="region granularity in log2 bytes (default: 20 = 1 MB)")
    ap.add_argument("--hll-p", type=int, default=12,
                    help="HyperLogLog precision bits (default: 12 -> 4KB, ~1.6%% err)")
    ap.add_argument("--binary-metadata", default=None,
                    help="optional CSV (cpu2026_binary_metadata.csv) to "
                         "merge into per-workload summary.csv")
    ap.add_argument("--c-helper", default=None,
                    help="path to compiled pb_helper binary. If given, .pb "
                         "files are streamed in C (50-100x faster than the "
                         "pure-Python fallback). Auto-detected at "
                         "<scripts>/pb_helper if it exists.")
    args = ap.parse_args()

    # Auto-detect compiled helper next to this script.
    c_helper = args.c_helper
    if c_helper is None:
        candidate = HERE / "pb_helper"
        if candidate.is_file() and os.access(candidate, os.X_OK):
            c_helper = str(candidate)
    if c_helper:
        if not os.access(c_helper, os.X_OK):
            raise SystemExit(f"--c-helper not executable: {c_helper}")
        print(f"  using C helper: {c_helper}", file=sys.stderr, flush=True)
    else:
        print(f"  using pure-Python parser (slow). Build the C helper with: "
              f"cd {HERE} && make", file=sys.stderr, flush=True)

    root = Path(args.root)
    if not root.is_dir():
        raise SystemExit(f"--root not a directory: {root}")
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    cells = enumerate_cells(root, set(args.tags) if args.tags else None)
    if not cells:
        raise SystemExit("no cells with memtrace_*.pb found under " + str(root))

    # Build the .pb -> cell_index task list.
    pb_tasks = []
    for cidx, c in enumerate(cells):
        for pb in find_pb_files(c["cfg_dir"]):
            pb_tasks.append((cidx, pb))

    total_gb = sum(t[1].stat().st_size for t in pb_tasks) / 1024 / 1024 / 1024
    print(f"[extract_drio_pb] cells={len(cells)} pb_files={len(pb_tasks)} "
          f"total={total_gb:.1f} GB workers={args.workers} "
          f"target_windows={args.target_windows} top_regions={args.top_regions} "
          f"hll_p={args.hll_p}",
          file=sys.stderr, flush=True)

    binary_meta = load_binary_metadata(
        Path(args.binary_metadata) if args.binary_metadata else None)
    if binary_meta:
        print(f"  loaded binary metadata: {len(binary_meta)} (workload, tag) rows",
              file=sys.stderr, flush=True)

    # ----- Stream all .pb files in parallel ---------------------------
    per_cell_pb_results: dict[int, list[dict]] = {i: [] for i in range(len(cells))}
    t_start = time.time()
    done = 0
    if pb_tasks:
        with ProcessPoolExecutor(max_workers=args.workers) as ex:
            futures = {
                ex.submit(pb_worker, str(pb),
                          1000, args.max_fine_bins,
                          args.region_size_log2, args.hll_p,
                          c_helper): cidx
                for cidx, pb in pb_tasks
            }
            for fut in as_completed(futures):
                cidx = futures[fut]
                c = cells[cidx]
                try:
                    result = fut.result()
                except Exception as e:
                    print(f"  FAIL {c['tag']}/{c['workload']}: {e}",
                          file=sys.stderr, flush=True)
                    continue
                per_cell_pb_results[cidx].append(result)
                done += 1
                elapsed = time.time() - t_start
                print(f"[{done:>3}/{len(pb_tasks)}] {c['tag']}/{c['workload']} "
                      f"events={result['n_events']:,} reads={result['total_reads']:,} "
                      f"writes={result['total_writes']:,} "
                      f"total_elapsed={elapsed:.0f}s",
                      file=sys.stderr, flush=True)

    # ----- Aggregate per cell, group by workload ----------------------
    by_workload: dict[str, list[dict]] = {}
    all_summary_rows: list[dict] = []

    for cidx, c in enumerate(cells):
        pb_res = per_cell_pb_results.get(cidx, [])
        if not pb_res:
            print(f"  skip {c['tag']}/{c['workload']}: no successful pb",
                  file=sys.stderr)
            continue
        agg = aggregate_cell(pb_res,
                             target_windows=args.target_windows,
                             top_k_regions=args.top_regions,
                             hll_precision=args.hll_p)

        # Build the wide summary row
        key = dict(
            source="cpu2026",
            workload_id=c["workload_id"],
            workload=c["workload"],
            compiler_tag=c["tag"],
            iteration=c["meta"]["iteration"],
            cell_dir=str(c["cfg_dir"]),
            **decompose(c["tag"]),
        )
        sim_cfg  = parse_sim_cfg(c["cfg_dir"] / "sim.cfg")
        sim_info = parse_sim_info(c["cfg_dir"] / "sim.info")
        inputs   = parse_speccmds(c["cfg_dir"] / "speccmds.cmd")
        bin_meta = binary_meta.get((c["workload"], c["tag"]), {})
        # Drop key duplicates that would already be in `key`.
        bin_meta = {k: v for k, v in bin_meta.items()
                    if k not in ("source", "workload", "workload_id",
                                 "compiler_tag", "iteration", "cell_dir")}

        summary_row = {**key, **inputs, **sim_cfg, **sim_info,
                       **agg["summary"], **bin_meta}
        ts_rows  = [{**key, **r} for r in agg["timeseries"]]
        reg_rows = [{**key, **r} for r in agg["regions"]]

        wl_bucket = by_workload.setdefault(c["workload_id"], {
            "summary": [], "timeseries": [], "regions": []})
        wl_bucket["summary"].append(summary_row)
        wl_bucket["timeseries"].extend(ts_rows)
        wl_bucket["regions"].extend(reg_rows)
        all_summary_rows.append(summary_row)

    # ----- Write per-workload CSVs ------------------------------------
    print(f"\n[extract_drio_pb] writing per-workload CSVs to {out_dir}",
          file=sys.stderr, flush=True)
    for workload_id, bundle in sorted(by_workload.items()):
        # Use the SPEC numeric prefix to keep files sorted naturally.
        stem = workload_id
        write_csv(bundle["summary"],    out_dir / f"{stem}.summary.csv")
        write_csv(bundle["timeseries"], out_dir / f"{stem}.timeseries.csv")
        write_csv(bundle["regions"],    out_dir / f"{stem}.hotregions.csv")

    master_path = Path(args.master)
    write_csv(all_summary_rows, master_path)

    meta_path = Path(args.meta)
    meta_path.parent.mkdir(parents=True, exist_ok=True)
    meta_path.write_text(json.dumps({
        "root": str(root),
        "out_dir": str(out_dir),
        "n_cells": len(cells),
        "n_pb_files": len(pb_tasks),
        "total_gb": total_gb,
        "target_windows": args.target_windows,
        "max_fine_bins": args.max_fine_bins,
        "top_regions": args.top_regions,
        "region_size_log2": args.region_size_log2,
        "hll_p": args.hll_p,
        "binary_metadata": args.binary_metadata,
        "wall_elapsed_s": time.time() - t_start,
    }, indent=2))
    print(f"  wrote {meta_path}", file=sys.stderr)

    print(f"\n[extract_drio_pb] done. "
          f"{len(by_workload)} workloads, {len(all_summary_rows)} cells, "
          f"{time.time() - t_start:.0f}s total wall time",
          file=sys.stderr)


if __name__ == "__main__":
    main()
