/*******************************************************************************
* Copyright (c) 2012-2013, The Microsystems Design Labratory (MDL)
* Department of Computer Science and Engineering, The Pennsylvania State University
* Exascale Computing Lab, Hewlett-Packard Company
* All rights reserved.
* 
* This source code is part of NVSim - An area, timing and power model for both 
* volatile (e.g., SRAM, DRAM) and non-volatile memory (e.g., PCRAM, STT-RAM, ReRAM, 
* SLC NAND Flash). The source code is free and you can redistribute and/or modify it
* by providing that the following conditions are met:
* 
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
* 
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* Author list: 
*   Cong Xu	    ( Email: czx102 at psu dot edu 
*                     Website: http://www.cse.psu.edu/~czx102/ )
*   Xiangyu Dong    ( Email: xydong at cse dot psu dot edu
*                     Website: http://www.cse.psu.edu/~xydong/ )
*******************************************************************************/


#include "InputParameter.hpp"
#include "global.hpp"

#include "yaml-cpp/yaml.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void InputParameter::ReadInputParameterFromFile(const std::string& inputFile) {
    try {
        YAML::Node config = YAML::LoadFile(inputFile);
        
        // Memory Cell Input File
        if (config["MemoryCellInputFile"]) {
            fileMemCell = config["MemoryCellInputFile"].as<std::string>();
        }
        
        // Process Technology
        if (config["ProcessNode"])
            processNode = config["ProcessNode"].as<int>();
        if (config["ProcessNodeW"])
            processNodeW = config["ProcessNodeW"].as<int>();
        else if (config["ProcessNode"])
            processNodeW = processNode;
            
        if (config["ProcessNodeR"])
            processNodeR = config["ProcessNodeR"].as<int>();
        else if (config["ProcessNode"])
            processNodeR = processNode;
        
        // Device Roadmap
        if (config["DeviceRoadmap"]) {
            std::string roadmap = config["DeviceRoadmap"].as<std::string>();
            if (roadmap == "HP")
                deviceRoadmap = HP;
            else if (roadmap == "LOP")
                deviceRoadmap = LOP;
            else if (roadmap == "IGZO")
                deviceRoadmap = IGZO;
            else if (roadmap == "CNT")
                deviceRoadmap = CNT;
            else {
                std::cout << "Invalid DeviceRoadmap (choose HP/LOP/CNT/IGZO)" << std::endl;
                exit(-1);
            }
        }
        
        if (config["DeviceRoadmapW"]) {
            std::string roadmap = config["DeviceRoadmapW"].as<std::string>();
            if (roadmap == "HP")
                deviceRoadmapW = HP;
            else if (roadmap == "LOP")
                deviceRoadmapW = LOP;
            else if (roadmap == "IGZO")
                deviceRoadmapW = IGZO;
            else if (roadmap == "CNT")
                deviceRoadmapW = CNT;
            else {
                std::cout << "Invalid DeviceRoadmapW (choose HP/LOP/CNT/IGZO)" << std::endl;
                exit(-1);
            }
        } else if (config["DeviceRoadmap"]) {
            deviceRoadmapW = deviceRoadmap;
        }
        
        if (config["DeviceRoadmapR"]) {
            std::string roadmap = config["DeviceRoadmapR"].as<std::string>();
            if (roadmap == "HP")
                deviceRoadmapR = HP;
            else if (roadmap == "LOP")
                deviceRoadmapR = LOP;
            else if (roadmap == "IGZO")
                deviceRoadmapR = IGZO;
            else if (roadmap == "CNT")
                deviceRoadmapR = CNT;
            else {
                std::cout << "Invalid DeviceRoadmapR (choose HP/LOP/CNT/IGZO)" << std::endl;
                exit(-1);
            }
        } else if (config["DeviceRoadmap"]) {
            deviceRoadmapR = deviceRoadmap;
        }
        
        // Design Configuration
        if (config["DesignTarget"]) {
            std::string target = config["DesignTarget"].as<std::string>();
            if (target == "cache")
                designTarget = cache;
            else if (target == "RAM") {
                designTarget = RAM_chip;
                minNumRowPerSet = 1;
                maxNumRowPerSet = 1;
            } else {
                designTarget = CAM_chip;
                minNumRowPerSet = 1;
                maxNumRowPerSet = 1;
            }
        }
        
        if (config["CacheAccessMode"]) {
            std::string mode = config["CacheAccessMode"].as<std::string>();
            if (mode == "Sequential")
                cacheAccessMode = sequential_access_mode;
            else if (mode == "Fast")
                cacheAccessMode = fast_access_mode;
            else
                cacheAccessMode = normal_access_mode;
        }
        
        if (config["Associativity"])
            associativity = config["Associativity"].as<int>();
        
        // Optimization
        if (config["OptimizationTarget"]) {
            std::string target = config["OptimizationTarget"].as<std::string>();
            if (target == "ReadLatency")
                optimizationTarget = read_latency_optimized;
            else if (target == "WriteLatency")
                optimizationTarget = write_latency_optimized;
            else if (target == "ReadDynamicEnergy")
                optimizationTarget = read_energy_optimized;
            else if (target == "WriteDynamicEnergy")
                optimizationTarget = write_energy_optimized;
            else if (target == "ReadEDP")
                optimizationTarget = read_edp_optimized;
            else if (target == "WriteEDP")
                optimizationTarget = write_edp_optimized;
            else if (target == "LeakagePower")
                optimizationTarget = leakage_optimized;
            else if (target == "Area")
                optimizationTarget = area_optimized;
            else
                optimizationTarget = full_exploration;
        }
        
        if (config["OutputFilePrefix"])
            outputFilePrefix = config["OutputFilePrefix"].as<std::string>();
        
        if (config["OutputDirectory"])
            outputDirectory = config["OutputDirectory"].as<std::string>();
        
        if (config["EnablePruning"]) {
            std::string enable = config["EnablePruning"].as<std::string>();
            isPruningEnabled = (enable == "Yes" || enable == "yes" || enable == "true");
        }
        
        // Memory Specifications - Support both nested and flat formats
        if (config["Capacity"]) {
            if (config["Capacity"].IsMap()) {
                // Nested format
                long cap = config["Capacity"]["Value"].as<long>();
                std::string unit = config["Capacity"]["Unit"].as<std::string>();
                if (unit == "B")
                    capacity = cap;
                else if (unit == "KB")
                    capacity = cap * 1024;
                else if (unit == "MB")
                    capacity = cap * 1024 * 1024;
            } else {
                // Flat format - assume KB for backwards compatibility
                capacity = config["Capacity"].as<long>() * 1024;
            }
        }
        
        // Also support old-style flat capacity fields
        if (config["Capacity_B"])
            capacity = config["Capacity_B"].as<long>();
        if (config["Capacity_KB"])
            capacity = config["Capacity_KB"].as<long>() * 1024;
        if (config["Capacity_MB"])
            capacity = config["Capacity_MB"].as<long>() * 1024 * 1024;
        
        if (config["WordWidth"])
            wordWidth = config["WordWidth"].as<long>();
        
        // Wire Configuration
        if (config["LocalWire"]) {
            YAML::Node localWire = config["LocalWire"];
            if (localWire["Type"]) {
                std::string type = localWire["Type"].as<std::string>();
                if (type == "LocalAggressive") {
                    minLocalWireType = local_aggressive;
                    maxLocalWireType = local_aggressive;
                } else if (type == "LocalConservative") {
                    minLocalWireType = local_conservative;
                    maxLocalWireType = local_conservative;
                } else if (type == "SemiAggressive") {
                    minLocalWireType = semi_aggressive;
                    maxLocalWireType = semi_aggressive;
                } else if (type == "SemiConservative") {
                    minLocalWireType = semi_conservative;
                    maxLocalWireType = semi_conservative;
                } else if (type == "GlobalAggressive") {
                    minLocalWireType = global_aggressive;
                    maxLocalWireType = global_aggressive;
                } else if (type == "GlobalConservative") {
                    minLocalWireType = global_conservative;
                    maxLocalWireType = global_conservative;
                } else {
                    minLocalWireType = dram_wordline;
                    maxLocalWireType = dram_wordline;
                }
            }
            
            if (localWire["RepeaterType"]) {
                std::string type = localWire["RepeaterType"].as<std::string>();
                if (type == "RepeatedOpt") {
                    minLocalWireRepeaterType = repeated_opt;
                    maxLocalWireRepeaterType = repeated_opt;
                } else if (type == "Repeated5%Penalty") {
                    minLocalWireRepeaterType = repeated_5;
                    maxLocalWireRepeaterType = repeated_5;
                } else if (type == "Repeated10%Penalty") {
                    minLocalWireRepeaterType = repeated_10;
                    maxLocalWireRepeaterType = repeated_10;
                } else if (type == "Repeated20%Penalty") {
                    minLocalWireRepeaterType = repeated_20;
                    maxLocalWireRepeaterType = repeated_20;
                } else if (type == "Repeated30%Penalty") {
                    minLocalWireRepeaterType = repeated_30;
                    maxLocalWireRepeaterType = repeated_30;
                } else if (type == "Repeated40%Penalty") {
                    minLocalWireRepeaterType = repeated_40;
                    maxLocalWireRepeaterType = repeated_40;
                } else if (type == "Repeated50%Penalty") {
                    minLocalWireRepeaterType = repeated_50;
                    maxLocalWireRepeaterType = repeated_50;
                } else {
                    minLocalWireRepeaterType = repeated_none;
                    maxLocalWireRepeaterType = repeated_none;
                }
            }
            
            if (localWire["UseLowSwing"]) {
                std::string use = localWire["UseLowSwing"].as<std::string>();
                bool useLowSwing = (use == "Yes" || use == "yes" || use == "true");
                minIsLocalWireLowSwing = useLowSwing;
                maxIsLocalWireLowSwing = useLowSwing;
            }
        }
        
        // Also support flat local wire fields
        if (config["LocalWireType"]) {
            std::string type = config["LocalWireType"].as<std::string>();
            if (type == "LocalAggressive") {
                minLocalWireType = local_aggressive;
                maxLocalWireType = local_aggressive;
            } else if (type == "LocalConservative") {
                minLocalWireType = local_conservative;
                maxLocalWireType = local_conservative;
            } else if (type == "SemiAggressive") {
                minLocalWireType = semi_aggressive;
                maxLocalWireType = semi_aggressive;
            } else if (type == "SemiConservative") {
                minLocalWireType = semi_conservative;
                maxLocalWireType = semi_conservative;
            } else if (type == "GlobalAggressive") {
                minLocalWireType = global_aggressive;
                maxLocalWireType = global_aggressive;
            } else if (type == "GlobalConservative") {
                minLocalWireType = global_conservative;
                maxLocalWireType = global_conservative;
            } else {
                minLocalWireType = dram_wordline;
                maxLocalWireType = dram_wordline;
            }
        }
        
        if (config["LocalWireRepeaterType"]) {
            std::string type = config["LocalWireRepeaterType"].as<std::string>();
            if (type == "RepeatedOpt") {
                minLocalWireRepeaterType = repeated_opt;
                maxLocalWireRepeaterType = repeated_opt;
            } else if (type == "Repeated5%Penalty") {
                minLocalWireRepeaterType = repeated_5;
                maxLocalWireRepeaterType = repeated_5;
            } else if (type == "Repeated10%Penalty") {
                minLocalWireRepeaterType = repeated_10;
                maxLocalWireRepeaterType = repeated_10;
            } else if (type == "Repeated20%Penalty") {
                minLocalWireRepeaterType = repeated_20;
                maxLocalWireRepeaterType = repeated_20;
            } else if (type == "Repeated30%Penalty") {
                minLocalWireRepeaterType = repeated_30;
                maxLocalWireRepeaterType = repeated_30;
            } else if (type == "Repeated40%Penalty") {
                minLocalWireRepeaterType = repeated_40;
                maxLocalWireRepeaterType = repeated_40;
            } else if (type == "Repeated50%Penalty") {
                minLocalWireRepeaterType = repeated_50;
                maxLocalWireRepeaterType = repeated_50;
            } else {
                minLocalWireRepeaterType = repeated_none;
                maxLocalWireRepeaterType = repeated_none;
            }
        }
        
        if (config["LocalWireUseLowSwing"]) {
            std::string use = config["LocalWireUseLowSwing"].as<std::string>();
            bool useLowSwing = (use == "Yes" || use == "yes" || use == "true");
            minIsLocalWireLowSwing = useLowSwing;
            maxIsLocalWireLowSwing = useLowSwing;
        }
        
        // Global Wire Configuration
        if (config["GlobalWire"]) {
            YAML::Node globalWire = config["GlobalWire"];
            if (globalWire["Type"]) {
                std::string type = globalWire["Type"].as<std::string>();
                if (type == "LocalAggressive") {
                    minGlobalWireType = local_aggressive;
                    maxGlobalWireType = local_aggressive;
                } else if (type == "LocalConservative") {
                    minGlobalWireType = local_conservative;
                    maxGlobalWireType = local_conservative;
                } else if (type == "SemiAggressive") {
                    minGlobalWireType = semi_aggressive;
                    maxGlobalWireType = semi_aggressive;
                } else if (type == "SemiConservative") {
                    minGlobalWireType = semi_conservative;
                    maxGlobalWireType = semi_conservative;
                } else if (type == "GlobalAggressive") {
                    minGlobalWireType = global_aggressive;
                    maxGlobalWireType = global_aggressive;
                } else if (type == "GlobalConservative") {
                    minGlobalWireType = global_conservative;
                    maxGlobalWireType = global_conservative;
                } else {
                    minGlobalWireType = dram_wordline;
                    maxGlobalWireType = dram_wordline;
                }
            }
            
            if (globalWire["RepeaterType"]) {
                std::string type = globalWire["RepeaterType"].as<std::string>();
                if (type == "RepeatedOpt") {
                    minGlobalWireRepeaterType = repeated_opt;
                    maxGlobalWireRepeaterType = repeated_opt;
                } else if (type == "Repeated5%Penalty") {
                    minGlobalWireRepeaterType = repeated_5;
                    maxGlobalWireRepeaterType = repeated_5;
                } else if (type == "Repeated10%Penalty") {
                    minGlobalWireRepeaterType = repeated_10;
                    maxGlobalWireRepeaterType = repeated_10;
                } else if (type == "Repeated20%Penalty") {
                    minGlobalWireRepeaterType = repeated_20;
                    maxGlobalWireRepeaterType = repeated_20;
                } else if (type == "Repeated30%Penalty") {
                    minGlobalWireRepeaterType = repeated_30;
                    maxGlobalWireRepeaterType = repeated_30;
                } else if (type == "Repeated40%Penalty") {
                    minGlobalWireRepeaterType = repeated_40;
                    maxGlobalWireRepeaterType = repeated_40;
                } else if (type == "Repeated50%Penalty") {
                    minGlobalWireRepeaterType = repeated_50;
                    maxGlobalWireRepeaterType = repeated_50;
                } else {
                    minGlobalWireRepeaterType = repeated_none;
                    maxGlobalWireRepeaterType = repeated_none;
                }
            }
            
            if (globalWire["UseLowSwing"]) {
                std::string use = globalWire["UseLowSwing"].as<std::string>();
                bool useLowSwing = (use == "Yes" || use == "yes" || use == "true");
                minIsGlobalWireLowSwing = useLowSwing;
                maxIsGlobalWireLowSwing = useLowSwing;
            }
        }
        
        // Also support flat global wire fields
        if (config["GlobalWireType"]) {
            std::string type = config["GlobalWireType"].as<std::string>();
            if (type == "LocalAggressive") {
                minGlobalWireType = local_aggressive;
                maxGlobalWireType = local_aggressive;
            } else if (type == "LocalConservative") {
                minGlobalWireType = local_conservative;
                maxGlobalWireType = local_conservative;
            } else if (type == "SemiAggressive") {
                minGlobalWireType = semi_aggressive;
                maxGlobalWireType = semi_aggressive;
            } else if (type == "SemiConservative") {
                minGlobalWireType = semi_conservative;
                maxGlobalWireType = semi_conservative;
            } else if (type == "GlobalAggressive") {
                minGlobalWireType = global_aggressive;
                maxGlobalWireType = global_aggressive;
            } else if (type == "GlobalConservative") {
                minGlobalWireType = global_conservative;
                maxGlobalWireType = global_conservative;
            } else {
                minGlobalWireType = dram_wordline;
                maxGlobalWireType = dram_wordline;
            }
        }
        
        if (config["GlobalWireRepeaterType"]) {
            std::string type = config["GlobalWireRepeaterType"].as<std::string>();
            if (type == "RepeatedOpt") {
                minGlobalWireRepeaterType = repeated_opt;
                maxGlobalWireRepeaterType = repeated_opt;
            } else if (type == "Repeated5%Penalty") {
                minGlobalWireRepeaterType = repeated_5;
                maxGlobalWireRepeaterType = repeated_5;
            } else if (type == "Repeated10%Penalty") {
                minGlobalWireRepeaterType = repeated_10;
                maxGlobalWireRepeaterType = repeated_10;
            } else if (type == "Repeated20%Penalty") {
                minGlobalWireRepeaterType = repeated_20;
                maxGlobalWireRepeaterType = repeated_20;
            } else if (type == "Repeated30%Penalty") {
                minGlobalWireRepeaterType = repeated_30;
                maxGlobalWireRepeaterType = repeated_30;
            } else if (type == "Repeated40%Penalty") {
                minGlobalWireRepeaterType = repeated_40;
                maxGlobalWireRepeaterType = repeated_40;
            } else if (type == "Repeated50%Penalty") {
                minGlobalWireRepeaterType = repeated_50;
                maxGlobalWireRepeaterType = repeated_50;
            } else {
                minGlobalWireRepeaterType = repeated_none;
                maxGlobalWireRepeaterType = repeated_none;
            }
        }
        
        if (config["GlobalWireUseLowSwing"]) {
            std::string use = config["GlobalWireUseLowSwing"].as<std::string>();
            bool useLowSwing = (use == "Yes" || use == "yes" || use == "true");
            minIsGlobalWireLowSwing = useLowSwing;
            maxIsGlobalWireLowSwing = useLowSwing;
        }
        
        // Routing
        if (config["Routing"]) {
            std::string routing = config["Routing"].as<std::string>();
            routingMode = (routing == "H-tree") ? h_tree : non_h_tree;
        }
        
        if (config["InternalSensing"]) {
            if (config["InternalSensing"].IsScalar()) {
                std::string sensing = config["InternalSensing"].as<std::string>();
                internalSensing = (sensing == "true" || sensing == "True" || sensing == "yes" || sensing == "Yes");
            } else {
                internalSensing = config["InternalSensing"].as<bool>();
            }
        }
        
        // Operating Conditions
        if (config["Temperature"])
            temperature = config["Temperature"].as<int>();
        
        // Additional parameters
        if (config["MaxDriverCurrent"])
            maxDriverCurrent = config["MaxDriverCurrent"].as<double>();
        
        if (config["MaxNmosSize"])
            maxNmosSize = config["MaxNmosSize"].as<double>();
        
        if (config["WriteScheme"]) {
            std::string scheme = config["WriteScheme"].as<std::string>();
            if (scheme == "SetBeforeReset")
                writeScheme = set_before_reset;
            else if (scheme == "ResetBeforeSet")
                writeScheme = reset_before_set;
            else if (scheme == "EraseBeforeSet")
                writeScheme = erase_before_set;
            else if (scheme == "EraseBeforeReset")
                writeScheme = erase_before_reset;
            else if (scheme == "WriteAndVerify")
                writeScheme = write_and_verify;
            else
                writeScheme = normal_write;
        }
        
        // Buffer Design Optimization
        if (config["BufferDesignOptimization"]) {
            std::string opt = config["BufferDesignOptimization"].as<std::string>();
            if (opt == "latency") {
                minAreaOptimizationLevel = 0;
                maxAreaOptimizationLevel = 0;
            } else if (opt == "area") {
                minAreaOptimizationLevel = 2;
                maxAreaOptimizationLevel = 2;
			} else {
                minAreaOptimizationLevel = 1;
                maxAreaOptimizationLevel = 1;
            }
        }
        
        // Flash-specific parameters
        if (config["FlashPageSize"]) {
            pageSize = config["FlashPageSize"].as<long>() * 8;  // Byte to bit
        }
        
        if (config["FlashBlockSize"]) {
            flashBlockSize = config["FlashBlockSize"].as<long>() * (8 * 1024);  // KB to bit
        }
        
        // Force configurations
        if (config["ForceBank"]) {
            YAML::Node forceBank = config["ForceBank"];
            if (forceBank["TotalRows"])
                minNumRowMat = maxNumRowMat = forceBank["TotalRows"].as<int>();
            if (forceBank["TotalColumns"])
                minNumColumnMat = maxNumColumnMat = forceBank["TotalColumns"].as<int>();
            if (forceBank["ActiveRows"])
                minNumActiveMatPerColumn = maxNumActiveMatPerColumn = forceBank["ActiveRows"].as<int>();
            if (forceBank["ActiveColumns"])
                minNumActiveMatPerRow = maxNumActiveMatPerRow = forceBank["ActiveColumns"].as<int>();
        }
        
        if (config["ForceMat"]) {
            YAML::Node forceMat = config["ForceMat"];
            if (forceMat["TotalRows"])
                minNumRowSubarray = maxNumRowSubarray = forceMat["TotalRows"].as<int>();
            if (forceMat["TotalColumns"])
                minNumColumnSubarray = maxNumColumnSubarray = forceMat["TotalColumns"].as<int>();
            if (forceMat["ActiveRows"])
                minNumActiveSubarrayPerColumn = maxNumActiveSubarrayPerColumn = forceMat["ActiveRows"].as<int>();
            if (forceMat["ActiveColumns"])
                minNumActiveSubarrayPerRow = maxNumActiveSubarrayPerRow = forceMat["ActiveColumns"].as<int>();
        }
        
        if (config["ForceMuxSenseAmp"]) {
            minMuxSenseAmp = maxMuxSenseAmp = config["ForceMuxSenseAmp"].as<int>();
        }
        
        if (config["ForceMuxOutputLev1"]) {
            minMuxOutputLev1 = maxMuxOutputLev1 = config["ForceMuxOutputLev1"].as<int>();
        }
        
        if (config["ForceMuxOutputLev2"]) {
            minMuxOutputLev2 = maxMuxOutputLev2 = config["ForceMuxOutputLev2"].as<int>();
        }
        
        // CACTI Assumption
        if (config["UseCactiAssumption"]) {
            std::string use = config["UseCactiAssumption"].as<std::string>();
            if (use == "Yes" || use == "yes" || use == "true") {
                useCactiAssumption = true;
                minNumActiveMatPerRow = maxNumColumnMat;
                maxNumActiveMatPerRow = maxNumColumnMat;
                minNumActiveMatPerColumn = 1;
                maxNumActiveMatPerColumn = 1;
                minNumRowSubarray = 2;
                maxNumRowSubarray = 2;
                minNumColumnSubarray = 2;
                maxNumColumnSubarray = 2;
                minNumActiveSubarrayPerRow = 2;
                maxNumActiveSubarrayPerRow = 2;
                minNumActiveSubarrayPerColumn = 2;
                maxNumActiveSubarrayPerColumn = 2;
            } else {
                useCactiAssumption = false;
            }
        }
        
        // Constraints
        if (config["Constraints"]) {
            YAML::Node constraints = config["Constraints"];
            if (constraints["ReadLatency"]) {
                readLatencyConstraint = constraints["ReadLatency"].as<double>();
                isConstraintApplied = true;
            }
            if (constraints["WriteLatency"]) {
                writeLatencyConstraint = constraints["WriteLatency"].as<double>();
                isConstraintApplied = true;
            }
            if (constraints["ReadDynamicEnergy"]) {
                readDynamicEnergyConstraint = constraints["ReadDynamicEnergy"].as<double>();
                isConstraintApplied = true;
            }
            if (constraints["WriteDynamicEnergy"]) {
                writeDynamicEnergyConstraint = constraints["WriteDynamicEnergy"].as<double>();
                isConstraintApplied = true;
            }
            if (constraints["Leakage"]) {
                leakageConstraint = constraints["Leakage"].as<double>();
                isConstraintApplied = true;
            }
            if (constraints["Area"]) {
                areaConstraint = constraints["Area"].as<double>();
                isConstraintApplied = true;
            }
            if (constraints["ReadEdp"]) {
                readEdpConstraint = constraints["ReadEdp"].as<double>();
                isConstraintApplied = true;
            }
            if (constraints["WriteEdp"]) {
                writeEdpConstraint = constraints["WriteEdp"].as<double>();
                isConstraintApplied = true;
            }
        }
        
        // Also support flat constraint fields for backwards compatibility
        if (config["ApplyReadLatencyConstraint"]) {
            readLatencyConstraint = config["ApplyReadLatencyConstraint"].as<double>();
            isConstraintApplied = true;
        }
        if (config["ApplyWriteLatencyConstraint"]) {
            writeLatencyConstraint = config["ApplyWriteLatencyConstraint"].as<double>();
            isConstraintApplied = true;
        }
        if (config["ApplyReadDynamicEnergyConstraint"]) {
            readDynamicEnergyConstraint = config["ApplyReadDynamicEnergyConstraint"].as<double>();
            isConstraintApplied = true;
        }
        if (config["ApplyWriteDynamicEnergyConstraint"]) {
            writeDynamicEnergyConstraint = config["ApplyWriteDynamicEnergyConstraint"].as<double>();
            isConstraintApplied = true;
        }
        if (config["ApplyLeakageConstraint"]) {
            leakageConstraint = config["ApplyLeakageConstraint"].as<double>();
            isConstraintApplied = true;
        }
        if (config["ApplyAreaConstraint"]) {
            areaConstraint = config["ApplyAreaConstraint"].as<double>();
            isConstraintApplied = true;
        }
        if (config["ApplyReadEdpConstraint"]) {
            readEdpConstraint = config["ApplyReadEdpConstraint"].as<double>();
            isConstraintApplied = true;
        }
        if (config["ApplyWriteEdpConstraint"]) {
            writeEdpConstraint = config["ApplyWriteEdpConstraint"].as<double>();
            isConstraintApplied = true;
        }
        
    } catch (const YAML::Exception& e) {
        std::cout << "Error parsing YAML file: " << e.what() << std::endl;
        exit(-1);
    } catch (const std::exception& e) {
        std::cout << "Error reading file: " << e.what() << std::endl;
        exit(-1);
    }
}

