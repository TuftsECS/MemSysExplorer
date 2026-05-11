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


#ifndef MSE_MACROS_H
#define MSE_MACROS_H


#define INITIAL_BASIC_WIRE { \
	WireType basicWireType; \
	WireRepeaterType basicWireRepeaterType; \
	bool isBasicLowSwing; \
	if (gInputParameter.minLocalWireType == gInputParameter.maxLocalWireType) \
		basicWireType = (WireType)gInputParameter.minLocalWireType; \
	else \
		basicWireType = local_aggressive; \
	if (gInputParameter.minLocalWireRepeaterType == gInputParameter.maxLocalWireRepeaterType) \
		basicWireRepeaterType = (WireRepeaterType)gInputParameter.minLocalWireRepeaterType; \
	else \
		basicWireRepeaterType = repeated_none; \
	if (gInputParameter.minIsLocalWireLowSwing == gInputParameter.maxIsLocalWireLowSwing) \
		isBasicLowSwing = gInputParameter.minIsLocalWireLowSwing; \
	else \
		isBasicLowSwing = false; \
	gLocalWire.Initialize(gInputParameter.processNode, basicWireType, basicWireRepeaterType, gInputParameter.temperature, isBasicLowSwing); \
	if (gInputParameter.minGlobalWireType == gInputParameter.maxGlobalWireType) \
		basicWireType = (WireType)gInputParameter.minGlobalWireType; \
	else \
		basicWireType = global_aggressive; \
	if (gInputParameter.minGlobalWireRepeaterType == gInputParameter.maxGlobalWireRepeaterType) \
		basicWireRepeaterType = (WireRepeaterType)gInputParameter.minGlobalWireRepeaterType; \
	else \
		basicWireRepeaterType = repeated_none; \
	if (gInputParameter.minIsGlobalWireLowSwing == gInputParameter.maxIsGlobalWireLowSwing) \
		isBasicLowSwing = gInputParameter.minIsGlobalWireLowSwing; \
	else \
		isBasicLowSwing = false; \
	gGlobalWire.Initialize(gInputParameter.processNode, basicWireType, basicWireRepeaterType, gInputParameter.temperature, isBasicLowSwing); \
}



#define REFINE_LOCAL_WIRE_FORLOOP \
	for (localWireType = gInputParameter.minLocalWireType; localWireType <= gInputParameter.maxLocalWireType; localWireType++) \
	for (localWireRepeaterType = gInputParameter.minLocalWireRepeaterType; localWireRepeaterType <= gInputParameter.maxLocalWireRepeaterType; localWireRepeaterType++) \
	for (isLocalWireLowSwing = gInputParameter.minIsLocalWireLowSwing; isLocalWireLowSwing <= gInputParameter.maxIsLocalWireLowSwing; isLocalWireLowSwing++) \
	if ((WireRepeaterType)localWireRepeaterType == repeated_none || (bool)isLocalWireLowSwing == false)


#define REFINE_GLOBAL_WIRE_FORLOOP \
	for (globalWireType = gInputParameter.minGlobalWireType; globalWireType <= gInputParameter.maxGlobalWireType; globalWireType++) \
	for (globalWireRepeaterType = gInputParameter.minGlobalWireRepeaterType; globalWireRepeaterType <= gInputParameter.maxGlobalWireRepeaterType; globalWireRepeaterType++) \
	for (isGlobalWireLowSwing = gInputParameter.minIsGlobalWireLowSwing; isGlobalWireLowSwing <= gInputParameter.maxIsGlobalWireLowSwing; isGlobalWireLowSwing++) \
	if ((WireRepeaterType)globalWireRepeaterType == repeated_none || (bool)isGlobalWireLowSwing == false)




#define LOAD_GLOBAL_WIRE(oldResult) { \
	gGlobalWire.Initialize(gInputParameter.processNode, (oldResult).globalWire->wireType, (oldResult).globalWire->wireRepeaterType, \
			gInputParameter.temperature, (oldResult).globalWire->isLowSwing); \
}


#define LOAD_LOCAL_WIRE(oldResult) \
	gLocalWire.Initialize(gInputParameter.processNode, (oldResult).localWire->wireType, (oldResult).localWire->wireRepeaterType, \
			gInputParameter.temperature, (oldResult).localWire->isLowSwing);



