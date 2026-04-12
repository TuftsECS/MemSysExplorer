Application Data Exploration

The folder contains:
- NVMsum_J.ipynb
    run with -> jupyter notebook
- NVMsum_S.py
    run with -> bokeh serve --show NVMsum_S.py
- MSX_Workload_S.py
    run with -> bokeh serve --show MSX_Workload_S.py
- MSXsum_S.py
    run with -> bokeh serve --show MSXsum_S.py

# NVMsum

They both contain the same graph: jupyter notebook to see the code with the graph, and the standalone python file to only see the graph

The graph presents application-level data from NVMExplorer, run with all technology, all optimization targets, all benchmarks, and all capacities. 

# MSX

## MSXsum

Visualizes data collected from MemSysExplorer's end-to-end pipeline. Each row of data is a specific memory system configuration (e.g., MRAM cache size + design target) evaluated under a workload (e.g., core_0 on Sniper simulator). Looks at data in dataviz/Website/CSV_Files/MSX_Data

## MSX_Workload

Visualizes workload data in dataviz/Website/CSV_Files/Example_Workload_Dataset. Compares how different profilers observe system and memory behavior across CPU workloads and benchmark suites.