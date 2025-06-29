#!/usr/bin/env python3

import sys
import os
import getopt
import csv
import sniper_lib
import sniper_stats

def usage():
    print('Usage:', sys.argv[0], '[-h (help)] [-d <resultsdir (default: .)>] [-l <level>]')
    print('Levels: l1, l2, l3, dram')
    sys.exit()

jobid = 0
resultsdir = '.'
level = None
csv_file = 'memsys_stats.out'  # Output CSV file

try:
    opts, args = getopt.getopt(sys.argv[1:], "hj:d:l:", ["help"])
except getopt.GetoptError as e:
    print(e)
    usage()

for o, a in opts:
    if o in ("-h", "--help"):
        usage()
    elif o == "-d":
        resultsdir = a
    elif o == "-j":
        jobid = int(a)
    elif o == "-l":
        level = a.lower()

if args:
    usage()

# ------------------------------
# Level-based filtering logic
# ------------------------------
level_prefix_map = {
    "l1": ["L1-I.", "L1-D."],
    "l2": ["L2."],
    "l3": ["L3."],
    "dram": ["dram."]
}

# All possible stats
all_memory_stats = [
    "L1-I.loads", "L1-I.stores", "L1-I.read-latency", "L1-I.write-latency", "L1-I.workingset-size",
    "L1-D.loads", "L1-D.stores", "L1-D.read-latency", "L1-D.write-latency", "L1-D.workingset-size",
    "L2.loads", "L2.stores", "L2.read-latency", "L2.write-latency", "L2.workingset-size",
    "L3.loads", "L3.stores", "L3.read-latency", "L3.write-latency", "L3.workingset-size",
    "dram.read-latency", "dram.reads", "dram.write-latency", "dram.writes", "dram.workingset-size"
]

if level is None:
    memory_stats = all_memory_stats
elif level not in level_prefix_map:
    print(f"Invalid level: {level}")
    usage()
else:
    memory_stats = [s for s in all_memory_stats if any(s.startswith(prefix) for prefix in level_prefix_map[level])]

def print_result(key, value):
    """Prints and stores results in a dictionary without brackets."""
    if isinstance(value, dict):
        for _key, _value in sorted(value.items()):
            print_result(f"{key}.{_key}", _value)
    else:
        if isinstance(value, list) and len(value) == 1:
            value = value[0]
        print(f"{key} = {value}")
        results_dict[key] = value

print(f"Extracting Memory Stats for level: {level or 'ALL'}")

results = sniper_lib.get_results(jobid, resultsdir)
results_dict = {}

for key, value in sorted(results["results"].items(), key=lambda kv: kv[0].lower()):
    if key in memory_stats:
        print_result(key, value)

print("Extraction Complete")

csv_filepath = os.path.join(resultsdir, csv_file)
with open(csv_filepath, mode="w", newline="") as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(["Statistic", "Value"])
    for key in memory_stats:
        writer.writerow([key, results_dict.get(key, "N/A")])

print(f"Filtered memory statistics successfully written to {csv_filepath}")

