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
#include "MemCell.hpp"
#include "Technology.hpp"
#include "component/BankWithHtree.hpp"
#include "component/BankWithoutHtree.hpp"
#include "Wire.hpp"
#include "Result.hpp"
#include "formula.hpp"
#include "macros.hpp"
#include "cell/types.hpp"
#include "input.hpp"
#include "flags.hpp"

#include <bit>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <math.h>

InputParameter gInputParameter;
Technology gTech;
Technology gTechW;
Technology gTechR;
MemCell gCell;
Wire gLocalWire;
Wire gGlobalWire;

void applyConstraint();

// Builds the bank for the configured routing mode
static std::shared_ptr<mse::component::Bank> makeBank(int matRows, int matColumns, long long capacity, long blockSize, int associativity,
		int rowsPerSet, int activeMatsPerRow, int activeMatsPerColumn, int bitlineMuxInputs, bool internalSenseAmp,
		int senseAmpMuxLevel1Inputs, int senseAmpMuxLevel2Inputs, int subarrayRows, int subarrayColumns,
		int activeSubarraysPerRow, int activeSubarraysPerColumn, BufferDesignTarget areaOptimizationLevel, MemoryType memoryType) {
	if (gInputParameter.routingMode == h_tree) {
		return std::make_shared<mse::component::BankWithHtree>(matRows, matColumns, capacity, blockSize, associativity, rowsPerSet,
				activeMatsPerRow, activeMatsPerColumn, bitlineMuxInputs, internalSenseAmp, senseAmpMuxLevel1Inputs, senseAmpMuxLevel2Inputs,
				subarrayRows, subarrayColumns, activeSubarraysPerRow, activeSubarraysPerColumn, areaOptimizationLevel, memoryType);
	}
	return std::make_shared<mse::component::BankWithoutHtree>(matRows, matColumns, capacity, blockSize, associativity, rowsPerSet,
			activeMatsPerRow, activeMatsPerColumn, bitlineMuxInputs, internalSenseAmp, senseAmpMuxLevel1Inputs, senseAmpMuxLevel2Inputs,
			subarrayRows, subarrayColumns, activeSubarraysPerRow, activeSubarraysPerColumn, areaOptimizationLevel, memoryType);
}

// Sets up a bank and evaluates it right away, since the wire globals change between candidates
static bool calculateBank(mse::component::Bank& bank) {
	if (!bank.setup()) {
		return false;
	}
	bank.dimensions();
	bank.timing();
	bank.power();
	return true;
}

