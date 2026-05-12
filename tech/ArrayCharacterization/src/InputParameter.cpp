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
#include "typedef.hpp"
#include "enuminfo.hpp"

#include "yaml-cpp/yaml.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void InputParameter::ReadInputParameterFromFile(const std::string& inputFile) {
    try {
        YAML::Node config = YAML::LoadFile(inputFile);
        
        // Memory Cell Input File
        yamlValueFromNode(fileMemCell, config, "MemoryCellInputFile");
        bool hasProcessNode = yamlValueFromNode(processNode, config, "ProcessNode");
        
        // Process Technology
        // if we cant get ProcessNodeW and ProcessNode exists, then use ProcessNode instead
        if (!yamlValueFromNode(processNodeW, config, "ProcessNodeW") && hasProcessNode) {
            processNodeW = processNode;
        }
        
        // if we cant get ProcessNodeR and ProcessNode exists, then use ProcessNode instead
        if (!yamlValueFromNode(processNodeR, config, "ProcessNodeR") && hasProcessNode) {
            processNodeR = processNode;
        }
        
        // Device Roadmap
        bool hasDeviceRoadmap = yamlValueFromNode(deviceRoadmap, config, "DeviceRoadmap");

        // if we cant get DeviceRoadmapW and DeviceRoadmap exists, then use DeviceRoadmap instead
        if (!yamlValueFromNode(deviceRoadmapW, config, "DeviceRoadmapW") && hasDeviceRoadmap) {
            deviceRoadmapW = deviceRoadmap;
        }
        
        // if we cant get DeviceRoadmapR and DeviceRoadmap exists, then use DeviceRoadmap instead
        if (!yamlValueFromNode(deviceRoadmapR, config, "DeviceRoadmapR") && hasDeviceRoadmap) {
            deviceRoadmapR = deviceRoadmap;
        }
        
        // Design Configuration
        yamlValueFromNode(designTarget, config, "DesignTarget");
        yamlValueFromNode(cacheAccessMode, config, "CacheAccessMode");
        yamlValueFromNode(associativity, config, "Associativity");
        
        // Optimization
        yamlValueFromNode(optimizationTarget, config, "OptimizationTarget");
        yamlValueFromNode(outputFilePrefix, config, "OutputFilePrefix");
        yamlValueFromNode(outputDirectory, config, "OutputDirectory");
        yamlValueFromNode(isPruningEnabled, config, "EnablePruning");
        
        // Memory Specifications - Support both nested and flat formats
        if (config["Capacity"]) {
            if (config["Capacity"].IsMap()) {
                // Nested format
                YAML::Node capacityNode = config["Capacity"];
                long cap;
                std::string unit;
                yamlValueFromNode(cap, capacityNode, "Value");
                if (yamlValueFromNode(unit, capacityNode, "Unit")) {
                    if (unit == "B" ) {
                        capacity = cap;
                    } else if (unit == "KB") {
                        capacity = cap * 1024;
                    } else if (unit == "MB") {
                        capacity = cap * 1024 * 1024;
                    }
                }
            } else {
                // Flat format - assume KB for backwards compatibility
                if (yamlValueFromNode(capacity, config, "Capacity")) {
                    capacity *= 1024;
                }
            }
        }
        
        // Also support old-style flat capacity fields
        if (yamlValueFromNode(capacity, config, "Capacity_B")) {
            // capacity *= 1;
        } else if (yamlValueFromNode(capacity, config, "Capacity_KB")) {
            capacity *= 1024;
        } else if (yamlValueFromNode(capacity, config, "Capacity_MB")) {
            capacity *= 1024 * 1024;
        }
        
        yamlValueFromNode(wordWidth, config, "WordWidth");
        
        // Wire Configuration
        if (config["LocalWire"]) {
            YAML::Node localWire = config["LocalWire"];
            yamlValueFromNode(minLocalWireType, localWire, "Type");
            yamlValueFromNode(maxLocalWireType, localWire, "Type");
            
            yamlValueFromNode(minLocalWireRepeaterType, localWire, "RepeaterType");
            yamlValueFromNode(maxLocalWireRepeaterType, localWire, "RepeaterType");

            yamlValueFromNode(minIsLocalWireLowSwing, localWire, "UseLowSwing");
            yamlValueFromNode(maxIsLocalWireLowSwing, localWire, "UseLowSwing");
        }
        
        // Also support flat local wire fields
        yamlValueFromNode(minLocalWireType, config, "LocalWireType");
        yamlValueFromNode(maxLocalWireType, config, "LocalWireType");
        
        yamlValueFromNode(minLocalWireRepeaterType, config, "LocalWireRepeaterType");
        yamlValueFromNode(maxLocalWireRepeaterType, config, "LocalWireRepeaterType");
        
        yamlValueFromNode(minIsLocalWireLowSwing, config, "LocalWireUseLowSwing");
        yamlValueFromNode(maxIsLocalWireLowSwing, config, "LocalWireUseLowSwing");
        
        // Global Wire Configuration
        if (config["GlobalWire"]) {
            YAML::Node globalWire = config["GlobalWire"];
            yamlValueFromNode(minGlobalWireType, globalWire, "Type");
            yamlValueFromNode(maxGlobalWireType, globalWire, "Type");
            
            yamlValueFromNode(minGlobalWireRepeaterType, globalWire, "RepeaterType");
            yamlValueFromNode(maxGlobalWireRepeaterType, globalWire, "RepeaterType");
            
            yamlValueFromNode(minIsGlobalWireLowSwing, globalWire, "UseLowSwing");
            yamlValueFromNode(maxIsGlobalWireLowSwing, globalWire, "UseLowSwing");
        }
        
        // Also support flat global wire fields
        yamlValueFromNode(minGlobalWireType, config, "GlobalWireType");
        yamlValueFromNode(maxGlobalWireType, config, "GlobalWireType");
        
        yamlValueFromNode(minGlobalWireRepeaterType, config, "GlobalWireRepeaterType");
        yamlValueFromNode(maxGlobalWireRepeaterType, config, "GlobalWireRepeaterType");
        
        yamlValueFromNode(minIsGlobalWireLowSwing, config, "GlobalWireUseLowSwing");
        yamlValueFromNode(maxIsGlobalWireLowSwing, config, "GlobalWireUseLowSwing");
        
        // Routing
        yamlValueFromNode(routingMode, config, "Routing");
        yamlValueFromNode(internalSensing, config, "InternalSensing");
        
        // Operating Conditions
        yamlValueFromNode(temperature, config, "Temperature");
        
        // Additional parameters
        yamlValueFromNode(maxDriverCurrent, config, "MaxDriverCurrent");
        yamlValueFromNode(maxNmosSize, config, "MaxNmosSize");
        yamlValueFromNode(writeScheme, config, "WriteScheme");
        
        // Buffer Design Optimization
        yamlValueFromNode(minAreaOptimizationLevel, config, "BufferDesignOptimization");
        yamlValueFromNode(maxAreaOptimizationLevel, config, "BufferDesignOptimization");
        
        // Flash-specific parameters
        if (yamlValueFromNode(pageSize, config, "FlashPageSize")) {
            pageSize *= 8; // Byte to bit
        }
        
        if (yamlValueFromNode(flashBlockSize, config, "FlashBlockSize")) {
            flashBlockSize *= (8 * 1024); // KB to bit
        }
        
        // Force configurations
        if (config["ForceBank"]) {
            YAML::Node forceBank = config["ForceBank"];
            yamlValueFromNode(minNumRowMat, forceBank, "TotalRows");
            yamlValueFromNode(maxNumRowMat, forceBank, "TotalRows");

            yamlValueFromNode(minNumColumnMat, forceBank, "TotalColumns");
            yamlValueFromNode(maxNumColumnMat, forceBank, "TotalColumns");

            yamlValueFromNode(minNumActiveMatPerColumn, forceBank, "ActiveRows");
            yamlValueFromNode(maxNumActiveMatPerColumn, forceBank, "ActiveRows");

            yamlValueFromNode(minNumActiveMatPerRow, forceBank, "ActiveColumns");
            yamlValueFromNode(maxNumActiveMatPerRow, forceBank, "ActiveColumns");
        }
        
        if (config["ForceMat"]) {
            YAML::Node forceMat = config["ForceMat"];
            yamlValueFromNode(minNumRowSubarray, forceMat, "TotalRows");
            yamlValueFromNode(maxNumRowSubarray, forceMat, "TotalRows");

            yamlValueFromNode(minNumColumnSubarray, forceMat, "TotalColumns");
            yamlValueFromNode(maxNumColumnSubarray, forceMat, "TotalColumns");

            yamlValueFromNode(minNumActiveSubarrayPerColumn, forceMat, "ActiveRows");
            yamlValueFromNode(maxNumActiveSubarrayPerColumn, forceMat, "ActiveRows");

            yamlValueFromNode(minNumActiveSubarrayPerRow, forceMat, "ActiveColumns");
            yamlValueFromNode(maxNumActiveSubarrayPerRow, forceMat, "ActiveColumns");
        }
        
        yamlValueFromNode(minMuxSenseAmp, config, "ForceMuxSenseAmp");
        yamlValueFromNode(maxMuxSenseAmp, config, "ForceMuxSenseAmp");
        
        yamlValueFromNode(minMuxOutputLev1, config, "ForceMuxOutputLev1");
        yamlValueFromNode(maxMuxOutputLev1, config, "ForceMuxOutputLev1");
        
        yamlValueFromNode(minMuxOutputLev2, config, "ForceMuxOutputLev2");
        yamlValueFromNode(maxMuxOutputLev2, config, "ForceMuxOutputLev2");
        
        // CACTI Assumption
        if (yamlValueFromNode(useCactiAssumption, config, "UseCactiAssumption")) {
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
        }
        
        // Constraints
        if (config["Constraints"]) {
            YAML::Node constraints = config["Constraints"];
            if (yamlValueFromNode(readLatencyConstraint, constraints, "ReadLatency") ||
                yamlValueFromNode(writeLatencyConstraint, constraints, "WriteLatency") ||
                yamlValueFromNode(readDynamicEnergyConstraint, constraints, "ReadDynamicEnergy") ||
                yamlValueFromNode(writeDynamicEnergyConstraint, constraints, "WriteDynamicEnergy") ||
                yamlValueFromNode(leakageConstraint, constraints, "Leakage") ||
                yamlValueFromNode(areaConstraint, constraints, "Area") ||
                yamlValueFromNode(readEdpConstraint, constraints, "ReadEdp") ||
                yamlValueFromNode(writeEdpConstraint, constraints, "WriteEdp")
               ) {
                isConstraintApplied = true;
            }
        }
        
        // Also support flat constraint fields for backwards compatibility
        if (yamlValueFromNode(readLatencyConstraint, config, "ApplyReadLatencyConstraint") ||
            yamlValueFromNode(writeLatencyConstraint, config, "ApplyWriteLatencyConstraint") ||
            yamlValueFromNode(readDynamicEnergyConstraint, config, "ApplyReadDynamicEnergyConstraint") ||
            yamlValueFromNode(writeDynamicEnergyConstraint, config, "ApplyWriteDynamicEnergyConstraint") ||
            yamlValueFromNode(leakageConstraint, config, "ApplyLeakageConstraint") ||
            yamlValueFromNode(areaConstraint, config, "ApplyAreaConstraint") ||
            yamlValueFromNode(readEdpConstraint, config, "ApplyReadEdpConstraint") ||
            yamlValueFromNode(writeEdpConstraint, config, "ApplyWriteEdpConstraint")
           ) {
            isConstraintApplied = true;
        }
        
    } catch (const YAML::Exception& e) {
        std::cerr << "Error parsing YAML file: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    } catch (const std::exception& e) {
        std::cerr << "Error reading file: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void InputParameter::PrintInputParameter() {
	std::cout << "\n====================\n"
              << "DESIGN SPECIFICATION\n"
              << "====================\n";

	std::cout << "Design Target: ";
	switch (designTarget) {
	case cache:
		std::cout << "Cache\n";
		break;
	case RAM_chip:
		std::cout << "Random Access Memory\n";
		break;
	case CAM_chip:
		std::cout << "Content Addressable Memory\n";
        break;
	}

	std::cout << "Capacity   : ";
    if (capacity < 1024) {
        std::cout << capacity << "B\n";
    } else if (capacity < 1024 * 1024) {
		std::cout << capacity / 1024 << "KB\n";
	} else if (capacity < 1024 * 1024 * 1024) {
		std::cout << capacity / 1024 / 1024 << "MB\n";
	} else {
		std::cout << capacity / 1024 / 1024 / 1024 << "GB\n";
    }

	if (designTarget == cache) {
		std::cout << "Cache Line Size: " << wordWidth / 8 << "Bytes\n";
		std::cout << "Cache Associativity: " << associativity << " Ways\n";
	} else {
		std::cout << "Data Width : " << wordWidth << "Bits";
		if (wordWidth % 8 == 0) {
			std::cout << " (" << wordWidth / 8 << "Bytes)";
        }
        std::cout << "\n";
	}
	if (designTarget == RAM_chip && (gCell.memCellType == SLCNAND || gCell.memCellType == MLCNAND)) {
		std::cout << "Page Size  : " << pageSize / 8 << "Bytes\n";
		std::cout << "Block Size : " << flashBlockSize / 8 / 1024 << "KB\n";
	}
	// TO-DO: tedious work here!!!

	if (optimizationTarget == full_exploration) {
		std::cout << "\nFull design space exploration ... might take hours\n";
	} else {
		std::cout << "\nSearching for the best solution that is optimized for " << optimizationTarget << " ...\n";
	}
}
