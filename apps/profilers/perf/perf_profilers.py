from profilers.FrontendInterface import FrontendInterface
import subprocess
import re
import os

# Cache line size in bytes
CACHE_LINE_SIZE = 64

# Supported architectures (Intel and AMD only)
SUPPORTED_ARCHS = ["intel", "amd"]

# Level dependencies - strict filtering (only requested level)
LEVEL_DEPENDENCIES = {
    "l1": ["l1"],
    "l2": ["l2"],
    "l3": ["l3"],
    "dram": ["dram"],
    "all": None,  # All levels
}

# Formula-first organization: level -> metric -> architecture counters
# Each metric shows the perf event name for each architecture
# None means the counter is not available on that architecture
PERF_FORMULAS = {
    "l1": {
        "l1d_loads": {
            "intel": "L1-dcache-loads:u",
            "amd": "L1-dcache-loads:u",
        },
        "l1d_load_misses": {
            "intel": "L1-dcache-load-misses:u",
            "amd": "L1-dcache-load-misses:u",
        },
        "l1d_stores": {
            "intel": "L1-dcache-stores:u",
            "amd": None,  # Not available on AMD; use l2_request_g1.change_to_x at L2
        },
        "l1i_load_misses": {
            "intel": "L1-icache-load-misses:u",
            "amd": "L1-icache-load-misses:u",
        },
    },
    "l2": {
        "l2_load_hits": {
            "intel": "mem_load_retired.l2_hit:u",
            "amd": "l2_cache_req_stat.ic_dc_hit_in_l2:u",
        },
        "l2_load_misses": {
            "intel": "mem_load_retired.l2_miss:u",
            "amd": "l2_cache_req_stat.ic_dc_miss_in_l2:u",
        },
        "l2_rfo_total": {
            "intel": "l2_rqsts.all_rfo:u",
            # change_to_x = requests for exclusive ownership (store-like)
            "amd": "l2_request_g1.change_to_x:u",
        },
        "l2_rfo_hits": {
            "intel": "l2_rqsts.rfo_hit:u",
            "amd": None,  # AMD doesn't split RFO hit/miss
        },
        "l2_rfo_misses": {
            "intel": "l2_rqsts.rfo_miss:u",
            "amd": None,  # AMD doesn't split RFO hit/miss
        },
        "l2_pf_hit_l3": {
            "intel": None,
            "amd": "l2_pf_miss_l2_hit_l3:u",
        },
        "l2_pf_miss_l3": {
            "intel": None,
            "amd": "l2_pf_miss_l2_l3:u",
        },
    },
    "l3": {
        "l3_hits": {
            "intel": "mem_load_retired.l3_hit:u",
            # AMD: derive from l2_pf_miss_l2_hit_l3 (prefetch that hit L3)
            "amd": "l2_pf_miss_l2_hit_l3:u",
        },
        "l3_misses": {
            "intel": "mem_load_retired.l3_miss:u",
            # AMD: l3_comb_clstr_state.request_miss not available; use l2_pf_miss_l2_l3
            "amd": "l2_pf_miss_l2_l3:u",
        },
        "llc_loads": {
            "intel": "LLC-loads:u",
            # AMD: L2 misses go to L3, so use all_l2_cache_misses
            "amd": None,
        },
        "llc_load_misses": {
            "intel": "LLC-load-misses:u",
            # AMD: L3 misses = fills from memory
            "amd": "l1_data_cache_fills_from_memory:u",
        },
        "llc_stores": {
            "intel": None,  # Not available on Intel
            "amd": None,    # Not directly available on AMD
        },
    },
    "dram": {
        # DRAM Reads (fills from memory)
        "dram_local": {
            "intel": "mem_load_l3_miss_retired.local_dram:u",
            # AMD: fills from local memory
            "amd": "l1_data_cache_fills_from_memory:u",
        },
        "dram_remote": {
            "intel": "mem_load_l3_miss_retired.remote_dram:u",
            # AMD: fills from remote node
            "amd": "l1_data_cache_fills_from_remote_node:u",
        },
        # DRAM Writes
        "dram_write_local": {
            "intel": None,
            "amd": None,  # Not directly measurable
        },
        "dram_write_remote": {
            "intel": None,
            "amd": None,  # Not directly measurable
        },
    },
    "interconnect": {
        "ccx_requests": {
            "intel": None,
            "amd": "xi_ccx_sdp_req1:u",
        },
    },
    "general": {
        "cycles": {
            "intel": "cycles:u",
            "amd": "cycles:u",
        },
        "instructions": {
            "intel": "instructions:u",
            "amd": "instructions:u",
        },
    },
}


