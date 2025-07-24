# MemSysExplorer: Memory Array Characterization

Memory array characterization in MemSysExplorer is performed using an extended version of the [NVSim](https://github.com/SEAL-UCSB/NVSim) framework for broader embedded memory Design Space Exploration (DSE). It incorporates features and models from [NVSim](https://github.com/SEAL-UCSB/NVSim), [NVMExplorer](https://github.com/lpentecost/NVMExplorer), [DESTINY](https://code.ornl.gov/3d_cache_modeling_tool/destiny), and [NeuroSim](https://github.com/neurosim/DNN_NeuroSim_V1.4), and has been extended (2025) to include emerging transistor technologies and additional embedded DRAM (eDRAM) memory cell architectures (for example, 3 Transistor (3T) eDRAMs with mixed-threshold voltages).


## Features & Extensions

- **Supported Memory Types**
    - **SRAM**: Static random access memory
    - **1T1C eDRAM**: Standard capacitor-based dynamic random access memory
    - **3T eDRAM**: Three transistor eDRAM with per-cell support for mixed device types and threshold voltages (Vt)
    - **333-eDRAM**: 3T eDRAM leveraging monolithic 3D integration of 3 types of transistors including emerging transistor channel materials ([in press](https://62dac.conference-program.com/presentation/?id=RESEARCH2286&sess=sess124))
    - **NVMs**: Support for various NVM technologies, including:
        + **CTT (Charge Trap Transistor)**
        + **FeFET (Ferroelectric Field-Effect Transistor)**: A type of NVM that uses a ferroelectric material to store data.
        + **MLC-CTT (Multi-Level Cell Charge Trap Transistor)**
        + **PCRAM (Phase Change Random Access Memory)**: A type of NVM that uses a phase-change material to store data.
        + **RRAM (Resistive Random Access Memory)**: A type of NVM that uses a resistive material to store data.
        + **SSTRAM (Spin-Transfer Torque Random Access Memory)**: A type of NVM that uses spin-transfer torque to store data.

- **Device Engineering Flexibility for Mixed-VT eDRAM**
    - Assign different transistor types (HP/LP) and process nodes for:
        - Peripherals (e.g., sense amps/decoders)
        - Write path (e.g., low-leakage/alternative node)
        - Read path (e.g., high-speed/alternative node)
    - For example:
        - High performance transistors for peripherals
        - Low leakage transistors for write path
        - High performance transistors for read path
- **Advanced Technology Nodes**
    - Includes advanced finFET nodes and experimental beyond-silicon technologies. Technology models ported and updated from NVSim and NeuroSim. For emerging devices (e.g., IGZO, CNFET), models are calibrated from academic sources:
        - Nodes above 22 nm are calibrated to the Predictive Technology Model ([PTM](https://mec.umn.edu/ptm)) Project as in [Neurosim](https://github.com/neurosim/DNN_NeuroSim_V1.4)
        - The 10 nm node is calibrated to IRDS 2016 as in [Neurosim](https://github.com/neurosim/DNN_NeuroSim_V1.4)
        - Silicon Technology nodes below 10 nm are calibrated to IRDS 2021 as in [Neurosim](https://github.com/neurosim/DNN_NeuroSim_V1.4)
        - Examples of beyond silicon transistor technologies for eDRAM such as indium-gallium-zinc-oxide transistors with gate length 45nm and carbon nanotube field effect transistors with gate length 22nm are calibrated to experimental data as in [333-eDRAM](https://62dac.conference-program.com/presentation/?id=RESEARCH2286&sess=sess124)
- **Bit Cell Configurations**
    - Each configuration references a sample cell calibrated to physical layouts from literature and specifications from industry. For SRAM and eDRAM, the following references were used:
        1. Barth, John, et al. "A 500 MHz random cycle, 1.5 ns latency, SOI embedded DRAM macro featuring a three-transistor micro sense amplifier." IEEE Journal of Solid-State Circuits 43.1 (2008): 86-95.
        2. Barth, John, et al. "A 45 nm SOI embedded DRAM macro for the POWER™ processor 32 MByte on-chip L3 cache." IEEE Journal of Solid-State Circuits 46.1 (2010): 64-75.
        3. Butt, N., et al. "A 0.039 um 2 high performance eDRAM cell based on 32nm High-K/Metal SOI technology." 2010 International Electron Devices Meeting. IEEE, 2010.
        4. Wang, Yih, et al. "Retention time optimization for eDRAM in 22nm tri-gate CMOS technology." 2013 IEEE International Electron Devices Meeting. IEEE, 2013.
        5. Edri, Noa, et al. "Silicon-proven, per-cell retention time distribution model for gain-cell based eDRAMs." IEEE Transactions on Circuits and Systems I: Regular Papers 63.2 (2016): 222-232.
        6. Narinx, Jonathan, et al. "A 24 kb single-well mixed 3T gain-cell eDRAM with body-bias in 28 nm FD-SOI for refresh-free DSP applications." 2019 IEEE Asian Solid-State Circuits Conference (A-SSCC). IEEE, 2019.
        7. Giterman, Robert, et al. "A 1-Mbit fully logic-compatible 3T gain-cell embedded DRAM in 16-nm FinFET." IEEE Solid-State Circuits Letters 3 (2020): 110-113.
        8. Kong, David, et al. "333-eDRAM – 3T Embedded DRAM Leveraging Monolithic 3D Integration of 3 Transistor Types: IGZO, Carbon Nanotube and Silicon FETs." ACM/IEEE Design Automation Conference (DAC), 2025.


## Compilation
**MemSysExplorer** is programmed in GNU C++. It can be compiled on both Unix-like OSes and Microsoft Windows. Under Linux, the tool can be built using `make`. 
```
make
```
Running `make` will automatically set the required compiler flags.


## Running Simulations
Example configuration files are provided in the sample_configs folder. Each configuration in sample_configs references a cell in the sample_cells folder. Each sample cell has been calibrated to published physical memory cell layouts/specifications from academia & industry. Run Simulations using the following command:

```
./nvsim sample_configs/<chosen_config>.cfg
```
## Example for 333-eDRAM
In addition to the configuration existing configuration parameters in NVSIM, MemSysExplorer adds the following additional parameters for 3T-eDRAMs.

```
-MemoryCellInputFile: sample_cells/sample_edram3t_333/sample_eDRAM3T_333eDRAM.cell

-ProcessNode: 7
-DeviceRoadmap: LOP

-ProcessNodeR: 22
-DeviceRoadmapR: CNT

-ProcessNodeW: 45
-DeviceRoadmapW: IGZO

```

Where `ProcessNode` is the process node of the peripherals, `ProcessNodeR` is the process node of the eDRAM cell read path, and `ProcessNodeW` is the process node of the eDRAM cell write path. In the example above, we are using 7 nm peripherals, CNFETs for the read path and IGZO FETs for the write path. The `MemoryCellInputFile` selects the parameters for the memory cell configuration. For example,
```
-MemCellType: eDRAM3T333

-CellArea (F^2): 387.76
-CellAspectRatio: 1.06

-ReadMode: voltage

-AccessType: CMOS
-AccessCMOSWidth (F): 1.64
-AccessCMOSWidthR (F): 3.6

-DRAMCellCapacitance (F): 0.5e-15
-ResetVoltage (V): vdd
-SetVoltage (V): vdd

-MinSenseVoltage (mV): 10
-MaxStorageNodeDrop (V): 0.1

-RetentionTime (us): 501
```

In NVSim, the feature size, is equal to the process node. For example the feature size is 7 nm at the 7 nm node. The feature size for the `CellArea` parameter is equal to the feature size of the peripherals. The read path (`AccessCMOSWidthR`) and write path (`AccessCMOSWidth`) transistor widths are calculated using the feature size of the technology for those transistors. For example, 1.64 $\times$ 45 nm for IGZO and 3.6 $\times$ 22 nm for CNFET. `DRAMCellCapacitance` specifies the capacitance of the storage node used in retention time calculations. `MinSenseVoltage` specifies the minimum voltage required for the sense amplifier to detect the correct voltage level.`MaxStorageNodeDrop` specifies the maximum voltage drop in storage node voltage before data loss. For 3T eDRAMs, it is desirable to have a small storage node voltage drop to maintain read speed, since the read speed is a function of the storage node voltage. 

When retention time is given using the `RetentionTime` flag, MemSysExplorer uses this value directly. If `RetentionTime` is not provided, MemSysExplorer calculated nominal retention time using C $\times$ $\Delta$ V / Ioff, where C is the storage node capacitance, $\Delta$ V is the maximum storage node drop, and Ioff is the subthreshold leakage of the write path transistor. Process variations can be studied using [msxfi](../msxFI). 

For memories leveraging monolithic 3D integration (e.g., 333-eDRAM) where the memory cell array is fabricated directly above the peripherals, the peripherals are first pitch matched to the array and then the area of the memory cells is subtracted from the final subarray area.


## Authors

David Kong, Zihan Zhang and Madi Gudin

## Contributing

Contributions are welcome! Please feel free to submit issues, feature requests, or pull requests.

## References

1. Dong, Xiangyu, et al. "Nvsim: A circuit-level performance, energy, and area model for emerging nonvolatile memory." IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems 31.7 (2012): 994-1007.
2. Poremba, Matt, et al. "Destiny: A tool for modeling emerging 3d nvm and edram caches." 2015 Design, Automation & Test in Europe Conference & Exhibition (DATE). IEEE, 2015.
3. Pentecost, Lillian, et al. "NVMExplorer: A Framework for Cross-Stack Comparisons of Embedded Non-Volatile Memories." 2022 IEEE International Symposium on High-Performance Computer Architecture (HPCA). IEEE, 2022.
4. Lee, Junmo, et al. "Neurosim v1. 4: Extending technology support for digital compute-in-memory toward 1nm node." IEEE Transactions on Circuits and Systems I: Regular Papers (2024).
5. Kong, David, et al. "333-eDRAM – 3T Embedded DRAM Leveraging Monolithic 3D Integration of 3 Transistor Types: IGZO, Carbon Nanotube and Silicon FETs." ACM/IEEE Design Automation Conference (DAC), 2025.
