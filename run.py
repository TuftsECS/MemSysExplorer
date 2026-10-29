#!/usr/bin/env python

import os
import sys
import tempfile
import argparse
import json
import yaml
import shutil
import pandas as pd
from run_src.utils import *
from run_src.interfaces import *
from run_src.model import evaluate
from run_src.config import Config


def main():

    # parse config file input
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', '-c', type=str, required=True)
    args = parser.parse_args()

    # load the config
    config = Config(args.config)

    success, diagnostics = config.check()
    for diagnostic in diagnostics:
        print(diagnostic)
    if not success:
        sys.exit(1)

    sys_cfg = config.get("system")
    apps_cfg = config.get("apps")
    tech_cfg = config.get("tech")

    # Making Directory For Run Outputs
    config_name = os.path.splitext(os.path.basename(args.config))[0]
    i = 1
    results_base = os.path.join("results", config_name)
    results_dir = f"{results_base}_{i}"
    while os.path.isdir(f"{results_base}_{i}"):
        i += 1
        results_dir = f"{results_base}_{i}"
    os.makedirs(results_dir)
    print(f"\nRun outputs will be saved to: {results_dir}")

    # Keeping track of original dir
    original_dir = os.getcwd()

    # Useful var for processes
    Tech_Dir = os.path.join("tech", "ArrayCharacterization")

    # run apps interface or pull existing run
    if apps_cfg['run'] == "new":
        if apps_cfg['profiler'] == "sniper":
            apps_cfg['executable'] = os.path.join(original_dir, apps_cfg['executable'])
            apps_cfg['config'] = os.path.join(original_dir, apps_cfg['config'])
        if apps_cfg['profiler'] == "perf" or apps_cfg['profiler'] == 'dynamorio':
            apps_cfg['executable'] = os.path.join(original_dir, apps_cfg['executable'])

        print(apps_cfg)

        os.chdir(results_dir)
        os.mkdir("apps_output")
        os.chdir("apps_output")

        if apps_cfg['profiler'] == "dynamorio":
            run_drio(apps_cfg['executable'], original_dir)
        elif apps_cfg['profiler'] == "sniper":
            run_sniper(apps_cfg['level'], apps_cfg['executable'], apps_cfg['config'], original_dir)
        else:
            run_perf(apps_cfg['level'], apps_cfg['arch'] if apps_cfg['arch'] != None else None, apps_cfg['executable'], original_dir)

        os.chdir(original_dir)

        apps_output_dir = os.path.join(results_dir, "apps_output")
        pattern_files = [os.path.join(apps_output_dir, f) for f in os.listdir(apps_output_dir)
                if f.endswith(".json") and "pattern" in f.lower()]

        if not pattern_files:
            raise FileNotFoundError(f"No pattern config JSON found in {apps_output_dir}")

        if len(pattern_files) == 1:
            apps_patternconfig = pattern_files[0]
        else:
            apps_patternconfig = pattern_files

    else:  # using existing run
        apps_patternconfig = apps_cfg['patternconfig_path']
        if os.path.isdir(apps_patternconfig):
            pattern_files = [os.path.join(apps_patternconfig, f) for f in os.listdir(apps_patternconfig)
                             if f.endswith(".json") and "pattern" in f.lower()]
            if not pattern_files:
                raise FileNotFoundError(f"No pattern config JSON found in directory {apps_patternconfig}")
            apps_patternconfig = pattern_files

    if isinstance(apps_patternconfig, list):
        print(f"Multiple pattern config files found: {apps_patternconfig}")
        apps_results = []
        for pattern_file in pattern_files:
            print(f"Parsing pattern config file: {pattern_file}")
            with open(pattern_file, 'r') as f:
                apps_result = json.load(f)
            print(f"Apps output from {pattern_file}:")
            print(apps_result)
            apps_results.append(apps_result)
    else:
        with open(apps_patternconfig, 'r') as f:
            apps_result = json.load(f)
        print("\nApps output:")
        print(apps_result)
        apps_results = [apps_result]
        pattern_files = [apps_patternconfig]

    # Array Characterization
    if tech_cfg['run'] == "new":
        arraychar_cfg = tech_cfg.copy()
        arraychar_cfg.update(sys_cfg)
        os.chdir(results_dir)
        os.mkdir("tech_output")
        os.chdir(original_dir)

        tech_output_dir = os.path.abspath(os.path.join(results_dir, "tech_output")) + "/"

        # Get all tech config files
        config_path = tech_cfg['array_characterization_config']
        if os.path.isdir(config_path):
            tech_config_paths = [
                os.path.join(config_path, f) for f in os.listdir(config_path)
                if f.endswith('.yaml') or f.endswith('.yml')
            ]
        else:
            tech_config_paths = [config_path]

        # Get all capacities
        capacity_list = sys_cfg.get('Capacity', [])
        if not isinstance(capacity_list, list):
            capacity_list = [capacity_list]

        # Get all optimization targets
        opt_target_list = sys_cfg.get('OptimizationTarget', [])
        if not isinstance(opt_target_list, list):
            opt_target_list = [opt_target_list]

        tech_results = []

        for tech_config_path in tech_config_paths:
            with open(tech_config_path, 'r') as f:
                single_tech_cfg = yaml.load(f, Loader=yaml.FullLoader)

            for cap in capacity_list:
                for opt_target in opt_target_list:
                    cap_label = f"{cap['Value']}{cap['Unit']}"

                    arraychar_cfg_copy = single_tech_cfg.copy()
                    arraychar_cfg_copy.update(sys_cfg)
                    arraychar_cfg_copy.update({k: v for k, v in tech_cfg.items()
                                            if k not in ('array_characterization_config', 'run')})
                    arraychar_cfg_copy['Capacity'] = cap
                    arraychar_cfg_copy['OptimizationTarget'] = opt_target
                    arraychar_cfg_copy['OutputDirectory'] = tech_output_dir

                    if 'MemoryCellInputFile' in single_tech_cfg:
                        mem_cell_path = arraychar_cfg_copy['MemoryCellInputFile']
                        if not os.path.isabs(mem_cell_path):
                            arraychar_cfg_copy['MemoryCellInputFile'] = os.path.join(original_dir, mem_cell_path)

                    # Use existing prefix if set, otherwise fall back to tech config filename
                    existing_prefix = arraychar_cfg_copy.get('OutputFilePrefix', '').strip()
                    if existing_prefix:
                        arraychar_cfg_copy['OutputFilePrefix'] = f"{existing_prefix}_{cap_label}_{opt_target}"
                    else:
                        tech_label = os.path.splitext(os.path.basename(tech_config_path))[0]
                        arraychar_cfg_copy['OutputFilePrefix'] = f"{tech_label}_{cap_label}_{opt_target}"

                    print(f"\nRunning array characterization: {os.path.basename(tech_config_path)} @ {cap_label} optimized for {opt_target}")

                    tech_yaml_str = yaml.dump(arraychar_cfg_copy)
                    with tempfile.NamedTemporaryFile(mode='w+', suffix='.yaml', delete=False) as tech_yaml:
                        tech_yaml.write(tech_yaml_str)
                        tech_yaml_path = tech_yaml.name

                    result = run_array_characterization(tech_yaml_path, Tech_Dir)
                    tech_results.append(result)
                    os.remove(tech_yaml_path)

    else:
        result_path = tech_cfg['array_characterization_result_path']
        if os.path.isdir(result_path):
            result_files = [
                os.path.join(result_path, f) for f in os.listdir(result_path)
                if f.endswith('.yaml') or f.endswith('.yml')
            ]
        else:
            result_files = [result_path]

        tech_results = []
        for path in result_files:
            result = parse_array_char_output(path)
            tech_results.append(result)

    print("\nTech Results:")
    print(tech_results)

    # ANALYTICAL MODEL
    os.chdir(results_dir)
    os.mkdir("model_output")
    os.chdir(original_dir)

    csv_filename = f"{config_name}_results.csv"
    csv_filepath = os.path.join(results_dir, "model_output", csv_filename)

    for tech_result in tech_results:
        mem_cell = tech_result.get('mem_cell_type', 'unknown')
        capacity = tech_result.get('capacity', 'unknown')
        print(f"\nEvaluating tech config: {mem_cell} @ {capacity}")

        for i, app_result in enumerate(apps_results):
            apps_result_name = os.path.splitext(os.path.basename(pattern_files[i]))[0]

            if apps_cfg['profiler'] == "sniper":
                if apps_cfg['multithread']:
                    print("\nEvaluating multiple benchmarks from Sniper output...")
                    for j, benchmark in enumerate(app_result):
                        model_result = evaluate(sys_cfg, benchmark, tech_result)
                        print(f"\nModel results for benchmark {j}:")
                        print(model_result)
                        results_to_csv(apps_cfg, sys_cfg, apps_result_name, benchmark, tech_result, model_result, csv_filepath)
                else:
                    apps_result_single = app_result[0]
                    model_result = evaluate(sys_cfg, apps_result_single, tech_result)
                    print("\nModel results:")
                    print(model_result)
                    results_to_csv(apps_cfg, sys_cfg, apps_result_name, apps_result_single, tech_result, model_result, csv_filepath)
            else:
                model_result = evaluate(sys_cfg, app_result, tech_result)
                print("\nModel results:")
                print(model_result)
                results_to_csv(apps_cfg, sys_cfg, apps_result_name, app_result, tech_result, model_result, csv_filepath)

    # Reordering CSV Rows for Readability
    df = pd.read_csv(csv_filepath)
    df['Capacity_Sort'] = df['Capacity'].apply(parse_capacity)
    df = df.sort_values(by=['Benchmark', 'MemCellType', 'Capacity_Sort', 'Optimization Target'])
    df = df.drop(columns=['Capacity_Sort'])
    df.to_csv(csv_filepath, index=False)
    print(f"\n✓ Model results saved to CSV: {csv_filepath}")


if __name__ == '__main__':
    main()