#define TRY_AND_UPDATE(oldResult, memoryType) { \
	if (gInputParameter.routingMode == h_tree) \
		trialBank = new BankWithHtree(); \
	else \
		trialBank = new BankWithoutHtree(); \
	trialBank->Initialize((oldResult).bank->numRowMat, (oldResult).bank->numColumnMat, (oldResult).bank->capacity, (oldResult).bank->blockSize, (oldResult).bank->associativity, \
				(oldResult).bank->numRowPerSet, (oldResult).bank->numActiveMatPerRow, (oldResult).bank->numActiveMatPerColumn, (oldResult).bank->muxSenseAmp, \
				gInputParameter.internalSensing, (oldResult).bank->muxOutputLev1, (oldResult).bank->muxOutputLev2, (oldResult).bank->numRowSubarray, (oldResult).bank->numColumnSubarray, \
				(oldResult).bank->numActiveSubarrayPerRow, (oldResult).bank->numActiveSubarrayPerColumn, (oldResult).bank->areaOptimizationLevel, (memoryType)); \
	trialBank->CalculateArea(); \
	trialBank->CalculateRC(); \
	trialBank->CalculateLatencyAndPower(); \
	*(tempResult.bank) = *trialBank; \
	*(tempResult.localWire) = gLocalWire; \
	*(tempResult.globalWire) = gGlobalWire; \
	oldResult.compareAndUpdate(tempResult); \
	delete trialBank; \
}



#define BIGFOR \
	for (numRowMat = gInputParameter.minNumRowMat; numRowMat <= gInputParameter.maxNumRowMat; numRowMat *= 2) \
	for (numColumnMat = gInputParameter.minNumColumnMat; numColumnMat <= gInputParameter.maxNumColumnMat; numColumnMat *= 2) \
	for (numActiveMatPerRow = MIN(numColumnMat, gInputParameter.minNumActiveMatPerRow); numActiveMatPerRow <= MIN(numColumnMat, gInputParameter.maxNumActiveMatPerRow); numActiveMatPerRow *= 2) \
	for (numActiveMatPerColumn = MIN(numRowMat, gInputParameter.minNumActiveMatPerColumn); numActiveMatPerColumn <= MIN(numRowMat, gInputParameter.maxNumActiveMatPerColumn); numActiveMatPerColumn *= 2) \
	for (numRowSubarray = gInputParameter.minNumRowSubarray; numRowSubarray <= gInputParameter.maxNumRowSubarray; numRowSubarray *= 2) \
	for (numColumnSubarray = gInputParameter.minNumColumnSubarray; numColumnSubarray <= gInputParameter.maxNumColumnSubarray; numColumnSubarray *= 2) \
	for (numActiveSubarrayPerRow = MIN(numColumnSubarray, gInputParameter.minNumActiveSubarrayPerRow); numActiveSubarrayPerRow <= MIN(numColumnSubarray, gInputParameter.maxNumActiveSubarrayPerRow); numActiveSubarrayPerRow *=2) \
	for (numActiveSubarrayPerColumn = MIN(numRowSubarray, gInputParameter.minNumActiveSubarrayPerColumn); numActiveSubarrayPerColumn <= MIN(numRowSubarray, gInputParameter.maxNumActiveSubarrayPerColumn); numActiveSubarrayPerColumn *= 2) \
	for (muxSenseAmp = gInputParameter.minMuxSenseAmp; muxSenseAmp <= gInputParameter.maxMuxSenseAmp; muxSenseAmp *= 2) \
	for (muxOutputLev1 = gInputParameter.minMuxOutputLev1; muxOutputLev1 <= gInputParameter.maxMuxOutputLev1; muxOutputLev1 *= 2) \
	for (muxOutputLev2 = gInputParameter.minMuxOutputLev2; muxOutputLev2 <= gInputParameter.maxMuxOutputLev2; muxOutputLev2 *= 2) \
	for (numRowPerSet = gInputParameter.minNumRowPerSet; numRowPerSet <= MIN(gInputParameter.maxNumRowPerSet, gInputParameter.associativity); numRowPerSet *= 2) \
	for (areaOptimizationLevel = gInputParameter.minAreaOptimizationLevel; areaOptimizationLevel <= gInputParameter.maxAreaOptimizationLevel; areaOptimizationLevel++)