int main(int argc, char* argv[])
{
	std::cout << std::fixed << std::setprecision(3);

    //==================================================
    // Flag parsing

	mse::flags::parseFlags(argc, argv);

	// The config file defaults to nvsim.cfg when no positional gives one
	const std::string configFile(mse::flags::settings.configFile.value_or("nvsim.cfg"));
	if (mse::flags::settings.configFile) {
		std::cout << "User-defined configuration file (" << configFile << ") is loaded\n";
	} else {
		std::cout << "Default configuration file (" << configFile << ") is loaded\n";
	}
	std::cout << "\n";

    //==================================================

	RESTORE_SEARCH_SIZE;
	gInputParameter.ReadInputParameterFromFile(configFile);

	gCell.ReadCellFromFile(gInputParameter.fileMemCell);

	// FeFET cells change the technologies' gate capacitance, so the type is read before the cell can be created
	mse::YamlInputFile cellYamlFile(gInputParameter.fileMemCell);
	mse::ScalarParameter<std::string> cellName{"MemCellType"};
	cellYamlFile.fillParameter(cellName);
	const bool is_FeFET = cellName.value() == mse::cell::FefetCell::name;
	gTech.Initialize(gInputParameter.processNode, gInputParameter.deviceRoadmap, is_FeFET);

	gTechR.Initialize(gInputParameter.processNodeR, gInputParameter.deviceRoadmapR, is_FeFET);

	/* Interpolation for techR */
	Technology techHighR;
	double alphaR = 0;
	if (gInputParameter.processNodeR > 200){
		// TODO: technology node > 200 nm
	} else if (gInputParameter.processNodeR > 120) { // 120 nm < technology node <= 200 nm
		techHighR.Initialize(200, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 120.0) / 60;
	} else if (gInputParameter.processNodeR > 90) { // 90 nm < technology node <= 120 nm
		techHighR.Initialize(120, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 90.0) / 30;
	} else if (gInputParameter.processNodeR > 65) { // 65 nm < technology node <= 90 nm
		techHighR.Initialize(90, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 65.0) / 25;
	} else if (gInputParameter.processNodeR > 45) { // 45 nm < technology node <= 65 nm
		techHighR.Initialize(65, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 45.0) / 20;
	} else if (gInputParameter.processNodeR >= 32) { // 32 nm < technology node <= 45 nm
		techHighR.Initialize(45, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 32.0) / 13;
	} else if (gInputParameter.processNodeR >= 22) { // 22 nm < technology node <= 32 nm
		techHighR.Initialize(32, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 22.0) / 10;
	} else if (gInputParameter.processNodeR >= 14) {         // 14 nm < node < 22 nm
		techHighR.Initialize(22, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 14.0) / 8;
	} else if (gInputParameter.processNodeR >= 10) {  // 10 nm ≤ node < 14 nm
		techHighR.Initialize(14, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 10.0) / 4;
	} else if (gInputParameter.processNodeR >= 7) {
		techHighR.Initialize(10, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 7.0) / 3;
	} else if (gInputParameter.processNodeR >= 5) {
		techHighR.Initialize(7, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 5.0) / 2;
	} else if (gInputParameter.processNodeR >= 3) {
		techHighR.Initialize(5, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 3.0) / 2;
	} else if (gInputParameter.processNodeR >= 2) {
		techHighR.Initialize(3, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 2.0) / 1;
	} else if (gInputParameter.processNodeR >= 1) {
		techHighR.Initialize(2, gInputParameter.deviceRoadmapR, is_FeFET);
		alphaR = (gInputParameter.processNodeR - 1.0) / 1;
	} else {
		// TODO no technology model below 1 nm
		std::cout << "Technology node below 1 nm is not supported!" << std::endl;
		exit(1);
	}
	gTechR.InterpolateWith(techHighR, alphaR);

	gTechW.Initialize(gInputParameter.processNodeW, gInputParameter.deviceRoadmapW, is_FeFET);

	/* Interpolation for techW */
	Technology techHighW;
	double alphaW = 0;
	if (gInputParameter.processNodeW > 200){
		// TODO: technology node > 200 nm
	} else if (gInputParameter.processNodeW > 120) { // 120 nm < technology node <= 200 nm
		techHighW.Initialize(200, gInputParameter.deviceRoadmapW, is_FeFET);
		alphaW = (gInputParameter.processNodeW - 120.0) / 60;
	} else if (gInputParameter.processNodeW > 90) { // 90 nm < technology node <= 120 nm
		techHighW.Initialize(120, gInputParameter.deviceRoadmapW, is_FeFET);
		alphaW = (gInputParameter.processNodeW - 90.0) / 30;
	} else if (gInputParameter.processNodeW > 65) { // 65 nm < technology node <= 90 nm
		techHighW.Initialize(90, gInputParameter.deviceRoadmapW, is_FeFET);
		alphaW = (gInputParameter.processNodeW - 65.0) / 25;
	} else if (gInputParameter.processNodeW > 45) { // 45 nm < technology node <= 65 nm
		techHighW.Initialize(65, gInputParameter.deviceRoadmapW, is_FeFET);
		alphaW = (gInputParameter.processNodeW - 45.0) / 20;
	} else if (gInputParameter.processNodeW >= 32) { // 32 nm < technology node <= 45 nm
		techHighW.Initialize(45, gInputParameter.deviceRoadmapW, is_FeFET);
		alphaW = (gInputParameter.processNodeW - 32.0) / 13;
	} else if (gInputParameter.processNodeW >= 22) { // 22 nm < technology node <= 32 nm
		techHighW.Initialize(32, gInputParameter.deviceRoadmapW, is_FeFET);
		alphaW = (gInputParameter.processNodeW - 22.0) / 10;
	} else if (gInputParameter.processNodeW >= 10) {  // 10 nm ≤ node < 14 nm
		techHighW.Initialize(14, gInputParameter.deviceRoadmapW, is_FeFET);
		alphaW = (gInputParameter.processNodeW - 10.0) / 4;
	} else if (gInputParameter.processNodeW >= 7) {
		techHighW.Initialize(10, gInputParameter.deviceRoadmapW, is_FeFET);
		alphaW = (gInputParameter.processNodeW - 7.0) / 3;
	} else if (gInputParameter.processNodeW >= 5) {
		techHighW.Initialize(7, gInputParameter.deviceRoadmapW, is_FeFET);
		alphaW = (gInputParameter.processNodeW - 5.0) / 2;
	} else if (gInputParameter.processNodeW >= 3) {
		techHighW.Initialize(5, gInputParameter.deviceRoadmapW, is_FeFET);
		alphaW = (gInputParameter.processNodeW - 3.0) / 2;
	} else if (gInputParameter.processNodeW >= 2) {
		techHighW.Initialize(3, gInputParameter.deviceRoadmapW, is_FeFET);
		alphaW = (gInputParameter.processNodeW - 2.0) / 1;
	} else if (gInputParameter.processNodeW >= 1) {
		techHighW.Initialize(2, gInputParameter.deviceRoadmapW, is_FeFET);
		alphaW = (gInputParameter.processNodeW - 1.0) / 1;
	} else {
		// TODO no technology model below 1 nm
		std::cout << "Technology node below 1 nm is not supported!" << std::endl;
		exit(1);
	}
	gTechW.InterpolateWith(techHighW, alphaW);

	Technology techHigh;
	double alpha = 0;
	if (gInputParameter.processNode > 200){
		// TODO: technology node > 200 nm
	} else if (gInputParameter.processNode > 120) { // 120 nm < technology node <= 200 nm
		techHigh.Initialize(200, gInputParameter.deviceRoadmap, is_FeFET);
		alpha = (gInputParameter.processNode - 120.0) / 60;
	} else if (gInputParameter.processNode > 90) { // 90 nm < technology node <= 120 nm
		techHigh.Initialize(120, gInputParameter.deviceRoadmap, is_FeFET);
		alpha = (gInputParameter.processNode - 90.0) / 30;
	} else if (gInputParameter.processNode > 65) { // 65 nm < technology node <= 90 nm
		techHigh.Initialize(90, gInputParameter.deviceRoadmap, is_FeFET);
		alpha = (gInputParameter.processNode - 65.0) / 25;
	} else if (gInputParameter.processNode > 45) { // 45 nm < technology node <= 65 nm
		techHigh.Initialize(65, gInputParameter.deviceRoadmap, is_FeFET);
		alpha = (gInputParameter.processNode - 45.0) / 20;
	} else if (gInputParameter.processNode >= 32) { // 32 nm < technology node <= 45 nm
		techHigh.Initialize(45, gInputParameter.deviceRoadmap, is_FeFET);
		alpha = (gInputParameter.processNode - 32.0) / 13;
	} else if (gInputParameter.processNode >= 22) { // 22 nm < technology node <= 32 nm
		techHigh.Initialize(32, gInputParameter.deviceRoadmap, is_FeFET);
		alpha = (gInputParameter.processNode - 22.0) / 10;
	} else if (gInputParameter.processNode >= 10) {  // 10 nm ≤ node < 14 nm
		techHigh.Initialize(14, gInputParameter.deviceRoadmap, is_FeFET);
		alpha = (gInputParameter.processNode - 10.0) / 4;
	} else if (gInputParameter.processNode >= 7) {
		techHigh.Initialize(10, gInputParameter.deviceRoadmap, is_FeFET);
		alpha = (gInputParameter.processNode - 7.0) / 3;
	} else if (gInputParameter.processNode >= 5) {
		techHigh.Initialize(7, gInputParameter.deviceRoadmap, is_FeFET);
		alpha = (gInputParameter.processNode - 5.0) / 2;
	} else if (gInputParameter.processNode >= 3) {
		techHigh.Initialize(5, gInputParameter.deviceRoadmap, is_FeFET);
		alpha = (gInputParameter.processNode - 3.0) / 2;
	} else if (gInputParameter.processNode >= 2) {
		techHigh.Initialize(3, gInputParameter.deviceRoadmap, is_FeFET);
		alpha = (gInputParameter.processNode - 2.0) / 1;
	} else if (gInputParameter.processNode >= 1) {
		techHigh.Initialize(2, gInputParameter.deviceRoadmap, is_FeFET);
		alpha = (gInputParameter.processNode - 1.0) / 1;
	} else {
		// TODO no technology model below 1 nm
		std::cout << "Technology node below 1 nm is not supported!" << std::endl;
		exit(1);
	}

	gTech.InterpolateWith(techHigh, alpha);

	gCell.ApplyPVT(); // must apply PVT after tech initialization

	std::ofstream outputFile;
	std::string outputFileName;
	if (gInputParameter.optimizationTarget == full_exploration) {
		std::stringstream temp;
		temp << gInputParameter.outputFilePrefix << "_" << gInputParameter.capacity / 1024 << "K_" << gInputParameter.wordWidth
				<< "_" << gInputParameter.associativity;
		if (gInputParameter.internalSensing)
			temp << "_IN";
		else
			temp << "_EX";
		if (gCell.readMode)
			temp << "_VOL";
		else
			temp << "_CUR";
		temp << ".yaml";
		outputFileName = temp.str();
		outputFile.open(outputFileName.c_str(), std::ofstream::trunc);
	}

	mse::cell::MemoryCellFactory<>::create(cellYamlFile, "MemCellType", mse::cell::runCell);

	gCell.PrintCell();

	applyConstraint();

	int numRowMat, numColumnMat, numActiveMatPerRow, numActiveMatPerColumn;
	int numRowSubarray, numColumnSubarray, numActiveSubarrayPerRow, numActiveSubarrayPerColumn;
	int muxSenseAmp, muxOutputLev1, muxOutputLev2, numRowPerSet;
	int areaOptimizationLevel;							/* actually BufferDesignTarget */
	int localWireType, globalWireType;					/* actually WireType */
	int localWireRepeaterType, globalWireRepeaterType;	/* actually WireRepeaterType */
	int isLocalWireLowSwing, isGlobalWireLowSwing;		/* actually boolean value */

	long long capacity;
	long blockSize;
	int associativity;

	/* for cache data array, memory array */
	Result bestDataResults[(int)full_exploration];	/* full_exploration is always set as the last element in the enum, so if full_exploration is 8, what we want here is a 0-7 array, which is correct */
	std::shared_ptr<mse::component::Bank> dataBank;
	for (int i = 0; i < (int)full_exploration; i++)
		bestDataResults[i].optimizationTarget = (OptimizationTarget)i;

	/* for cache tag array only */
	Result bestTagResults[(int)full_exploration];	/* full_exploration is always set as the last element in the enum, so if full_exploration is 8, what we want here is a 0-7 array, which is correct */
	for (int i = 0; i < (int)full_exploration; i++)
		bestTagResults[i].optimizationTarget = (OptimizationTarget)i;

	long long numSolution = 0;

	gInputParameter.PrintInputParameter();

	/* search tag first */
	if (gInputParameter.designTarget == cache) {
		/* need to design the tag array */
		REDUCE_SEARCH_SIZE;
		/* calculate the tag configuration */
		int numDataSet = gInputParameter.capacity * 8 / gInputParameter.wordWidth / gInputParameter.associativity;
		int numIndexBit = (int)(log2(numDataSet) + 0.1);
		int numOffsetBit = (int)(log2(gInputParameter.wordWidth / 8) + 0.1);
		INITIAL_BASIC_WIRE;
		/* Simulate tag */
		// BIGFOR
		#pragma omp parallel firstprivate(gInputParameter, gTech, gTechR, gTechW, gCell, gLocalWire, gGlobalWire)
		{
			// These parameters need to be declared here so that they are treated as firstprivate by OpenMP.
			int numRowMat, numColumnMat, numActiveMatPerRow, numActiveMatPerColumn;
			int numRowSubarray, numColumnSubarray, numActiveSubarrayPerRow, numActiveSubarrayPerColumn;
			int muxSenseAmp, muxOutputLev1, muxOutputLev2, numRowPerSet;
			int areaOptimizationLevel;							/* actually BufferDesignTarget */

			#pragma omp single
			for (numRowMat = gInputParameter.minNumRowMat; numRowMat <= gInputParameter.maxNumRowMat; numRowMat *= 2)
			for (numColumnMat = gInputParameter.minNumColumnMat; numColumnMat <= gInputParameter.maxNumColumnMat; numColumnMat *= 2)
			for (numActiveMatPerRow = std::min(numColumnMat, gInputParameter.minNumActiveMatPerRow); numActiveMatPerRow <= std::min(numColumnMat, gInputParameter.maxNumActiveMatPerRow); numActiveMatPerRow *= 2)
			for (numActiveMatPerColumn = std::min(numRowMat, gInputParameter.minNumActiveMatPerColumn); numActiveMatPerColumn <= std::min(numRowMat, gInputParameter.maxNumActiveMatPerColumn); numActiveMatPerColumn *= 2)
			for (numRowSubarray = gInputParameter.minNumRowSubarray; numRowSubarray <= gInputParameter.maxNumRowSubarray; numRowSubarray *= 2)
			for (numColumnSubarray = gInputParameter.minNumColumnSubarray; numColumnSubarray <= gInputParameter.maxNumColumnSubarray; numColumnSubarray *= 2) {
				#pragma omp task
				{
					Result bestTagResultsTask[full_exploration];
					for (int i = 0; i < (int)full_exploration; i++)
						bestTagResultsTask[i].optimizationTarget = (OptimizationTarget)i;
					long long numSolutionTask = 0;

					for (numActiveSubarrayPerRow = std::min(numColumnSubarray, gInputParameter.minNumActiveSubarrayPerRow); numActiveSubarrayPerRow <= std::min(numColumnSubarray, gInputParameter.maxNumActiveSubarrayPerRow); numActiveSubarrayPerRow *=2)
					for (numActiveSubarrayPerColumn = std::min(numRowSubarray, gInputParameter.minNumActiveSubarrayPerColumn); numActiveSubarrayPerColumn <= std::min(numRowSubarray, gInputParameter.maxNumActiveSubarrayPerColumn); numActiveSubarrayPerColumn *= 2)
					for (muxSenseAmp = gInputParameter.minMuxSenseAmp; muxSenseAmp <= gInputParameter.maxMuxSenseAmp; muxSenseAmp *= 2)
					for (muxOutputLev1 = gInputParameter.minMuxOutputLev1; muxOutputLev1 <= gInputParameter.maxMuxOutputLev1; muxOutputLev1 *= 2)
					for (muxOutputLev2 = gInputParameter.minMuxOutputLev2; muxOutputLev2 <= gInputParameter.maxMuxOutputLev2; muxOutputLev2 *= 2)
					for (numRowPerSet = gInputParameter.minNumRowPerSet; numRowPerSet <= std::min(gInputParameter.maxNumRowPerSet, gInputParameter.associativity); numRowPerSet *= 2)
					for (areaOptimizationLevel = gInputParameter.minAreaOptimizationLevel; areaOptimizationLevel <= gInputParameter.maxAreaOptimizationLevel; areaOptimizationLevel++) {
						long blockSize = mse::constant::totalAddressBit - numIndexBit - numOffsetBit;
						blockSize += 2;		/* add dirty bits and valid bits */
						if (blockSize / (numActiveMatPerRow * numActiveMatPerColumn * numActiveSubarrayPerRow * numActiveSubarrayPerColumn) == 0) {
							/* To aggressive partitioning */
							continue;
						}
						if (blockSize % (numActiveMatPerRow * numActiveMatPerColumn * numActiveSubarrayPerRow * numActiveSubarrayPerColumn)) {
							blockSize = (blockSize / (numActiveMatPerRow * numActiveMatPerColumn * numActiveSubarrayPerRow * numActiveSubarrayPerColumn) + 1)
									* (numActiveMatPerRow * numActiveMatPerColumn * numActiveSubarrayPerRow * numActiveSubarrayPerColumn);
						}
						long long capacity = (long long)gInputParameter.capacity * 8 / gInputParameter.wordWidth * blockSize;
						int associativity = gInputParameter.associativity;
						std::shared_ptr<mse::component::Bank> tagBank;
						if (CALCULATE(tagBank, tag)) {
							Result tempResult;
							VERIFY_TAG_CAPACITY;
							numSolutionTask++;
							// UPDATE_BEST_TAG;
							tempResult.bank = tagBank;
							*(tempResult.localWire) = gLocalWire;
							*(tempResult.globalWire) = gGlobalWire;
							for (int i = 0; i < (int)full_exploration; i++)
								bestTagResultsTask[i].compareAndUpdate(tempResult);
						}
					}

					#pragma omp critical
					{
						for (int i = 0; i < full_exploration; i++) {
							bestTagResults[i].compareAndUpdate(bestTagResultsTask[i]);
						}
						numSolution += numSolutionTask;
					}
				}
			}
		}
		if (numSolution > 0) {
			Result tempResult;
			/* refine local wire type */
			REFINE_LOCAL_WIRE_FORLOOP {
			    gLocalWire.Initialize(gInputParameter.processNode, (WireType)localWireType,
						(WireRepeaterType)localWireRepeaterType, gInputParameter.temperature,
						(bool)isLocalWireLowSwing);
				for (int i = 0; i < (int)full_exploration; i++) {
					LOAD_GLOBAL_WIRE(bestTagResults[i]);
					TRY_AND_UPDATE(bestTagResults[i], tag);
				}
			}
			/* refine global wire type */
			REFINE_GLOBAL_WIRE_FORLOOP {
				gGlobalWire.Initialize(gInputParameter.processNode, (WireType)globalWireType,
						(WireRepeaterType)globalWireRepeaterType, gInputParameter.temperature,
						(bool)isGlobalWireLowSwing);
				for (int i = 0; i < (int)full_exploration; i++) {
					LOAD_LOCAL_WIRE(bestTagResults[i]);
					TRY_AND_UPDATE(bestTagResults[i], tag);
				}
			}
		}

		if (numSolution == 0) {
			std::cout << "No valid solutions for tags." << std::endl;
			std::cout << std::endl << "Finished!" << std::endl;
			outputFile.close();
			return 1;
		} else {
			numSolution = 0;
			RESTORE_SEARCH_SIZE;
			gInputParameter.ReadInputParameterFromFile(configFile);	/* just for restoring the search space */
			applyConstraint();
		}
	}

	/* adjust cache data array parameters according to the access mode */
	capacity = (long long)gInputParameter.capacity * 8;
	blockSize = gInputParameter.wordWidth;
	associativity = gInputParameter.associativity;

	if (gInputParameter.designTarget == cache) {
		switch (gInputParameter.cacheAccessMode) {
			case sequential_access_mode:
				/* already knows which way to access */
				associativity = 1;
				break;
			case fast_access_mode:
				/* load the entire set as a single word */
				blockSize *= associativity;
				associativity = 1;
				break;
			default:	/* Normal */
				/* Normal access does not allow one set be distributed into multiple rows
				 * otherwise, the row activation has to be delayed until the hit signals arrive.
				 */
				gInputParameter.minNumRowPerSet = gInputParameter.maxNumRowPerSet = 1;
		}
	}

	/* adjust block size is it is SLC NAND flash or DRAM memory chip */
	if (gInputParameter.designTarget == RAM_chip && mse::cell::memoryCell().properties().flash) {
		blockSize = gInputParameter.pageSize;
		associativity = 1;
	}

	INITIAL_BASIC_WIRE;
	// BIGFOR
	#pragma omp parallel firstprivate(gInputParameter, gTech, gTechR, gTechW, gCell, gLocalWire, gGlobalWire)
	{
		// These parameters need to be declared here so that they are treated as firstprivate by OpenMP.
		int numRowMat, numColumnMat, numActiveMatPerRow, numActiveMatPerColumn;
		int numRowSubarray, numColumnSubarray, numActiveSubarrayPerRow, numActiveSubarrayPerColumn;
		int muxSenseAmp, muxOutputLev1, muxOutputLev2, numRowPerSet;
		int areaOptimizationLevel;							/* actually BufferDesignTarget */

		#pragma omp single
		for (numRowMat = gInputParameter.minNumRowMat; numRowMat <= gInputParameter.maxNumRowMat; numRowMat *= 2)
		for (numColumnMat = gInputParameter.minNumColumnMat; numColumnMat <= gInputParameter.maxNumColumnMat; numColumnMat *= 2)
		for (numActiveMatPerRow = std::min(numColumnMat, gInputParameter.minNumActiveMatPerRow); numActiveMatPerRow <= std::min(numColumnMat, gInputParameter.maxNumActiveMatPerRow); numActiveMatPerRow *= 2)
		for (numActiveMatPerColumn = std::min(numRowMat, gInputParameter.minNumActiveMatPerColumn); numActiveMatPerColumn <= std::min(numRowMat, gInputParameter.maxNumActiveMatPerColumn); numActiveMatPerColumn *= 2)
		for (numRowSubarray = gInputParameter.minNumRowSubarray; numRowSubarray <= gInputParameter.maxNumRowSubarray; numRowSubarray *= 2)
		for (numColumnSubarray = gInputParameter.minNumColumnSubarray; numColumnSubarray <= gInputParameter.maxNumColumnSubarray; numColumnSubarray *= 2)
			#pragma omp task
			{
				Result bestDataResultsTask[full_exploration];
				for (int i = 0; i < (int)full_exploration; i++)
					bestDataResultsTask[i].optimizationTarget = (OptimizationTarget)i;
				long long numSolutionTask = 0;

				for (numActiveSubarrayPerRow = std::min(numColumnSubarray, gInputParameter.minNumActiveSubarrayPerRow); numActiveSubarrayPerRow <= std::min(numColumnSubarray, gInputParameter.maxNumActiveSubarrayPerRow); numActiveSubarrayPerRow *=2)
				for (numActiveSubarrayPerColumn = std::min(numRowSubarray, gInputParameter.minNumActiveSubarrayPerColumn); numActiveSubarrayPerColumn <= std::min(numRowSubarray, gInputParameter.maxNumActiveSubarrayPerColumn); numActiveSubarrayPerColumn *= 2)
				for (muxSenseAmp = gInputParameter.minMuxSenseAmp; muxSenseAmp <= gInputParameter.maxMuxSenseAmp; muxSenseAmp *= 2)
				for (muxOutputLev1 = gInputParameter.minMuxOutputLev1; muxOutputLev1 <= gInputParameter.maxMuxOutputLev1; muxOutputLev1 *= 2)
				for (muxOutputLev2 = gInputParameter.minMuxOutputLev2; muxOutputLev2 <= gInputParameter.maxMuxOutputLev2; muxOutputLev2 *= 2)
				for (numRowPerSet = gInputParameter.minNumRowPerSet; numRowPerSet <= std::min(gInputParameter.maxNumRowPerSet, gInputParameter.associativity); numRowPerSet *= 2)
				for (areaOptimizationLevel = gInputParameter.minAreaOptimizationLevel; areaOptimizationLevel <= gInputParameter.maxAreaOptimizationLevel; areaOptimizationLevel++) {
					if (blockSize / (numActiveMatPerRow * numActiveMatPerColumn * numActiveSubarrayPerRow * numActiveSubarrayPerColumn) == 0) {
						/* To aggressive partitioning */
						continue;
					}
					std::shared_ptr<mse::component::Bank> dataBank;
					if (CALCULATE(dataBank, dataT)) {
						Result tempResult;
						VERIFY_DATA_CAPACITY;
						numSolutionTask++;
						// UPDATE_BEST_DATA;
						tempResult.bank = dataBank;
						*(tempResult.localWire) = gLocalWire;
						*(tempResult.globalWire) = gGlobalWire;
						for (int i = 0; i < (int)full_exploration; i++)
							bestDataResultsTask[i].compareAndUpdate(tempResult);
						if (gInputParameter.optimizationTarget == full_exploration && !gInputParameter.isPruningEnabled) {
							OUTPUT_TO_FILE;
						}
					}
				}

				#pragma omp critical
				{
					for (int i = 0; i < full_exploration; i++) {
						bestDataResults[i].compareAndUpdate(bestDataResultsTask[i]);
					}
					numSolution += numSolutionTask;
				}
			}
	}

	if (numSolution > 0) {
		Result tempResult;
		/* refine local wire type */
		REFINE_LOCAL_WIRE_FORLOOP {
			gLocalWire.Initialize(gInputParameter.processNode, (WireType)localWireType,
					(WireRepeaterType)localWireRepeaterType, gInputParameter.temperature,
					(bool)isLocalWireLowSwing);
			for (int i = 0; i < (int)full_exploration; i++) {
				LOAD_GLOBAL_WIRE(bestDataResults[i]);
				TRY_AND_UPDATE(bestDataResults[i], dataT);
			}
		}
		/* refine global wire type */
		REFINE_GLOBAL_WIRE_FORLOOP {
			gGlobalWire.Initialize(gInputParameter.processNode, (WireType)globalWireType,
					(WireRepeaterType)globalWireRepeaterType, gInputParameter.temperature,
					(bool)isGlobalWireLowSwing);
			for (int i = 0; i < (int)full_exploration; i++) {
				LOAD_LOCAL_WIRE(bestDataResults[i]);
				TRY_AND_UPDATE(bestDataResults[i], dataT);
			}
		}
	}

	if (gInputParameter.optimizationTarget == full_exploration && gInputParameter.isPruningEnabled) {
		/* pruning is enabled */
		Result**** pruningResults;
		/* pruningResults[x][y][z] points to the result which is optimized for x, with constraint on y with z overhead */
		pruningResults = new Result***[(int)full_exploration];	/* full_exploration is always set as the last element in the enum, so if full_exploration is 8, what we want here is a 0-7 array, which is correct */
		for (int i = 0; i < (int)full_exploration; i++) {
			pruningResults[i] = new Result**[(int)full_exploration];
			for (int j = 0; j < (int)full_exploration; j++) {
				pruningResults[i][j] = new Result*[3];		/* 10%, 20%, and 30% overhead */
				for (int k = 0; k < 3; k++)
					pruningResults[i][j][k] = new Result;
			}
		}

		/* assign the constraints */
		for (int i = 0; i < (int)full_exploration; i++)
			for (int j = 0; j < (int)full_exploration; j++)
				for (int k = 0; k < 3; k++) {
					pruningResults[i][j][k]->optimizationTarget = (OptimizationTarget)i;
					*(pruningResults[i][j][k]->localWire) = *(bestDataResults[i].localWire);
					*(pruningResults[i][j][k]->globalWire) = *(bestDataResults[i].globalWire);
					switch ((OptimizationTarget)j) {
						case read_latency_optimized:
							pruningResults[i][j][k]->limitReadLatency = bestDataResults[j].readLatency() * (1 + (k + 1.0) / 10);
							break;
						case write_latency_optimized:
							pruningResults[i][j][k]->limitWriteLatency = bestDataResults[j].writeLatency() * (1 + (k + 1.0) / 10);
							break;
						case read_energy_optimized:
							pruningResults[i][j][k]->limitReadDynamicEnergy = bestDataResults[j].readDynamicEnergy() * (1 + (k + 1.0) / 10);
							break;
						case write_energy_optimized:
							pruningResults[i][j][k]->limitWriteDynamicEnergy = bestDataResults[j].writeDynamicEnergy() * (1 + (k + 1.0) / 10);
							break;
						case read_edp_optimized:
							pruningResults[i][j][k]->limitReadEdp = bestDataResults[j].readLatency() * bestDataResults[j].readDynamicEnergy() * (1 + (k + 1.0) / 10);
							break;
						case write_edp_optimized:
							pruningResults[i][j][k]->limitWriteEdp = bestDataResults[j].writeLatency() * bestDataResults[j].writeDynamicEnergy() * (1 + (k + 1.0) / 10);
							break;
						case area_optimized:
							pruningResults[i][j][k]->limitArea = bestDataResults[j].area() * (1 + (k + 1.0) / 10);
							break;
						case leakage_optimized:
							pruningResults[i][j][k]->limitLeakage = bestDataResults[j].leakage() * (1 + (k + 1.0) / 10);
							break;
						default:
							/* nothing should happen here */
							std::cout << "Warning: should not happen" << std::endl;
					}
				}

		for (int i = 0; i < (int)full_exploration; i++) {
    		if (gInputParameter.designTarget == cache)
        		bestDataResults[i].printAsCacheToYamlFile(bestTagResults[i], gInputParameter.cacheAccessMode, outputFile);
    		else
        		bestDataResults[i].printToYamlFile(outputFile);
		}
		std::cout << "Pruning done" << std::endl;
		for (int i = 0; i < (int)full_exploration; i++) {
			for (int j = 0; j < (int)full_exploration; j++) {
				for (int k = 0; k < 3; k++)
					delete pruningResults[i][j][k];
				delete [] pruningResults[i][j];
			}
			delete [] pruningResults[i];
		}
	}

	/* If design constraint is applied */
	if (gInputParameter.optimizationTarget != full_exploration && gInputParameter.isConstraintApplied) {
		double allowedDataReadLatency = bestDataResults[read_latency_optimized].readLatency() * (gInputParameter.readLatencyConstraint + 1);
		double allowedDataWriteLatency = bestDataResults[write_latency_optimized].writeLatency() * (gInputParameter.writeLatencyConstraint + 1);
		double allowedDataReadDynamicEnergy = bestDataResults[read_energy_optimized].readDynamicEnergy() * (gInputParameter.readDynamicEnergyConstraint + 1);
		double allowedDataWriteDynamicEnergy = bestDataResults[write_energy_optimized].writeDynamicEnergy() * (gInputParameter.writeDynamicEnergyConstraint + 1);
		double allowedDataLeakage = bestDataResults[leakage_optimized].leakage() * (gInputParameter.leakageConstraint + 1);
		double allowedDataArea = bestDataResults[area_optimized].area() * (gInputParameter.areaConstraint + 1);
		double allowedDataReadEdp = bestDataResults[read_edp_optimized].readLatency()
				* bestDataResults[read_edp_optimized].readDynamicEnergy() * (gInputParameter.readEdpConstraint + 1);
		double allowedDataWriteEdp = bestDataResults[write_edp_optimized].writeLatency()
				* bestDataResults[write_edp_optimized].writeDynamicEnergy() * (gInputParameter.writeEdpConstraint + 1);
		for (int i = 0; i < (int)full_exploration; i++) {
			APPLY_LIMIT(bestDataResults[i]);
		}

		numSolution = 0;
		INITIAL_BASIC_WIRE;
		BIGFOR {
			if (blockSize / (numActiveMatPerRow * numActiveMatPerColumn * numActiveSubarrayPerRow * numActiveSubarrayPerColumn) == 0) {
				/* To aggressive partitioning */
				continue;
			}
			if (!CALCULATE(dataBank, dataT)) {
				continue;
			}
			const double readLatency = dataBank->timing().latency[mse::cell::Operation::Read].value();
			const double writeLatency = dataBank->timing().latency[mse::cell::Operation::Write].value();
			const double readDynamicEnergy = dataBank->power().dynamicEnergy[mse::cell::Operation::Read].value();
			const double writeDynamicEnergy = dataBank->power().dynamicEnergy[mse::cell::Operation::Write].value();
			if (readLatency <= allowedDataReadLatency && writeLatency <= allowedDataWriteLatency
					&& readDynamicEnergy <= allowedDataReadDynamicEnergy && writeDynamicEnergy <= allowedDataWriteDynamicEnergy
					&& dataBank->power().leakage.value() <= allowedDataLeakage && dataBank->dimensions().area().value() <= allowedDataArea
					&& readLatency * readDynamicEnergy <= allowedDataReadEdp && writeLatency * writeDynamicEnergy <= allowedDataWriteEdp) {
				Result tempResult;
				VERIFY_DATA_CAPACITY;
				numSolution++;
				UPDATE_BEST_DATA;
			}
		}
	}

	if (gInputParameter.optimizationTarget != full_exploration) {
		if (numSolution > 0) {
			// Print to console (for user to see)
			if (gInputParameter.designTarget == cache)
				bestDataResults[gInputParameter.optimizationTarget].printAsCache(bestTagResults[gInputParameter.optimizationTarget], gInputParameter.cacheAccessMode);
			else
				bestDataResults[gInputParameter.optimizationTarget].print();

			// NEW: Also write to YAML file (for pipeline to parse)
			std::string outputDirectory = gInputParameter.outputDirectory;
			std::stringstream temp;
			temp << outputDirectory << gInputParameter.outputFilePrefix << ".yaml";
			std::string yamlFileName = temp.str();
			std::ofstream yamlFile;
			std::error_code directoryError;
			std::filesystem::create_directory(outputDirectory, directoryError);	/* create the output directory if it is missing */
			yamlFile.open(yamlFileName.c_str());
			if (yamlFile.is_open()) {
				if (gInputParameter.designTarget == cache)
					bestDataResults[gInputParameter.optimizationTarget].printAsCacheToYamlFile(
						bestTagResults[gInputParameter.optimizationTarget], 
						gInputParameter.cacheAccessMode, 
						yamlFile);
				else
					bestDataResults[gInputParameter.optimizationTarget].printToYamlFile(yamlFile);
				yamlFile.close();
				std::cout << "Results written to " << yamlFileName << std::endl;
			} else {
				std::cout << "Error: cannot write " << yamlFileName << " (does the output directory exist?)" << std::endl;
				numSolution = 0;
			}
		} else {
			std::cout << "No valid solutions." << std::endl;
		}
    std::cout << std::endl << "Finished!" << std::endl;
	} else {
		std::cout << std::endl << outputFileName << " generated successfully!" << std::endl;
		if (gInputParameter.isPruningEnabled) {
			std::cout << "The results are pruned" << std::endl;
		} else {
			int solutionMultiplier = 1;
			if (gInputParameter.designTarget == cache)
				solutionMultiplier = 8;
			std::cout << numSolution * solutionMultiplier << " solutions in total" << std::endl;
		}
	}

	if (outputFile.is_open())
		outputFile.close();

	return (numSolution > 0) ? 0 : 1;
}

