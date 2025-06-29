import ast
from profilers.PatternConfig import PatternConfig

class SniperConfig(PatternConfig):
    @classmethod
    def populating(cls, report_data, level="l2", metadata=None):
        """
        Parse Sniper raw report data and generate PatternConfig instances per core.

        This function reads core-level memory statistics from Sniper's output, focusing
        on a specified memory hierarchy level (L1, L2, L3, or DRAM). For each core, it
        extracts read/write latencies and access counts, computes throughput (in bytes/sec),
        and returns a list of PatternConfig objects.

        Parameters
        ----------
        report_data : dict
            Dictionary of raw metrics from Sniper output, where keys are metric names
            (e.g., "L2.read-latency") and values are lists per core (e.g., "[10, 12, ...]").
        level : str, optional
            Memory level to target for pattern config (default is "l2").
        metadata : object, optional
            Optional metadata to attach to each PatternConfig.

        Returns
        -------
        list of PatternConfig
            One PatternConfig per core, populated with throughput and access metrics.

        Raises
        ------
        ValueError
            If no matching metrics are found for the selected level.
        """
        level_prefix_map = {
            "l1": ["L1-I", "L1-D"],
            "l2": ["L2"],
            "l3": ["L3"],
            "dram": ["dram"]
        }
        prefixes = level_prefix_map.get(level.lower(), [])

        # Determine core count from first valid list-valued metric
        core_count = 0
        example_key = None
        for k, v in report_data.items():
            if any(k.startswith(p) for p in prefixes):
                try:
                    if isinstance(v, list):
                        core_count = len(v)
                        example_key = k
                        break
                    parsed = ast.literal_eval(v)
                    if isinstance(parsed, list):
                        core_count = len(parsed)
                        example_key = k
                        break
                except Exception:
                    continue

        if core_count == 0 or example_key is None:
            raise ValueError(f"No valid list-valued metrics found for level: {level}")

        # Extract per-core execution time in nanoseconds
        core_times_ns = report_data.get("core_time", [])

        if not isinstance(core_times_ns, list):
            raise ValueError("core_time is not a list")

        # Pad or truncate to match core_count
        if len(core_times_ns) < core_count:
            core_times_ns += [core_times_ns[-1]] * (core_count - len(core_times_ns))
        elif len(core_times_ns) > core_count:
            core_times_ns = core_times_ns[:core_count]

        # Safe helper for parsing list-valued metrics
        def get_value(metric, default=0):
            raw = report_data.get(metric)
            if raw is None or raw == "N/A":
                return [default] * core_count
            try:
                if isinstance(raw, list):
                    result = raw
                else:
                    result = ast.literal_eval(raw)
                    if not isinstance(result, list):
                        result = [result]
            except Exception:
                return [default] * core_count

            # Pad or truncate to match core_count
            if len(result) < core_count:
                result += [default] * (core_count - len(result))
            elif len(result) > core_count:
                result = result[:core_count]
            return result

        pattern_configs = []
        read_size = 8  # in bytes
        write_size = 8  # in bytes

        for core_id in range(core_count):
            total_reads = 0
            total_writes = 0
            read_latency = 0
            write_latency = 0
            workingset_size = 0

            for prefix in prefixes:
                read_latency += get_value(f"{prefix}.read-latency")[core_id]
                write_latency += get_value(f"{prefix}.write-latency")[core_id]
                total_reads += get_value(f"{prefix}.loads")[core_id]
                total_writes += get_value(f"{prefix}.stores")[core_id]
                workingset_size += get_value(f"{prefix}.workingset-size")[core_id]

            # Convert per-core time from ns to seconds
            core_time_sec = core_times_ns[core_id] / 1e9 if core_id < len(core_times_ns) else 0.0

            # Convert latency to throughput: accesses/sec
            read_freq = total_reads / core_time_sec if core_time_sec > 0 else 0.0
            write_freq = total_writes / core_time_sec if core_time_sec > 0 else 0.0

            # Unit overrides for this config
            unit_overrides = {
                "read_freq": "count/s",
                "write_freq": "count/s",
                "total_reads": "count",
                "total_write": "count",
                "workingset_size": "count"
            }

            pattern_config = cls(
                exp_name="SniperProfilers",
                benchmark_name=f"core_{core_id}",
                read_freq=read_freq,
                total_reads=total_reads,
                write_freq=write_freq,
                total_writes=total_writes,
                read_size=read_size,
                write_size=write_size,
                workingset_size=workingset_size,
                metadata=metadata,
                unit=unit_overrides  # Set unit override here
            )
            pattern_configs.append(pattern_config)

        return pattern_configs

