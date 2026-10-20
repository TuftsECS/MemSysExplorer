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


#ifndef MSE_MACROS_HPP
#define MSE_MACROS_HPP


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



// A trial bank is not checked against the retention time
#define TRY_AND_UPDATE(oldResult, memoryType) { \
	std::shared_ptr<mse::component::Bank> trialBank = makeBank((oldResult).bank->matRows, (oldResult).bank->matColumns, (oldResult).bank->capacity, \
				(oldResult).bank->blockSize, (oldResult).bank->associativity, (oldResult).bank->rowsPerSet, (oldResult).bank->activeMatsPerRow, \
				(oldResult).bank->activeMatsPerColumn, (oldResult).bank->bitlineMuxInputs, gInputParameter.internalSensing, \
				(oldResult).bank->senseAmpMuxLevel1Inputs, (oldResult).bank->senseAmpMuxLevel2Inputs, (oldResult).bank->subarrayRows, \
				(oldResult).bank->subarrayColumns, (oldResult).bank->activeSubarraysPerRow, (oldResult).bank->activeSubarraysPerColumn, \
				(oldResult).bank->areaOptimizationLevel, (memoryType)); \
	if (calculateBank(*trialBank)) { \
		tempResult.bank = trialBank; \
		*(tempResult.localWire) = gLocalWire; \
		*(tempResult.globalWire) = gGlobalWire; \
		oldResult.compareAndUpdate(tempResult); \
	} \
}


#define BIGFOR \
	for (numRowMat = gInputParameter.minNumRowMat; numRowMat <= gInputParameter.maxNumRowMat; numRowMat *= 2) \
	for (numColumnMat = gInputParameter.minNumColumnMat; numColumnMat <= gInputParameter.maxNumColumnMat; numColumnMat *= 2) \
	for (numActiveMatPerRow = std::min(numColumnMat, gInputParameter.minNumActiveMatPerRow); numActiveMatPerRow <= std::min(numColumnMat, gInputParameter.maxNumActiveMatPerRow); numActiveMatPerRow *= 2) \
	for (numActiveMatPerColumn = std::min(numRowMat, gInputParameter.minNumActiveMatPerColumn); numActiveMatPerColumn <= std::min(numRowMat, gInputParameter.maxNumActiveMatPerColumn); numActiveMatPerColumn *= 2) \
	for (numRowSubarray = gInputParameter.minNumRowSubarray; numRowSubarray <= gInputParameter.maxNumRowSubarray; numRowSubarray *= 2) \
	for (numColumnSubarray = gInputParameter.minNumColumnSubarray; numColumnSubarray <= gInputParameter.maxNumColumnSubarray; numColumnSubarray *= 2) \
	for (numActiveSubarrayPerRow = std::min(numColumnSubarray, gInputParameter.minNumActiveSubarrayPerRow); numActiveSubarrayPerRow <= std::min(numColumnSubarray, gInputParameter.maxNumActiveSubarrayPerRow); numActiveSubarrayPerRow *=2) \
	for (numActiveSubarrayPerColumn = std::min(numRowSubarray, gInputParameter.minNumActiveSubarrayPerColumn); numActiveSubarrayPerColumn <= std::min(numRowSubarray, gInputParameter.maxNumActiveSubarrayPerColumn); numActiveSubarrayPerColumn *= 2) \
	for (muxSenseAmp = gInputParameter.minMuxSenseAmp; muxSenseAmp <= gInputParameter.maxMuxSenseAmp; muxSenseAmp *= 2) \
	for (muxOutputLev1 = gInputParameter.minMuxOutputLev1; muxOutputLev1 <= gInputParameter.maxMuxOutputLev1; muxOutputLev1 *= 2) \
	for (muxOutputLev2 = gInputParameter.minMuxOutputLev2; muxOutputLev2 <= gInputParameter.maxMuxOutputLev2; muxOutputLev2 *= 2) \
	for (numRowPerSet = gInputParameter.minNumRowPerSet; numRowPerSet <= std::min(gInputParameter.maxNumRowPerSet, gInputParameter.associativity); numRowPerSet *= 2) \
	for (areaOptimizationLevel = gInputParameter.minAreaOptimizationLevel; areaOptimizationLevel <= gInputParameter.maxAreaOptimizationLevel; areaOptimizationLevel++)