#define CALCULATE(bank, memoryType) { \
	if (gInputParameter.routingMode == h_tree) \
		(bank) = new BankWithHtree(); \
	else \
		(bank) = new BankWithoutHtree(); \
	(bank)->Initialize(numRowMat, numColumnMat, capacity, blockSize, associativity, \
				numRowPerSet, numActiveMatPerRow, numActiveMatPerColumn, muxSenseAmp, \
				gInputParameter.internalSensing, muxOutputLev1, muxOutputLev2, numRowSubarray, numColumnSubarray, \
				numActiveSubarrayPerRow, numActiveSubarrayPerColumn, (BufferDesignTarget)areaOptimizationLevel, (memoryType)); \
	(bank)->CalculateArea(); \
	(bank)->CalculateRC(); \
	(bank)->CalculateLatencyAndPower(); \
}


#define UPDATE_BEST_DATA { \
	*(tempResult.bank) = *dataBank; \
	*(tempResult.localWire) = gLocalWire; \
	*(tempResult.globalWire) = gGlobalWire; \
	for (int i = 0; i < (int)full_exploration; i++) \
		bestDataResults[i].compareAndUpdate(tempResult); \
}


#define VERIFY_DATA_CAPACITY { \
	if ((long long)dataBank->mat.subarray.numColumn * dataBank->mat.subarray.numRow * dataBank->numColumnMat * \
			dataBank->numRowMat * dataBank->numColumnSubarray * dataBank->numRowSubarray != capacity) { \
				std::cout << "1 Bank = " << dataBank->numRowMat << "x" << dataBank->numColumnMat << " Mats" << std::endl; \
				std::cout << "Activation - " << dataBank->numActiveMatPerColumn << "x" << dataBank->numActiveMatPerRow << " Mats" << std::endl; \
				std::cout << "1 Mat  = " << dataBank->numRowSubarray << "x" << dataBank->numColumnSubarray << " Subarrays" << std::endl; \
				std::cout << "Activation - " << dataBank->numActiveSubarrayPerColumn << "x" << dataBank->numActiveSubarrayPerRow << " Subarrays" << std::endl; \
				std::cout << "Mux Degree - " << dataBank->muxSenseAmp << " x " << dataBank->muxOutputLev1 << " x " << dataBank->muxOutputLev2 << std::endl; \
				std::cout << "ERROR: DATA capacity violation. Shouldn't happen" << std::endl; \
				exit(-1); \
			} \
}


#define UPDATE_BEST_TAG { \
	*(tempResult.bank) = *tagBank; \
	*(tempResult.localWire) = gLocalWire; \
	*(tempResult.globalWire) = gGlobalWire; \
	for (int i = 0; i < (int)full_exploration; i++) \
		bestTagResults[i].compareAndUpdate(tempResult); \
}


#define VERIFY_TAG_CAPACITY { \
	if ((long long)tagBank->mat.subarray.numColumn * tagBank->mat.subarray.numRow * tagBank->numColumnMat * \
			tagBank->numRowMat * tagBank->numColumnSubarray * tagBank->numRowSubarray != capacity) { \
				std::cout << "1 Bank = " << tagBank->numRowMat << "x" << tagBank->numColumnMat << " Mats" << std::endl; \
				std::cout << "Activation - " << tagBank->numActiveMatPerColumn << "x" << tagBank->numActiveMatPerRow << " Mats" << std::endl; \
				std::cout << "1 Mat  = " << tagBank->numRowSubarray << "x" << tagBank->numColumnSubarray << " Subarrays" << std::endl; \
				std::cout << "Activation - " << tagBank->numActiveSubarrayPerColumn << "x" << tagBank->numActiveSubarrayPerRow << " Subarrays" << std::endl; \
				std::cout << "Mux Degree - " << tagBank->muxSenseAmp << " x " << tagBank->muxOutputLev1 << " x " << tagBank->muxOutputLev2 << std::endl; \
				std::cout << "ERROR: DATA capacity violation. Shouldn't happen" << std::endl; \
				exit(-1); \
			} \
}