void InputParameter::PrintInputParameter() {
	std::cout << std::endl << "====================" << std::endl << "DESIGN SPECIFICATION" << std::endl << "====================" << std::endl;
	std::cout << "Design Target: ";
	switch (designTarget) {
	case cache:
		std::cout << "Cache" << std::endl;
		break;
	case RAM_chip:
		std::cout << "Random Access Memory" << std::endl;
		break;
	default:	/* CAM */
		std::cout << "Content Addressable Memory" << std::endl;
	}

	std::cout << "Capacity   : ";
	if (capacity < 1024 * 1024)
		std::cout << capacity / 1024 << "KB" << std::endl;
	else if (capacity < 1024 * 1024 * 1024)
		std::cout << capacity / 1024 / 1024 << "MB" << std::endl;
	else
		std::cout << capacity / 1024 / 1024 / 1024 << "GB" << std::endl;

	if (designTarget == cache) {
		std::cout << "Cache Line Size: " << wordWidth / 8 << "Bytes" << std::endl;
		std::cout << "Cache Associativity: " << associativity << " Ways" << std::endl;
	} else {
		std::cout << "Data Width : " << wordWidth << "Bits";
		if (wordWidth % 8 == 0)
			std::cout << " (" << wordWidth / 8 << "Bytes)" << std::endl;
		else
			std::cout << std::endl;
	}
	if (designTarget == RAM_chip && (gCell.memCellType == SLCNAND || gCell.memCellType == MLCNAND)) {
		std::cout << "Page Size  : " << pageSize / 8 << "Bytes" << std::endl;
		std::cout << "Block Size : " << flashBlockSize / 8 / 1024 << "KB" << std::endl;
	}
	// TO-DO: tedious work here!!!

	if (optimizationTarget == full_exploration) {
		std::cout << std::endl << "Full design space exploration ... might take hours" << std::endl;
	} else {
		std::cout << std::endl << "Searching for the best solution that is optimized for ";
		switch (optimizationTarget) {
		case read_latency_optimized:
			std::cout << "read latency ..." << std::endl;
			break;
		case write_latency_optimized:
			std::cout << "write latency ..." << std::endl;
			break;
		case read_energy_optimized:
			std::cout << "read energy ..." << std::endl;
			break;
		case write_energy_optimized:
			std::cout << "write energy ..." << std::endl;
			break;
		case read_edp_optimized:
			std::cout << "read energy-delay-product ..." << std::endl;
			break;
		case write_edp_optimized:
			std::cout << "write energy-delay-product ..." << std::endl;
			break;
		case leakage_optimized:
			std::cout << "leakage power ..." << std::endl;
			break;
		default:	/* area */
			std::cout << "area ..." << std::endl;
		}
	}
}