// Builds and evaluates a bank for the current organization, and is true when the bank is valid
#define CALCULATE(bank, memoryType) \
	(((bank) = makeBank(numRowMat, numColumnMat, capacity, blockSize, associativity, \
				numRowPerSet, numActiveMatPerRow, numActiveMatPerColumn, muxSenseAmp, \
				gInputParameter.internalSensing, muxOutputLev1, muxOutputLev2, numRowSubarray, numColumnSubarray, \
				numActiveSubarrayPerRow, numActiveSubarrayPerColumn, (BufferDesignTarget)areaOptimizationLevel, (memoryType))), \
		calculateBank(*(bank)) && (bank)->meetsRetention())


#define UPDATE_BEST_DATA { \
	tempResult.bank = dataBank; \
	*(tempResult.localWire) = gLocalWire; \
	*(tempResult.globalWire) = gGlobalWire; \
	for (int i = 0; i < (int)full_exploration; i++) \
		bestDataResults[i].compareAndUpdate(tempResult); \
}


#define VERIFY_DATA_CAPACITY { \
	if ((long long)dataBank->mat.subarray.columns * dataBank->mat.subarray.rows * dataBank->matColumns * \
			dataBank->matRows * dataBank->subarrayColumns * dataBank->subarrayRows != capacity) { \
				std::cout << "1 Bank = " << dataBank->matRows << "x" << dataBank->matColumns << " Mats" << std::endl; \
				std::cout << "Activation - " << dataBank->activeMatsPerColumn << "x" << dataBank->activeMatsPerRow << " Mats" << std::endl; \
				std::cout << "1 Mat  = " << dataBank->subarrayRows << "x" << dataBank->subarrayColumns << " Subarrays" << std::endl; \
				std::cout << "Activation - " << dataBank->activeSubarraysPerColumn << "x" << dataBank->activeSubarraysPerRow << " Subarrays" << std::endl; \
				std::cout << "Mux Degree - " << dataBank->bitlineMuxInputs << " x " << dataBank->senseAmpMuxLevel1Inputs << " x " << dataBank->senseAmpMuxLevel2Inputs << std::endl; \
				std::cout << "ERROR: DATA capacity violation. Shouldn't happen" << std::endl; \
				exit(-1); \
			} \
}


#define UPDATE_BEST_TAG { \
	tempResult.bank = tagBank; \
	*(tempResult.localWire) = gLocalWire; \
	*(tempResult.globalWire) = gGlobalWire; \
	for (int i = 0; i < (int)full_exploration; i++) \
		bestTagResults[i].compareAndUpdate(tempResult); \
}


#define VERIFY_TAG_CAPACITY { \
	if ((long long)tagBank->mat.subarray.columns * tagBank->mat.subarray.rows * tagBank->matColumns * \
			tagBank->matRows * tagBank->subarrayColumns * tagBank->subarrayRows != capacity) { \
				std::cout << "1 Bank = " << tagBank->matRows << "x" << tagBank->matColumns << " Mats" << std::endl; \
				std::cout << "Activation - " << tagBank->activeMatsPerColumn << "x" << tagBank->activeMatsPerRow << " Mats" << std::endl; \
				std::cout << "1 Mat  = " << tagBank->subarrayRows << "x" << tagBank->subarrayColumns << " Subarrays" << std::endl; \
				std::cout << "Activation - " << tagBank->activeSubarraysPerColumn << "x" << tagBank->activeSubarraysPerRow << " Subarrays" << std::endl; \
				std::cout << "Mux Degree - " << tagBank->bitlineMuxInputs << " x " << tagBank->senseAmpMuxLevel1Inputs << " x " << tagBank->senseAmpMuxLevel2Inputs << std::endl; \
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

#endif /* MSE_MACROS_HPP */
