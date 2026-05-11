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


#include "BankWithoutHtree.h"
#include "formula.h"
#include "global.h"
#include <math.h>

void BankWithoutHtree::Initialize(int _numRowMat, int _numColumnMat, long long _capacity,
		long _blockSize, int _associativity, int _numRowPerSet, int _numActiveMatPerRow,
		int _numActiveMatPerColumn, int _muxSenseAmp, bool _internalSenseAmp, int _muxOutputLev1, int _muxOutputLev2,
		int _numRowSubarray, int _numColumnSubarray,
		int _numActiveSubarrayPerRow, int _numActiveSubarrayPerColumn,
		BufferDesignTarget _areaOptimizationLevel, MemoryType _memoryType) {
	if (initialized) {
		/* Reset the class for re-initialization */
		initialized = false;
		invalid = false;
	}

	if (!_internalSenseAmp) {
		if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM || gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
			invalid = true;
			std::cout << "[BankWithoutHtree] Error: DRAM does not support external sense amplification!" << std::endl;
			return;
		} else if (gGlobalWire.wireRepeaterType != repeated_none) {
			invalid = true;
			initialized = true;
			return;
		}
	}

	numRowMat = _numRowMat;
	numColumnMat = _numColumnMat;
	capacity = _capacity;
	blockSize = _blockSize;
	associativity = _associativity;
	numRowPerSet = _numRowPerSet;
	internalSenseAmp = _internalSenseAmp;
	areaOptimizationLevel = _areaOptimizationLevel;
	memoryType = _memoryType;
	numWay = 1;	/* default value for non-cache design */

	/* Calculate the physical signals that are required in routing */
	numAddressBit = (int)(log2((double)capacity / blockSize / associativity) + 0.1);
			/* use double during the calculation to avoid overflow */

	if (_numActiveMatPerRow > numColumnMat) {
		std::cout << "[Bank] Warning: The number of active subarray per row is larger than the number of subarray per row!"  << std::endl;
		std::cout << _numActiveMatPerRow << " > " << numColumnMat << std::endl;
		numActiveMatPerRow = numColumnMat;
	} else {
		numActiveMatPerRow = _numActiveMatPerRow;
	}
	if (_numActiveMatPerColumn > numRowMat) {
		std::cout << "[Bank] Warning: The number of active subarray per column is larger than the number of subarray per column!"  << std::endl;
		std::cout << _numActiveMatPerColumn << " > " << numRowMat << std::endl;
		numActiveMatPerColumn = numRowMat;
	} else {
		numActiveMatPerColumn = _numActiveMatPerColumn;
	}
	muxSenseAmp = _muxSenseAmp;
	muxOutputLev1 = _muxOutputLev1;
	muxOutputLev2 = _muxOutputLev2;

	numRowSubarray = _numRowSubarray;
	numColumnSubarray = _numColumnSubarray;
	if (_numActiveSubarrayPerRow > numColumnSubarray) {
		std::cout << "[Bank] Warning: The number of active subarray per row is larger than the number of subarray per row!"  << std::endl;
		std::cout << _numActiveSubarrayPerRow << " > " << numColumnSubarray << std::endl;
		numActiveSubarrayPerRow = numColumnSubarray;
	} else {
		numActiveSubarrayPerRow = _numActiveSubarrayPerRow;
	}
	if (_numActiveSubarrayPerColumn > numRowSubarray) {
		std::cout << "[Bank] Warning: The number of active subarray per column is larger than the number of subarray per column!"  << std::endl;
		std::cout << _numActiveSubarrayPerColumn << " > " << numRowSubarray << std::endl;
		numActiveSubarrayPerColumn = numRowSubarray;
	} else {
		numActiveSubarrayPerColumn = _numActiveSubarrayPerColumn;
	}

	/* The number of address bits that are used to power gate inactive mats */
	int numAddressForGating = (int)(log2(numRowMat * numColumnMat / numActiveMatPerColumn / numActiveMatPerRow)+0.1);
	numAddressBitRouteToMat = numAddressBit - numAddressForGating;	/* Only use the effective address bits in the following calculation */
	numDataBitRouteToMat = blockSize;


	if (memoryType == dataT) { /* Data array */
		numDataBitRouteToMat = blockSize / numActiveMatPerColumn / numActiveMatPerRow;
		if (numRowPerSet > associativity) {
			/* There is no enough ways to distribute into multiple rows */
			invalid = true;
			initialized = true;
			return;
		}
		numWay = associativity;
		int numWayPerRow = numWay / numRowPerSet;	/* At least 1, otherwise it is invalid, and returned already */
		if (numWayPerRow > 1) {		/* multiple ways per row, needs extra mux level */
			/* Do mux level recalculation to contain the multiple ways */
			if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM || gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
				/* for DRAM, mux before sense amp has to be 1, only mux output1 and mux output2 can be used */
				int numWayPerRowInLog = (int)(log2((double)numWayPerRow) + 0.1);
				int extraMuxOutputLev2 = (int)pow(2, numWayPerRowInLog / 2);
				int extraMuxOutputLev1 = numWayPerRow / extraMuxOutputLev2;
				muxOutputLev1 *= extraMuxOutputLev1;
				muxOutputLev2 *= extraMuxOutputLev2;
			} else {
				/* for non-DRAM, all mux levels can be used */
				int numWayPerRowInLog = (int)(log2((double)numWayPerRow) + 0.1);
				int extraMuxOutputLev2 = (int)pow(2, numWayPerRowInLog / 3);
				int extraMuxOutputLev1 = extraMuxOutputLev2;
				int extraMuxSenseAmp = numWayPerRow / extraMuxOutputLev1 / extraMuxOutputLev2;
				muxSenseAmp *= extraMuxSenseAmp;
				muxOutputLev1 *= extraMuxOutputLev1;
				muxOutputLev2 *= extraMuxOutputLev2;
			}
		}
	} else if (memoryType == tag) { /* Tag array */
		if (numRowPerSet > 1) {
			/* tag array cannot have multiple rows to contain ways in a set, otherwise the bitline has to be shared */
			invalid = true;
			initialized = true;
			return;
		}
		numDataBitRouteToMat = blockSize;
		numWay = associativity / numActiveMatPerColumn / numActiveMatPerRow;
		if (numWay < 1) {
			/* This mat does not contain at least one way */
			invalid = true;
			initialized = true;
			return;
		}
	} else { /* CAM */
		numDataBitRouteToMat = blockSize;
		numWay = 1;
	}


	mat.Initialize(numRowSubarray, numColumnSubarray, numAddressBitRouteToMat, numDataBitRouteToMat,
			numWay, numRowPerSet, false, numActiveSubarrayPerRow, numActiveSubarrayPerColumn,
			muxSenseAmp, internalSenseAmp, muxOutputLev1, muxOutputLev2, areaOptimizationLevel, memoryType);
	/* Check if mat is under a legal configuration */
	if (mat.invalid) {
		invalid = true;
		initialized = true;
		return;
	}

	mat.CalculateArea();

	if (!internalSenseAmp) {
		bool voltageSense = true;
		double senseVoltage;
		senseVoltage = gCell.minSenseVoltage;
		if (gCell.memCellType == SRAM) {
			/* SRAM, DRAM, and eDRAM all use voltage sensing */
			voltageSense = true;
		} else if (gCell.memCellType == MRAM || gCell.memCellType == PCRAM || gCell.memCellType == memristor || gCell.memCellType == FBRAM || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
			voltageSense = gCell.readMode;
		} else {/* NAND flash */
			// TO-DO
		}

		int numSenseAmp;
		if (memoryType == dataT)
			numSenseAmp = blockSize;
		else
			numSenseAmp = blockSize * associativity;

                bool mlc = false; 
                if (gCell.memCellType == MLCCTT || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
                    mlc = true;
                }
		globalSenseAmp.Initialize(numSenseAmp, !voltageSense, senseVoltage, mat.width * numColumnMat / numSenseAmp, mlc, gCell.nLvl, gCell.nFingers);
		if (globalSenseAmp.invalid) {
			invalid = true;
			initialized = true;
			return;
		}
		globalSenseAmp.CalculateRC();
		globalBitlineMux.Initialize(numRowMat * numColumnMat / numActiveMatPerColumn / numActiveMatPerRow, numSenseAmp, globalSenseAmp.capLoad, globalSenseAmp.capLoad, 0);
		globalBitlineMux.CalculateRC();

		if (memoryType == tag)
			globalComparator.Initialize(blockSize, 0 /* TO-DO: only for test */);
	}

	/* Reset the mux values for correct printing */
	muxSenseAmp = _muxSenseAmp;
	muxOutputLev1 = _muxOutputLev1;
	muxOutputLev2 = _muxOutputLev2;

	initialized = true;
}