#define REDUCE_SEARCH_SIZE { \
	gInputParameter.minNumRowMat = 1; \
	gInputParameter.maxNumRowMat = 64; \
	gInputParameter.minNumColumnMat = 1; \
	gInputParameter.maxNumColumnMat = 64; \
	gInputParameter.minNumActiveMatPerRow = 1; \
	gInputParameter.maxNumActiveMatPerRow = gInputParameter.maxNumColumnMat; \
	gInputParameter.minNumActiveMatPerColumn = 1; \
	gInputParameter.maxNumActiveMatPerColumn = gInputParameter.maxNumRowMat; \
	gInputParameter.minNumRowSubarray = 1; \
	gInputParameter.maxNumRowSubarray = 2; \
	gInputParameter.minNumColumnSubarray = 1; \
	gInputParameter.maxNumColumnSubarray = 2; \
	gInputParameter.minNumActiveSubarrayPerRow = 1; \
	gInputParameter.maxNumActiveSubarrayPerRow = gInputParameter.maxNumColumnSubarray; \
	gInputParameter.minNumActiveSubarrayPerColumn = 1; \
	gInputParameter.maxNumActiveSubarrayPerColumn = gInputParameter.maxNumRowSubarray; \
	gInputParameter.minMuxSenseAmp = 1; \
	gInputParameter.maxMuxSenseAmp = 64; \
	gInputParameter.minMuxOutputLev1 = 1; \
	gInputParameter.maxMuxOutputLev1 = 64; \
	gInputParameter.minMuxOutputLev2 = 1; \
	gInputParameter.maxMuxOutputLev2 = 64; \
	gInputParameter.minNumRowPerSet = 1; \
	gInputParameter.maxNumRowPerSet = 1; \
	gInputParameter.minAreaOptimizationLevel = latency_first; \
	gInputParameter.maxAreaOptimizationLevel = area_first;	\
	gInputParameter.minLocalWireType = local_aggressive; \
	gInputParameter.maxLocalWireType = local_conservative; \
	gInputParameter.minGlobalWireType = global_aggressive; \
	gInputParameter.maxGlobalWireType = global_conservative; \
	gInputParameter.minLocalWireRepeaterType = repeated_none; \
	gInputParameter.maxLocalWireRepeaterType = repeated_opt; \
	gInputParameter.minGlobalWireRepeaterType = repeated_none; \
	gInputParameter.maxGlobalWireRepeaterType = repeated_opt; \
	gInputParameter.minIsLocalWireLowSwing = false; \
	gInputParameter.maxIsLocalWireLowSwing = true; \
	gInputParameter.minIsGlobalWireLowSwing = false; \
	gInputParameter.maxIsGlobalWireLowSwing = true; \
}


#define RESTORE_SEARCH_SIZE { \
	gInputParameter.minNumRowMat = 1; \
	gInputParameter.maxNumRowMat = 512; \
	gInputParameter.minNumColumnMat = 1; \
	gInputParameter.maxNumColumnMat = 512; \
	gInputParameter.minNumActiveMatPerRow = 1; \
	gInputParameter.maxNumActiveMatPerRow = gInputParameter.maxNumColumnMat; \
	gInputParameter.minNumActiveMatPerColumn = 1; \
	gInputParameter.maxNumActiveMatPerColumn = gInputParameter.maxNumRowMat; \
	gInputParameter.minNumRowSubarray = 1; \
	gInputParameter.maxNumRowSubarray = 2; \
	gInputParameter.minNumColumnSubarray = 1; \
	gInputParameter.maxNumColumnSubarray = 2; \
	gInputParameter.minNumActiveSubarrayPerRow = 1; \
	gInputParameter.maxNumActiveSubarrayPerRow = gInputParameter.maxNumColumnSubarray; \
	gInputParameter.minNumActiveSubarrayPerColumn = 1; \
	gInputParameter.maxNumActiveSubarrayPerColumn = gInputParameter.maxNumRowSubarray; \
	gInputParameter.minMuxSenseAmp = 1; \
	gInputParameter.maxMuxSenseAmp = 256; \
	gInputParameter.minMuxOutputLev1 = 1; \
	gInputParameter.maxMuxOutputLev1 = 256; \
	gInputParameter.minMuxOutputLev2 = 1; \
	gInputParameter.maxMuxOutputLev2 = 256; \
	gInputParameter.minNumRowPerSet = 1; \
	gInputParameter.maxNumRowPerSet = gInputParameter.associativity; \
	gInputParameter.minAreaOptimizationLevel = latency_first; \
	gInputParameter.maxAreaOptimizationLevel = area_first; \
	gInputParameter.minLocalWireType = local_aggressive; \
	gInputParameter.maxLocalWireType = semi_conservative; \
	gInputParameter.minGlobalWireType = semi_aggressive; \
	gInputParameter.maxGlobalWireType = global_conservative; \
	gInputParameter.minLocalWireRepeaterType = repeated_none; \
	gInputParameter.maxLocalWireRepeaterType = repeated_50;		/* The limit is repeated_50 */ \
	gInputParameter.minGlobalWireRepeaterType = repeated_none; \
	gInputParameter.maxGlobalWireRepeaterType = repeated_50;	/* The limit is repeated_50 */ \
	gInputParameter.minIsLocalWireLowSwing = false; \
	gInputParameter.maxIsLocalWireLowSwing = true; \
	gInputParameter.minIsGlobalWireLowSwing = false; \
	gInputParameter.maxIsGlobalWireLowSwing = true; \
}