void applyConstraint() {
	/* Check functions that are not yet implemented */
	// TODO no CAM model
	if (gInputParameter.designTarget == CAM_chip) {
		std::cout << "[ERROR] CAM model is still under development" << std::endl;
		exit(-1);
	}
	// TODO the DRAM model is unfinished and rejected here
	if (std::holds_alternative<mse::cell::DramCell>(mse::cell::runCell)) {
		std::cout << "[ERROR] DRAM model is still under development" << std::endl;
		exit(-1);
	}
	// TODO the eDRAM model is unfinished but still runs
	if (std::holds_alternative<mse::cell::EdramCell>(mse::cell::runCell)) {
		std::cout << "[Warning] Embedded DRAM model is still under development" << std::endl;
		//exit(-1);
	}

	if (gInputParameter.designTarget != cache && gInputParameter.associativity > 1) {
		std::cout << "[WARNING] Associativity setting is ignored for non-cache designs" << std::endl;
		gInputParameter.associativity = 1;
	}

	if (!std::has_single_bit(static_cast<unsigned>(gInputParameter.associativity))) {
		std::cout << "[ERROR] The associativity value has to be a power of 2 in this version" << std::endl;
		exit(-1);
	}

	// TODO no external sensing with H-tree routing
	if (gInputParameter.routingMode == h_tree && gInputParameter.internalSensing == false) {
		std::cout << "[ERROR] H-tree does not support external sensing scheme in this version" << std::endl;
		exit(-1);
	}
}