void BankWithoutHtree::CalculateArea() {
	if (!initialized) {
		std::cout << "[BankWithoutHtree] Error: Require initialization first!" << std::endl;
	} else if (invalid) {
		height = width = area = invalid_value;
	} else {
		height = mat.height * numRowMat;
		width = mat.width * numColumnMat;

		int numWireSharingWidth;
		double effectivePitch;
		if (gGlobalWire.wireRepeaterType == repeated_none) {
			numWireSharingWidth = 1;
			effectivePitch = 0;		/* assume that the wire is built on another metal layer, there does not cause silicon area */
			//effectivePitch = gGlobalWire.wirePitch;
		} else {
			numWireSharingWidth = (int)floor(gGlobalWire.repeaterSpacing / gGlobalWire.repeaterHeight);
			effectivePitch = gGlobalWire.repeatedWirePitch;
		}

		width += ceil((double)numRowMat * numColumnMat * numAddressBitRouteToMat / numWireSharingWidth) * effectivePitch;

		if (!internalSenseAmp) {
			globalSenseAmp.CalculateArea();
			height += globalSenseAmp.height;
			globalBitlineMux.CalculateArea();
			height += globalBitlineMux.height;
			if (memoryType == tag) {
				globalComparator.CalculateArea();
				height += associativity * globalComparator.area / width;
			}
		}

		/* Determine if the aspect ratio meets the constraint */
		if (memoryType == dataT)
			if (height / width > CONSTRAINT_ASPECT_RATIO_BANK || width / height > CONSTRAINT_ASPECT_RATIO_BANK) {
				/* illegal */
				invalid = true;
				height = width = area = invalid_value;
				return;
			}

		area = height * width;
	}
}