#define APPLY_LIMIT(result) { \
	(result).reset(); \
	(result).limitReadLatency = allowedDataReadLatency; \
	(result).limitWriteLatency = allowedDataWriteLatency; \
	(result).limitReadDynamicEnergy = allowedDataReadDynamicEnergy; \
	(result).limitWriteDynamicEnergy = allowedDataWriteDynamicEnergy; \
	(result).limitReadEdp = allowedDataReadEdp; \
	(result).limitWriteEdp = allowedDataWriteEdp; \
	(result).limitArea = allowedDataArea; \
	(result).limitLeakage = allowedDataLeakage; \
}


#define OUTPUT_TO_FILE { \
	if (gInputParameter.designTarget == cache) { \
		tempResult.printAsCacheToYamlFile(bestTagResults[0], gInputParameter.cacheAccessMode, outputFile); \
	} else { \
		tempResult.printToYamlFile(outputFile); \
	} \
}

#define TO_SECOND(x) \
	((x) < 1e-9 ? (x) * 1e12 : (x) < 1e-6 ? (x) * 1e9 : (x) < 1e-3 ? (x) * 1e6 : (x) < 1 ? (x) * 1e3 : (x)) \
	<< \
	((x) < 1e-9 ? "ps" : (x) < 1e-6 ? "ns" : (x) < 1e-3 ? "us" : (x) < 1 ? "ms" : "s")

#define TO_BPS(x) \
	((x) < 1e3 ? (x) : (x) < 1e6 ? (x) / 1e3 : (x) < 1e9 ? (x) / 1e6 : (x) < 1e12 ? (x) / 1e9 : (x) / 1e12) \
	<< \
	((x) < 1e3 ? "B/s" : (x) < 1e6 ? "KB/s" : (x) < 1e9 ? "MB/s" : (x) < 1e12 ? "GB/s" : "TB/s")

#define TO_JOULE(x) \
	((x) < 1e-9 ? (x) * 1e12 : (x) < 1e-6 ? (x) * 1e9 : (x) < 1e-3 ? (x) * 1e6 : (x) < 1 ? (x) * 1e3 : (x)) \
	<< \
	((x) < 1e-9 ? "pJ" : (x) < 1e-6 ? "nJ" : (x) < 1e-3 ? "uJ" : (x) < 1 ? "mJ" : "J")

#define TO_WATT(x) \
	((x) < 1e-9 ? (x) * 1e12 : (x) < 1e-6 ? (x) * 1e9 : (x) < 1e-3 ? (x) * 1e6 : (x) < 1 ? (x) * 1e3 : (x)) \
	<< \
	((x) < 1e-9 ? "pW" : (x) < 1e-6 ? "nW" : (x) < 1e-3 ? "uW" : (x) < 1 ? "mW" : "W")

#define TO_METER(x) \
	((x) < 1e-9 ? (x) * 1e12 : (x) < 1e-6 ? (x) * 1e9 : (x) < 1e-3 ? (x) * 1e6 : (x) < 1 ? (x) * 1e3 : (x)) \
	<< \
	((x) < 1e-9 ? "pm" : (x) < 1e-6 ? "nm" : (x) < 1e-3 ? "um" : (x) < 1 ? "mm" : "m")

#define TO_SQM(x) \
	((x) < 1e-12 ? (x) * 1e18 : (x) < 1e-6 ? (x) * 1e12 : (x) < 1 ? (x) * 1e6 : (x)) \
	<< \
	((x) < 1e-12 ? "nm^2" : (x) < 1e-6 ? "um^2" : (x) < 1 ? "mm^2" : "m^2")

#endif /* MSE_MACROS_H */