class PerfProfilers(FrontendInterface):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.executable_cmd = self.config.get("executable")
        self.action = self.config.get("action")
        self.level = self.config.get("level", "custom")
        self.arch = self.config.get("arch", "intel").lower()
        self.repeat = self.config.get("repeat", 3)

        if self.arch not in SUPPORTED_ARCHS:
            raise ValueError(f"Unsupported arch '{self.arch}'. Must be one of: {SUPPORTED_ARCHS}")

        self.output = ""
        self.report = None
        self.data = {}
        self.active_events = self.get_events_for_arch()

    def get_events_for_arch(self):
        """Get events based on architecture and requested level."""
        events = []
        required_levels = LEVEL_DEPENDENCIES.get(self.level)

        for level, metrics in PERF_FORMULAS.items():
            if required_levels is not None and level not in required_levels:
                continue
            for metric_key, arch_counters in metrics.items():
                event_name = arch_counters.get(self.arch)
                if event_name is not None:
                    events.append((event_name, metric_key))
        return events

    def validate_events(self):
        """Validate that perf events are available on this system.

        Runs a quick perf stat probe with all events and parses stderr
        to identify unsupported events. Returns list of valid events
        and prints warnings for invalid ones.
        """
        if not self.active_events:
            return []

        event_string = ",".join(event for event, _ in self.active_events)

        try:
            result = subprocess.run(
                ["perf", "stat", "-e", event_string] + ["--hybrid-merge"] if self.arch == "intel" else [] + ["--", "true"],
                capture_output=True,
                text=True
            )
        except FileNotFoundError:
            print("Warning: perf not found, skipping event validation")
            return self.active_events

        stderr = result.stderr

        # Find invalid events from stderr
        invalid_events = set()

        for event, metric in self.active_events:
            base_event = re.sub(r':[uk]+$', '', event)
            # Check for common error patterns
            if (f"event '{base_event}'" in stderr and "not found" in stderr) or \
               (f"event '{event}'" in stderr and "not found" in stderr):
                invalid_events.add(event)

        # Also check the output for <not supported> markers
        for line in stderr.split('\n'):
            if '<not supported>' in line or '<not counted>' in line:
                # Extract event name from: "0  event_name  <not supported>"
                for event, metric in self.active_events:
                    base_event = re.sub(r':[uk]+$', '', event)
                    if base_event in line:
                        invalid_events.add(event)

        # Filter out invalid events
        valid_events = [(event, metric) for event, metric in self.active_events
                        if event not in invalid_events]

        # Print warnings for invalid events
        if invalid_events:
            print(f"Warning: {len(invalid_events)} event(s) not supported on this system:")
            for event in sorted(invalid_events):
                metric = next((m for e, m in self.active_events if e == event), "unknown")
                print(f"  - {event} ({metric})")
            print(f"Continuing with {len(valid_events)} valid event(s)")

        return valid_events

    def build_event_string(self):
        """Build comma-separated event string from active events."""
        return ",".join(event for event, _ in self.active_events)

    def construct_command(self):
        """Construct the perf command with target event counters."""
        if isinstance(self.executable_cmd, str):
            executable_with_args = self.executable_cmd.split()
        else:
            executable_with_args = list(self.executable_cmd)

        if hasattr(self.config, "get") and "executable_args" in self.config:
            exec_args = self.config.get("executable_args") or []
            if isinstance(exec_args, str):
                exec_args = [exec_args]
            executable_with_args += exec_args

        report = os.path.basename(executable_with_args[0])
        event_string = self.build_event_string()

        perf_command = [
            "perf", "stat",
            "-r", str(self.repeat),
            "-e", event_string,
            "-o", "/dev/stdout"
        ] + ["--hybrid-merge"] if self.arch == "intel" else [] + executable_with_args

        return perf_command, report

    def profiling(self, **kwargs):
        """Run the target executable under perf stat and save output."""
        # Validate events before profiling
        self.active_events = self.validate_events()
        if not self.active_events:
            raise RuntimeError("No valid perf events available for this system")

        perf_command, report = self.construct_command()
        try:
            print(f"Executing: {' '.join(perf_command)}")
            profiler_data = subprocess.run(
                perf_command,
                check=True,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )
        except subprocess.CalledProcessError as e:
            print(f"Command failed with exit code {e.returncode}")
            print(f"stderr: {e.stderr}")
            raise

        self.output = profiler_data.stdout + profiler_data.stderr

        print("\n--- Raw perf output ---")
        print(self.output)
        print("--- End perf output ---\n")

        if self.action == "profiling":
            self.report = f"{report}.perf-rep"
            with open(self.report, 'w') as perf_report:
                perf_report.write(f"Profiling output:\n {self.output}")
            print(f"Output written to file {report}.perf-rep")

    def extract_metrics(self, report_file=None, **kwargs):
        """Extract raw performance metrics from perf output."""
        toparse = ""
        if self.action == "extract_metrics":
            with open(report_file) as file:
                toparse = file.read()
        if self.action == "both":
            toparse = self.output

        try:
            for event_name, metric_key in self.active_events:
                base_event = re.sub(r':[uk]+$', '', event_name)
                escaped_event = re.escape(base_event)
                pattern = rf"([\d,]+)\s+{escaped_event}"
                match = re.search(pattern, toparse)
                value = int(match.group(1).replace(',', '')) if match else 0
                self.data[metric_key] = value

            time_match = re.search(r"([\d.]+)\s+(?:\+\-\s+[\d.]+\s+)?seconds time elapsed", toparse)
            self.data["time_elapsed"] = float(time_match.group(1)) if time_match else 0.0

            # Store the level for PerfConfig to use
            self.data["level"] = self.level

            # NO compute_derived_metrics() call - let PerfConfig handle it
            return self.data

        except AttributeError as e:
            print(f"Failed to extract data: {e}")
            raise

    def print_summary(self):
        """Print raw counters collected for the requested level."""
        print(f"\n{'='*50}")
        print(f"RAW PERF COUNTERS (level={self.level}, arch={self.arch})")
        print(f"{'='*50}")
        for key, value in self.data.items():
            if key not in ["level", "time_elapsed"]:
                print(f"  {key}: {value:,}")
        print(f"  time_elapsed: {self.data.get('time_elapsed', 0):.6f}s")
        
    @classmethod
    def required_profiling_args(cls):
        """Define required arguments for profiling."""
        return ["executable", "level"]

    @classmethod
    def optional_profiling_args(cls):
        """Define optional arguments for profiling."""
        return [
            {
                "name": "arch",
                "choices": SUPPORTED_ARCHS,
                "default": "intel",
                "help": "CPU architecture: intel or amd"
            },
            {
                "name": "repeat",
                "type": int,
                "default": 3,
                "help": "Number of measurement repeats"
            },
        ]

    @classmethod
    def required_extract_args(cls, action):
        """Define required arguments for metric extraction."""
        if action == "extract_metrics":
            return ["report_file"]
        else:
            return []