void BankWithoutHtree::CalculateRC() {
	if (!initialized) {
		std::cout << "[BankWithoutHtree] Error: Require initialization first!" << std::endl;
	} else if (!invalid) {
		mat.CalculateRC();
		if (!internalSenseAmp) {
			globalBitlineMux.CalculateRC();
			globalSenseAmp.CalculateRC();
			if (memoryType == tag)
				globalComparator.CalculateRC();
		}
	}
}

void BankWithoutHtree::CalculateLatencyAndPower() {
	if (!initialized) {
		std::cout << "[BankWithoutHtree] Error: Require initialization first!" << std::endl;
	} else if (invalid) {
		readLatency = writeLatency = invalid_value;
		readDynamicEnergy = writeDynamicEnergy = invalid_value;
		leakage = invalid_value;
	} else {
		double latency = 0;
		double energy = 0;
		double leakageWire = 0;

		mat.CalculateLatency(infinite_ramp);
		mat.CalculatePower();
		readLatency = resetLatency = setLatency = writeLatency = 0;
        refreshLatency = mat.refreshLatency * numColumnMat; // TOTAL refresh time for all Mats
		readDynamicEnergy = writeDynamicEnergy = resetDynamicEnergy = setDynamicEnergy = 0;
        refreshDynamicEnergy = mat.refreshDynamicEnergy * numRowMat * numColumnMat;
		leakage = 0;

		double lengthWire;
		lengthWire = mat.height * (numRowMat + 1);
		for (int i = 0; i < numRowMat; i++) {
			lengthWire -= mat.height;
			if (internalSenseAmp) {
				double numBitRouteToMat = 0;
				gGlobalWire.CalculateLatencyAndPower(lengthWire, &latency, &energy, &leakageWire);
				if (i == 0){
					readLatency += latency;
					writeLatency += latency;
                    refreshLatency += latency;
				}
				if (i < numActiveMatPerColumn) {
					if (memoryType == tag)
						numBitRouteToMat = numAddressBitRouteToMat + numDataBitRouteToMat + numWay;
					else
						numBitRouteToMat = numAddressBitRouteToMat + numDataBitRouteToMat;
					readDynamicEnergy += energy * numBitRouteToMat * numActiveMatPerRow;
					writeDynamicEnergy += energy * numBitRouteToMat * numActiveMatPerRow;
                    refreshDynamicEnergy += energy * numBitRouteToMat * numActiveMatPerRow;
				}
				leakage += leakageWire * numBitRouteToMat * numColumnMat;
			} else {
				double resLocalBitline, capLocalBitline, resBitlineMux, capBitlineMux;
				capBitlineMux = globalBitlineMux.capNMOSPassTransistor;
				resBitlineMux = globalBitlineMux.resNMOSPassTransistor;
				resLocalBitline = mat.subarray.resBitline + 3 * resBitlineMux;
				capLocalBitline = mat.subarray.capBitline + 6 * capBitlineMux;
				double resGlobalBitline, capGlobalBitline;
				resGlobalBitline = lengthWire * gGlobalWire.resWirePerUnit;
				capGlobalBitline = lengthWire * gGlobalWire.capWirePerUnit;
				double capGlobalBitlineMux;
				capGlobalBitlineMux = globalBitlineMux.capForPreviousDelayCalculation;
				if (gCell.memCellType == SRAM) {
					double vpre = gCell.readVoltage;	/* This value should be equal to resetVoltage and setVoltage for SRAM */
					if (i == 0) {
						latency = resLocalBitline * capGlobalBitline / 2 +
								(resLocalBitline + resGlobalBitline) * (capGlobalBitline / 2 + capGlobalBitlineMux);
						latency *= log(vpre / (vpre - globalSenseAmp.senseVoltage));
						latency += resLocalBitline * capGlobalBitline / 2;
						globalBitlineMux.CalculateLatency(1e20);
						latency += globalBitlineMux.readLatency;
						globalSenseAmp.CalculateLatency();
						writeLatency += latency;
						latency += globalSenseAmp.readLatency;
						readLatency += latency;
					}
					if (i <  numActiveMatPerColumn) {
						energy = capGlobalBitline * gTech.vdd * gTech.vdd * numAddressBitRouteToMat;
						readDynamicEnergy += energy;
						writeDynamicEnergy += energy;
						readDynamicEnergy += capGlobalBitline * vpre * vpre * numWay;
						writeDynamicEnergy += capGlobalBitline * vpre * vpre * numDataBitRouteToMat;
					}
				} else if (gCell.memCellType == MRAM || gCell.memCellType == PCRAM || gCell.memCellType == memristor || gCell.memCellType == FBRAM || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
					double vWrite = MAX(fabs(gCell.resetVoltage), fabs(gCell.setVoltage));
					double tau, latencyOff, latencyOn;
					double vPre = mat.subarray.voltagePrecharge;
					double vOn = mat.subarray.voltageMemCellOn;
					double vOff = mat.subarray.voltageMemCellOff;
					if (i == 0) {
						tau = resBitlineMux * capGlobalBitline / 2 + (resBitlineMux + resGlobalBitline)
								* (capGlobalBitline + capLocalBitline) / 2 + (resBitlineMux + resGlobalBitline
										+ resLocalBitline) * capLocalBitline / 2;
						writeLatency += 0.63 * tau;
						if (gCell.readMode == false) {	/* current-sensing */
							/* Use ICCAD 2009 model */
							resLocalBitline += mat.subarray.resMemCellOff;
							tau = resGlobalBitline * capGlobalBitline / 2 *
									(resLocalBitline + resGlobalBitline / 3) / (resLocalBitline + resGlobalBitline);
							readLatency += 0.63 * tau;
						} else {						/* voltage-sensing */
							if (gCell.readVoltage == 0) {  /* Current-in voltage sensing */
								resLocalBitline += mat.subarray.resMemCellOn;
								tau = resLocalBitline * capGlobalBitline + (resLocalBitline + resGlobalBitline) * capGlobalBitline / 2;
								latencyOn = tau * log((vPre - vOn)/(vPre - vOn - globalSenseAmp.senseVoltage));
								resLocalBitline += gCell.resistanceOff - gCell.resistanceOn;
								tau = resLocalBitline * capGlobalBitline + (resLocalBitline + resGlobalBitline) * capGlobalBitline / 2;
								latencyOff = tau * log((vOff - vPre)/(vOff - vPre - globalSenseAmp.senseVoltage));
							} else {   /*Voltage-in voltage sensing */
								resLocalBitline += mat.subarray.resEquivalentOn;
								tau = resLocalBitline * capGlobalBitline + (resLocalBitline + resGlobalBitline) * capGlobalBitline / 2;
								latencyOn = tau * log((vPre - vOn)/(vPre - vOn - globalSenseAmp.senseVoltage));
								resLocalBitline += mat.subarray.resEquivalentOff - mat.subarray.resEquivalentOn;
								tau = resLocalBitline * capGlobalBitline + (resLocalBitline + resGlobalBitline) * capGlobalBitline / 2;
								latencyOff = tau * log((vOff - vPre)/(vOff - vPre - globalSenseAmp.senseVoltage));
							}
							readLatency -= mat.subarray.bitlineDelay;
							if ((latencyOn + mat.subarray.bitlineDelayOn) > (latencyOff + mat.subarray.bitlineDelayOff))
								readLatency += latencyOn + mat.subarray.bitlineDelayOn;
							else
								readLatency += latencyOff + mat.subarray.bitlineDelayOff;
						}
					}
					if (i <  numActiveMatPerColumn) {
						energy = capGlobalBitline * gTech.vdd * gTech.vdd * numAddressBitRouteToMat;
						readDynamicEnergy += energy;
						writeDynamicEnergy += energy;
						writeDynamicEnergy += capGlobalBitline * vWrite * vWrite * numDataBitRouteToMat;
						if (gCell.readMode) { /*Voltage-in voltage sensing */
							readDynamicEnergy += capGlobalBitline * (vPre * vPre - vOn * vOn )* numDataBitRouteToMat;
						}
					}
				}

			}
		}
		if (!internalSenseAmp) {
			globalBitlineMux.CalculateLatency(1e40);
			globalSenseAmp.CalculateLatency();
			readLatency += globalBitlineMux.readLatency + globalSenseAmp.readLatency;
			writeLatency += globalBitlineMux.writeLatency + globalSenseAmp.writeLatency;
			globalBitlineMux.CalculatePower();
			globalSenseAmp.CalculatePower();
			readDynamicEnergy += (globalBitlineMux.readDynamicEnergy + globalSenseAmp.readDynamicEnergy) * numActiveMatPerRow;
			writeDynamicEnergy += (globalBitlineMux.writeDynamicEnergy + globalSenseAmp.writeDynamicEnergy) * numActiveMatPerRow;
			leakage += (globalBitlineMux.leakage + globalSenseAmp.leakage) * numColumnMat;
			if (memoryType == tag) {
				globalComparator.CalculateLatency(1e40);
				readLatency += globalComparator.readLatency;
				globalComparator.CalculatePower();
				readDynamicEnergy += numWay * globalComparator.readDynamicEnergy;
				leakage += associativity * globalComparator.leakage;
			}
		}
	}

	/* only 1/A wires are activated in fast mode cache write */
	if (gInputParameter.designTarget == cache && gInputParameter.cacheAccessMode == fast_access_mode)
		writeDynamicEnergy /= gInputParameter.associativity;

	readLatency += mat.readLatency;
	resetLatency = writeLatency + mat.resetLatency;
	setLatency = writeLatency + mat.setLatency;
	writeLatency += mat.writeLatency;
	readDynamicEnergy += mat.readDynamicEnergy * numActiveMatPerRow * numActiveMatPerColumn;
	cellReadEnergy = mat.cellReadEnergy * numActiveMatPerRow * numActiveMatPerColumn;
	cellSetEnergy = mat.cellSetEnergy * numActiveMatPerRow * numActiveMatPerColumn;
	cellResetEnergy = mat.cellResetEnergy * numActiveMatPerRow * numActiveMatPerColumn;
	resetDynamicEnergy = writeDynamicEnergy + mat.resetDynamicEnergy * numActiveMatPerRow * numActiveMatPerColumn;
	setDynamicEnergy = writeDynamicEnergy + mat.setDynamicEnergy * numActiveMatPerRow * numActiveMatPerColumn;
	writeDynamicEnergy += mat.writeDynamicEnergy * numActiveMatPerRow * numActiveMatPerColumn;
	leakage += mat.leakage * numRowMat * numColumnMat;

	if (gCell.memCellType == eDRAM || gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
        if (refreshLatency > gCell.retentionTime) {
            invalid = true;
        }
    }
}
