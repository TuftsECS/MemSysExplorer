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


#ifndef MSE_INPUTPARAMETER_HPP
#define MSE_INPUTPARAMETER_HPP

#include "constant.hpp"
#include "typedef.hpp"

#include <iostream>
#include <string>
#include <stdint.h>

class InputParameter {
public:
	/* Functions */
	void ReadInputParameterFromFile(const std::string& inputFile);
	void PrintInputParameter();

	/* Properties */
	DesignTarget designTarget = cache;		/* Cache, RAM, or CAM */
	OptimizationTarget optimizationTarget = read_latency_optimized;	/* Either read latency, write latency, read energy, write energy, leakage, or area */
	int processNode = 90;				/* Process node (nm) */
	int processNodeW = 90;				/* Process node (nm) */
	int processNodeR = 90;				/* Process node (nm) */
	int64_t capacity;				/* Memory/cache capacity, Unit: Byte */
	long wordWidth;					/* The width of each input/output word, Unit: bit */
	DeviceRoadmap deviceRoadmap = LOP;	/* ITRS roadmap: HP, LSTP, or LOP */
	DeviceRoadmap deviceRoadmapW = LOP;	/* ITRS roadmap: HP, LSTP, or LOP */
	DeviceRoadmap deviceRoadmapR = LOP;	/* ITRS roadmap: HP, LSTP, or LOP */
	std::string fileMemCell;				/* Input file name of memory cell type */
	int temperature;				/* The ambient temperature, Unit: K */
	double maxDriverCurrent = 0;        /* The maximum driving current that the wordline/bitline driver can provide */
	WriteScheme writeScheme = normal_write;		/* The write scheme */
	double readLatencyConstraint = invalid_value;	/* The allowed variation to the best read latency */
	double writeLatencyConstraint = invalid_value;	/* The allowed variation to the best write latency */
	double readDynamicEnergyConstraint = invalid_value;		/* The allowed variation to the best read dynamic energy */
	double writeDynamicEnergyConstraint = invalid_value;	/* The allowed variation to the best write dynamic energy */
	double leakageConstraint = invalid_value;		/* The allowed variation to the best leakage energy */
	double areaConstraint = invalid_value;			/* The allowed variation to the best leakage energy */
	double readEdpConstraint = invalid_value;		/* The allowed variation to the best read EDP */
	double writeEdpConstraint = invalid_value;		/* The allowed variation to the best write EDP */
	bool isConstraintApplied = false;		/* If any design constraint is applied */
	bool isPruningEnabled = false;			/* Whether to prune the results during the exploration */
	bool useCactiAssumption = false;		/* Use the CACTI assumptions on the array organization */

	int associativity = 1;				/* Associativity, for cache design only, default value for non-cache design */
	CacheAccessMode cacheAccessMode = normal_access_mode;	/* Access mode (for cache only) : normal, sequential, fast */

	long pageSize = 0;					/* Unit: bit, For DRAM and NAND flash memory only */
	long flashBlockSize = 0;				/* Unit: bit, For NAND flash memory only */

	RoutingMode routingMode = h_tree;
	bool internalSensing = true;

	double maxNmosSize = MAX_NMOS_SIZE;				/* Default value is MAX_NMOS_SIZE in constant.h, however, user might change it, Unit: F */

	std::string outputFilePrefix = "output";
	std::string outputDirectory = "results/";

	int minNumRowMat = 1;
	int maxNumRowMat = 512;
	int minNumColumnMat = 1;
	int maxNumColumnMat = 512;
	int minNumActiveMatPerRow = 1;
	int maxNumActiveMatPerRow = maxNumColumnMat;
	int minNumActiveMatPerColumn = 1;
	int maxNumActiveMatPerColumn = maxNumRowMat;
	int minNumRowSubarray = 1;
	int maxNumRowSubarray = 2;
	int minNumColumnSubarray = 1;
	int maxNumColumnSubarray = 2;
	int minNumActiveSubarrayPerRow = 1;
	int maxNumActiveSubarrayPerRow = maxNumColumnSubarray;
	int minNumActiveSubarrayPerColumn = 1;
	int maxNumActiveSubarrayPerColumn = maxNumRowSubarray;
	int minMuxSenseAmp = 1;
	int maxMuxSenseAmp = 256;
	int minMuxOutputLev1 = 1;
	int maxMuxOutputLev1 = 256;
	int minMuxOutputLev2 = 1;
	int maxMuxOutputLev2 = 256;
	int minNumRowPerSet = 1;
	int maxNumRowPerSet = 256;
	BufferDesignTarget minAreaOptimizationLevel = latency_first;
	BufferDesignTarget maxAreaOptimizationLevel = area_first;
	WireType minLocalWireType = local_aggressive;
	WireType maxLocalWireType = local_conservative;
	WireType minGlobalWireType = global_aggressive;
	WireType maxGlobalWireType = global_conservative;
	WireRepeaterType minLocalWireRepeaterType = repeated_none;
	WireRepeaterType maxLocalWireRepeaterType = repeated_50;
	WireRepeaterType minGlobalWireRepeaterType = repeated_none;
	WireRepeaterType maxGlobalWireRepeaterType = repeated_50;
	bool minIsLocalWireLowSwing = false;
	bool maxIsLocalWireLowSwing = true;
	bool minIsGlobalWireLowSwing = false;
	bool maxIsGlobalWireLowSwing = true;
};

#endif /* MSE_INPUTPARAMETER_HPP */
