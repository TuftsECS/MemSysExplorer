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


#include "Result.h"
#include "global.h"
#include "formula.h"
#include "macros.h"
#include "yaml-cpp/yaml.h"

#include <iostream>
#include <fstream>

using namespace std;

Result::Result() {
	// TODO Auto-generated constructor stub
	if (inputParameter->routingMode == h_tree)
		bank = new BankWithHtree();
	else
		bank = new BankWithoutHtree();
	localWire = new Wire();
	globalWire = new Wire();

	/* initialize the worst case */
	bank->readLatency = invalid_value;
	bank->writeLatency = invalid_value;
	bank->readDynamicEnergy = invalid_value;
	bank->writeDynamicEnergy = invalid_value;
	bank->leakage = invalid_value;
	bank->height = invalid_value;
	bank->width = invalid_value;
	bank->area = invalid_value;

	/* No constraints */
	limitReadLatency = invalid_value;
	limitWriteLatency = invalid_value;
	limitReadDynamicEnergy = invalid_value;
	limitWriteDynamicEnergy = invalid_value;
	limitReadEdp = invalid_value;
	limitWriteEdp = invalid_value;
	limitArea = invalid_value;
	limitLeakage = invalid_value;

	/* Default read latency optimization */
	optimizationTarget = read_latency_optimized;
}

Result::~Result() {
	// TODO Auto-generated destructor stub
	if (bank)
		delete bank;
	if (Result::localWire)
		delete Result::localWire;
	if (Result::globalWire)
		delete Result::globalWire;
}

void Result::reset() {
	bank->readLatency = invalid_value;
	bank->writeLatency = invalid_value;
	bank->readDynamicEnergy = invalid_value;
	bank->writeDynamicEnergy = invalid_value;
	bank->leakage = invalid_value;
	bank->height = invalid_value;
	bank->width = invalid_value;
	bank->area = invalid_value;
}

void Result::compareAndUpdate(Result &newResult) {
	if (newResult.bank->readLatency <= limitReadLatency && newResult.bank->writeLatency <= limitWriteLatency
			&& newResult.bank->readDynamicEnergy <= limitReadDynamicEnergy && newResult.bank->writeDynamicEnergy <= limitWriteDynamicEnergy
			&& newResult.bank->readLatency * newResult.bank->readDynamicEnergy <= limitReadEdp
			&& newResult.bank->writeLatency * newResult.bank->writeDynamicEnergy <= limitWriteEdp
			&& newResult.bank->area <= limitArea && newResult.bank->leakage <= limitLeakage) {
		bool toUpdate = false;
		switch (optimizationTarget) {
		case read_latency_optimized:
			if 	(newResult.bank->readLatency < bank->readLatency)
				toUpdate = true;
			break;
		case write_latency_optimized:
			if 	(newResult.bank->writeLatency < bank->writeLatency)
				toUpdate = true;
			break;
		case read_energy_optimized:
			if 	(newResult.bank->readDynamicEnergy < bank->readDynamicEnergy)
				toUpdate = true;
			break;
		case write_energy_optimized:
			if 	(newResult.bank->writeDynamicEnergy < bank->writeDynamicEnergy)
				toUpdate = true;
			break;
		case read_edp_optimized:
			if 	(newResult.bank->readLatency * newResult.bank->readDynamicEnergy < bank->readLatency * bank->readDynamicEnergy)
				toUpdate = true;
			break;
		case write_edp_optimized:
			if 	(newResult.bank->writeLatency * newResult.bank->writeDynamicEnergy < bank->writeLatency * bank->writeDynamicEnergy)
				toUpdate = true;
			break;
		case area_optimized:
			if 	(newResult.bank->area < bank->area)
				toUpdate = true;
			break;
		case leakage_optimized:
			if 	(newResult.bank->leakage < bank->leakage)
				toUpdate = true;
			break;
		default:	/* Exploration */
			/* should not happen */
			;
		}
		if (toUpdate) {
			*bank = *(newResult.bank);
			*localWire = *(newResult.localWire);
			*globalWire = *(newResult.globalWire);
		}
	}
}

void Result::print() {
	cout << endl << "=============" << endl << "CONFIGURATION" << endl << "=============" << endl;
	cout << "Bank Organization: " << bank->numRowMat << " x " << bank->numColumnMat << endl;
	cout << " - Row Activation   : " << bank->numActiveMatPerColumn << " / " << bank->numRowMat << endl;
	cout << " - Column Activation: " << bank->numActiveMatPerRow << " / " << bank->numColumnMat << endl;
	cout << "Mat Organization: " << bank->numRowSubarray << " x " << bank->numColumnSubarray << endl;
	cout << " - Row Activation   : " << bank->numActiveSubarrayPerColumn << " / " << bank->numRowSubarray << endl;
	cout << " - Column Activation: " << bank->numActiveSubarrayPerRow << " / " << bank->numColumnSubarray << endl;
	cout << " - Subarray Size    : " << bank->mat.subarray.numRow << " Rows x " << bank->mat.subarray.numColumn << " Columns" << endl;
	cout << "Mux Level:" << endl;
	cout << " - Senseamp Mux      : " << bank->muxSenseAmp << endl;
	cout << " - Output Level-1 Mux: " << bank->muxOutputLev1 << endl;
	cout << " - Output Level-2 Mux: " << bank->muxOutputLev2 << endl;
	if (inputParameter->designTarget == cache)
		cout << " - One set is partitioned into " << bank->numRowPerSet << " rows" << endl;
	cout << "Local Wire:" << endl;
	cout << " - Wire Type : ";
	switch (localWire->wireType) {
	case local_aggressive:
		cout << "Local Aggressive" << endl;
		break;
	case local_conservative:
		cout << "Local Conservative" << endl;
		break;
	case semi_aggressive:
		cout << "Semi-Global Aggressive" << endl;
		break;
	case semi_conservative:
		cout << "Semi-Global Conservative" << endl;
		break;
	case global_aggressive:
		cout << "Global Aggressive" << endl;
		break;
	case global_conservative:
		cout << "Global Conservative" << endl;
		break;
	default:
		cout << "DRAM Wire" << endl;
	}
	cout << " - Repeater Type: ";
	switch (localWire->wireRepeaterType) {
	case repeated_none:
		cout << "No Repeaters" << endl;
		break;
	case repeated_opt:
		cout << "Fully-Optimized Repeaters" << endl;
		break;
	case repeated_5:
		cout << "Repeaters with 5% Overhead" << endl;
		break;
	case repeated_10:
		cout << "Repeaters with 10% Overhead" << endl;
		break;
	case repeated_20:
		cout << "Repeaters with 20% Overhead" << endl;
		break;
	case repeated_30:
		cout << "Repeaters with 30% Overhead" << endl;
		break;
	case repeated_40:
		cout << "Repeaters with 40% Overhead" << endl;
		break;
	case repeated_50:
		cout << "Repeaters with 50% Overhead" << endl;
		break;
	default:
		cout << "Unknown" << endl;
	}
	cout << " - Low Swing : ";
	if (localWire->isLowSwing)
		cout << "Yes" << endl;
	else
		cout << "No" << endl;
	cout << "Global Wire:" << endl;
	cout << " - Wire Type : ";
	switch (globalWire->wireType) {
	case local_aggressive:
		cout << "Local Aggressive" << endl;
		break;
	case local_conservative:
		cout << "Local Conservative" << endl;
		break;
	case semi_aggressive:
		cout << "Semi-Global Aggressive" << endl;
		break;
	case semi_conservative:
		cout << "Semi-Global Conservative" << endl;
		break;
	case global_aggressive:
		cout << "Global Aggressive" << endl;
		break;
	case global_conservative:
		cout << "Global Conservative" << endl;
		break;
	default:
		cout << "DRAM Wire" << endl;
	}
	cout << " - Repeater Type: ";
	switch (globalWire->wireRepeaterType) {
	case repeated_none:
		cout << "No Repeaters" << endl;
		break;
	case repeated_opt:
		cout << "Fully-Optimized Repeaters" << endl;
		break;
	case repeated_5:
		cout << "Repeaters with 5% Overhead" << endl;
		break;
	case repeated_10:
		cout << "Repeaters with 10% Overhead" << endl;
		break;
	case repeated_20:
		cout << "Repeaters with 20% Overhead" << endl;
		break;
	case repeated_30:
		cout << "Repeaters with 30% Overhead" << endl;
		break;
	case repeated_40:
		cout << "Repeaters with 40% Overhead" << endl;
		break;
	case repeated_50:
		cout << "Repeaters with 50% Overhead" << endl;
		break;
	default:
		cout << "Unknown" << endl;
	}
	cout << " - Low Swing : ";
	if (globalWire->isLowSwing)
		cout << "Yes" << endl;
	else
		cout << "No" << endl;
	cout << "Buffer Design Style: ";
	switch (bank->areaOptimizationLevel) {
	case latency_first:
		cout << "Latency-Optimized" << endl;
		break;
	case area_first:
		cout << "Area-Optimized" << endl;
		break;
	default:	/* balance */
		cout << "Balanced" << endl;
	}

	cout << "=============" << endl << "   RESULT" << endl << "=============" << endl;

	cout << "Area:" << endl;

	cout << " - Total Area = " << TO_METER(bank->height) << " x " << TO_METER(bank->width)
			<< " = " << TO_SQM(bank->area) << endl;
	cout << " |--- Mat Area      = " << TO_METER(bank->mat.height) << " x " << TO_METER(bank->mat.width)
			<< " = " << TO_SQM(bank->mat.area) << "   (" << cell->area * tech->featureSize * tech->featureSize
			* bank->capacity / bank->numRowMat / bank->numColumnMat / bank->mat.area * 100 << "%)" << endl;
	cout << " |--- Subarray Area = " << TO_METER(bank->mat.subarray.height) << " x "
			<< TO_METER(bank->mat.subarray.width) << " = " << TO_SQM(bank->mat.subarray.area) << "   ("
			<< cell->area * tech->featureSize * tech->featureSize * bank->capacity / bank->numRowMat
			/ bank->numColumnMat / bank->numRowSubarray / bank->numColumnSubarray
			/ bank->mat.subarray.area * 100 << "%)" <<endl;
	//Qing: subarray buffer area
	cout << " |--- Subarray Buffer Area = " << TO_METER(bank->mat.subarray.subarrayBuffer.height) << " x "
			<< TO_METER(bank->mat.subarray.subarrayBuffer.width) << " = " << TO_SQM(bank->mat.subarray.subarrayBuffer.area) <<endl;
	//Qing.
	cout << " - Area Efficiency = " << cell->area * tech->featureSize * tech->featureSize
			* bank->capacity / bank->area * 100 << "%" << endl;

	cout << "Timing:" << endl;

	cout << " -  Read Latency = " << TO_SECOND(bank->readLatency) << endl;
	if (inputParameter->routingMode == h_tree)
		cout << " |--- H-Tree Latency = " << TO_SECOND(bank->readLatency - bank->mat.readLatency) << endl;
	else
		cout << " |--- Non-H-Tree Latency = " << TO_SECOND(bank->readLatency - bank->mat.readLatency) << endl;
	cout << " |--- Mat Latency    = " << TO_SECOND(bank->mat.readLatency) << endl;
	cout << "    |--- Predecoder Latency = " << TO_SECOND(bank->mat.predecoderLatency) << endl;
	cout << "    |--- Subarray Latency   = " << TO_SECOND(bank->mat.subarray.readLatency) << endl;
	cout << "       |--- Row Decoder Latency = " << TO_SECOND(bank->mat.subarray.rowDecoder.readLatency) << endl;
	if (cell->memCellType != eDRAM3T333 && cell->memCellType != eDRAM3T)
		cout << "       |--- Bitline Latency     = " << TO_SECOND(bank->mat.subarray.bitlineDelay) << endl;
	else
		cout << "       |--- Bitline Latency     = " << TO_SECOND(bank->mat.subarray.bitlineDelayR) << endl;
	if (inputParameter->internalSensing)
		cout << "       |--- Senseamp Latency    = " << TO_SECOND(bank->mat.subarray.senseAmp.readLatency) << endl;
	cout << "       |--- Mux Latency         = " << TO_SECOND(bank->mat.subarray.bitlineMux.readLatency
													+ bank->mat.subarray.senseAmpMuxLev1.readLatency
													+ bank->mat.subarray.senseAmpMuxLev2.readLatency) << endl;
	cout << "       |--- Precharge Latency   = " << TO_SECOND(bank->mat.subarray.precharger.readLatency) << endl;
	if (bank->mat.memoryType == tag && bank->mat.internalSenseAmp)
		cout << "    |--- Comparator Latency  = " << TO_SECOND(bank->mat.comparator.readLatency) << endl;

	if (cell->memCellType == PCRAM || cell->memCellType == FBRAM || cell->memCellType == FeFET || cell->memCellType == MLCFeFET || cell->memCellType == MLCRRAM ||
			(cell->memCellType == memristor && (cell->accessType == CMOS_access || cell->accessType == BJT_access))) {
		cout << " - RESET Latency = " << TO_SECOND(bank->resetLatency) << endl;
		if (inputParameter->routingMode == h_tree)
			cout << " |--- H-Tree Latency = " << TO_SECOND(bank->resetLatency - bank->mat.resetLatency) << endl;
		else
			cout << " |--- Non-H-Tree Latency = " << TO_SECOND(bank->resetLatency - bank->mat.resetLatency) << endl;
		cout << " |--- Mat Latency    = " << TO_SECOND(bank->mat.resetLatency) << endl;
		cout << "    |--- Predecoder Latency = " << TO_SECOND(bank->mat.predecoderLatency) << endl;
		cout << "    |--- Subarray Latency   = " << TO_SECOND(bank->mat.subarray.resetLatency) << endl;
		cout << "       |--- RESET Pulse Duration = " << TO_SECOND(cell->resetPulse) << endl;
		cout << "       |--- Row Decoder Latency  = " << TO_SECOND(bank->mat.subarray.rowDecoder.writeLatency) << endl;
		cout << "       |--- Charge Latency   = " << TO_SECOND(bank->mat.subarray.chargeLatency) << endl;
		cout << " - SET Latency   = " << TO_SECOND(bank->setLatency) << endl;
		if (inputParameter->routingMode == h_tree)
			cout << " |--- H-Tree Latency = " << TO_SECOND(bank->setLatency - bank->mat.setLatency) << endl;
		else
			cout << " |--- Non-H-Tree Latency = " << TO_SECOND(bank->setLatency - bank->mat.setLatency) << endl;
		cout << " |--- Mat Latency    = " << TO_SECOND(bank->mat.setLatency) << endl;
		cout << "    |--- Predecoder Latency = " << TO_SECOND(bank->mat.predecoderLatency) << endl;
		cout << "    |--- Subarray Latency   = " << TO_SECOND(bank->mat.subarray.setLatency) << endl;
		cout << "       |--- SET Pulse Duration   = " << TO_SECOND(cell->setPulse) << endl;
		cout << "       |--- Row Decoder Latency  = " << TO_SECOND(bank->mat.subarray.rowDecoder.writeLatency) << endl;
		cout << "       |--- Charger Latency      = " << TO_SECOND(bank->mat.subarray.chargeLatency) << endl;
	} else if (cell->memCellType == SLCNAND) {
		cout << " - Erase Latency = " << TO_SECOND(bank->resetLatency) << endl;
		if (inputParameter->routingMode == h_tree)
			cout << " |--- H-Tree Latency = " << TO_SECOND(bank->resetLatency - bank->mat.resetLatency) << endl;
		else
			cout << " |--- Non-H-Tree Latency = " << TO_SECOND(bank->resetLatency - bank->mat.resetLatency) << endl;
		cout << " |--- Mat Latency    = " << TO_SECOND(bank->mat.resetLatency) << endl;
		cout << " - Programming Latency   = " << TO_SECOND(bank->setLatency) << endl;
		if (inputParameter->routingMode == h_tree)
			cout << " |--- H-Tree Latency = " << TO_SECOND(bank->setLatency - bank->mat.setLatency) << endl;
		else
			cout << " |--- Non-H-Tree Latency = " << TO_SECOND(bank->setLatency - bank->mat.setLatency) << endl;
		cout << " |--- Mat Latency    = " << TO_SECOND(bank->mat.setLatency) << endl;
	} else {
		cout << " - Write Latency = " << TO_SECOND(bank->writeLatency) << endl;
		if (inputParameter->routingMode == h_tree)
			cout << " |--- H-Tree Latency = " << TO_SECOND(bank->writeLatency - bank->mat.writeLatency) << endl;
		else
			cout << " |--- Non-H-Tree Latency = " << TO_SECOND(bank->writeLatency - bank->mat.writeLatency) << endl;
		cout << " |--- Mat Latency    = " << TO_SECOND(bank->mat.writeLatency) << endl;
		cout << "    |--- Predecoder Latency = " << TO_SECOND(bank->mat.predecoderLatency) << endl;
		cout << "    |--- Subarray Latency   = " << TO_SECOND(bank->mat.subarray.writeLatency) << endl;
		if (cell->memCellType == MRAM)
			cout << "       |--- Write Pulse Duration = " << TO_SECOND(cell->resetPulse) << endl;	// MRAM reset/set is equal
		cout << "       |--- Row Decoder Latency = " << TO_SECOND(bank->mat.subarray.rowDecoder.writeLatency) << endl;
		cout << "       |--- Charge Latency      = " << TO_SECOND(bank->mat.subarray.chargeLatency) << endl;
		if (cell->memCellType != eDRAM3T333 && cell->memCellType != eDRAM3T)
			cout << "       |--- Bitline Latency     = " << TO_SECOND(bank->mat.subarray.bitlineDelay) << endl;
		else
			cout << "       |--- Bitline Latency     = " << TO_SECOND(bank->mat.subarray.bitlineDelayW) << endl;
		}

	//Qing: subarray buffer latency
	cout << "- Subarray Buf R/W Latency  = " << TO_SECOND(bank->mat.subarray.subarrayBuffer.readLatency) << endl;
	cout << "- Subarray Buf XOR Latency  = " << TO_SECOND(bank->mat.subarray.subarrayBuffer.xorLatency) << endl;
	//Qing.
    //bank->mat.subarray.PrintProperty();	
	double readBandwidth = (double)bank->blockSize /
			(bank->mat.subarray.readLatency - bank->mat.subarray.rowDecoder.readLatency
			+ bank->mat.subarray.precharger.readLatency) / 8;
	if (cell->memCellType == MLCCTT || cell->memCellType == MLCFeFET || cell->memCellType == MLCRRAM) {
            readBandwidth *= log2(cell->nLvl);
        }
        cout << " - Read Bandwidth  = " << TO_BPS(readBandwidth) << endl;

	double writeBandwidth = (double)bank->blockSize /
			(bank->mat.subarray.writeLatency) / 8;
	cout << " - Write Bandwidth = " << TO_BPS(writeBandwidth) << endl;

	cout << "Power:" << endl;

	cout << " -  Read Dynamic Energy = " << TO_JOULE(bank->readDynamicEnergy) << endl;
	if (inputParameter->routingMode == h_tree)
		cout << " |--- H-Tree Read Dynamic Energy = " << TO_JOULE(bank->readDynamicEnergy - bank->mat.readDynamicEnergy
													* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
													<< endl;
	else
		cout << " |--- Non-H-Tree Read Dynamic Energy = " << TO_JOULE(bank->readDynamicEnergy - bank->mat.readDynamicEnergy
													* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
													<< endl;
	cout << " |--- Mat Dynamic Energy    = " << TO_JOULE(bank->mat.readDynamicEnergy) << " per mat" << endl;
	cout << "    |--- Predecoder Dynamic Energy = " << TO_JOULE(bank->mat.readDynamicEnergy - bank->mat.subarray.readDynamicEnergy
														* bank->numActiveSubarrayPerRow * bank->numActiveSubarrayPerColumn)
														<< endl;
	cout << "    |--- Subarray Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.readDynamicEnergy) << " per active subarray" << endl;
	cout << "       |--- Row Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.rowDecoder.readDynamicEnergy) << endl;
	cout << "       |--- Mux Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.bitlineMuxDecoder.readDynamicEnergy
													+ bank->mat.subarray.senseAmpMuxLev1Decoder.readDynamicEnergy
													+ bank->mat.subarray.senseAmpMuxLev2Decoder.readDynamicEnergy) << endl;
	if (cell->memCellType == PCRAM || cell->memCellType == FBRAM || cell->memCellType == MRAM || cell->memCellType == memristor || cell->memCellType == FeFET || cell->memCellType == MLCFeFET || cell->memCellType == MLCRRAM) {
		cout << "       |--- Bitline & Cell Read Energy = " << TO_JOULE(bank->mat.subarray.cellReadEnergy) << endl;
	}
	if (inputParameter->internalSensing)
		cout << "       |--- Senseamp Dynamic Energy    = " << TO_JOULE(bank->mat.subarray.senseAmp.readDynamicEnergy) << endl;
	cout << "       |--- Mux Dynamic Energy         = " << TO_JOULE(bank->mat.subarray.bitlineMux.readDynamicEnergy
													+ bank->mat.subarray.senseAmpMuxLev1.readDynamicEnergy
													+ bank->mat.subarray.senseAmpMuxLev2.readDynamicEnergy) << endl;
	cout << "       |--- Precharge Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.precharger.readDynamicEnergy) << endl;
	
	if (cell->memCellType == PCRAM || cell->memCellType == FBRAM || cell->memCellType == FeFET || cell->memCellType == MLCFeFET || cell->memCellType == MLCRRAM ||
			(cell->memCellType == memristor && (cell->accessType == CMOS_access || cell->accessType == BJT_access))) {
		cout << " - RESET Dynamic Energy = " << TO_JOULE(bank->resetDynamicEnergy) << endl;
		if (inputParameter->routingMode == h_tree)
			cout << " |--- H-Tree Write Dynamic Energy = " << TO_JOULE(bank->resetDynamicEnergy - bank->mat.resetDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< endl;
		else
			cout << " |--- H-Tree Write Dynamic Energy = " << TO_JOULE(bank->resetDynamicEnergy - bank->mat.resetDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< endl;
		cout << " |--- Mat Dynamic Energy    = " << TO_JOULE(bank->mat.resetDynamicEnergy) << " per mat" << endl;
		cout << "    |--- Predecoder Dynamic Energy = " << TO_JOULE(bank->mat.writeDynamicEnergy - bank->mat.subarray.writeDynamicEnergy
															* bank->numActiveSubarrayPerRow * bank->numActiveSubarrayPerColumn)
															<< endl;
		cout << "    |--- Subarray Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.writeDynamicEnergy) << " per active subarray" << endl;
		cout << "       |--- Row Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.rowDecoder.writeDynamicEnergy) << endl;
		cout << "       |--- Mux Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.bitlineMuxDecoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1Decoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2Decoder.writeDynamicEnergy) << endl;
		cout << "       |--- Mux Dynamic Energy         = " << TO_JOULE(bank->mat.subarray.bitlineMux.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2.writeDynamicEnergy) << endl;
		cout << "       |--- Cell RESET Dynamic Energy  = " << TO_JOULE(bank->mat.subarray.cellResetEnergy) << endl;
		cout << " - SET Dynamic Energy = " << TO_JOULE(bank->setDynamicEnergy) << endl;
		if (inputParameter->routingMode == h_tree)
			cout << " |--- H-Tree Write Dynamic Energy = " << TO_JOULE(bank->setDynamicEnergy - bank->mat.setDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< endl;
		else
			cout << " |--- Non-H-Tree Write Dynamic Energy = " << TO_JOULE(bank->setDynamicEnergy - bank->mat.setDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< endl;
		cout << " |--- Mat Dynamic Energy    = " << TO_JOULE(bank->mat.setDynamicEnergy) << " per mat" << endl;
		cout << "    |--- Predecoder Dynamic Energy = " << TO_JOULE(bank->mat.writeDynamicEnergy - bank->mat.subarray.writeDynamicEnergy
															* bank->numActiveSubarrayPerRow * bank->numActiveSubarrayPerColumn)
															<< endl;
		cout << "    |--- Subarray Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.writeDynamicEnergy) << " per active subarray" << endl;
		cout << "       |--- Row Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.rowDecoder.writeDynamicEnergy) << endl;
		cout << "       |--- Mux Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.bitlineMuxDecoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1Decoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2Decoder.writeDynamicEnergy) << endl;
		cout << "       |--- Mux Dynamic Energy         = " << TO_JOULE(bank->mat.subarray.bitlineMux.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2.writeDynamicEnergy) << endl;
		cout << "       |--- Cell SET Dynamic Energy    = " << TO_JOULE(bank->mat.subarray.cellSetEnergy) << endl;
	} else if (cell->memCellType == SLCNAND) {
		cout << " - Erase Dynamic Energy = " << TO_JOULE(bank->resetDynamicEnergy) << " per block" << endl;
		if (inputParameter->routingMode == h_tree)
			cout << " |--- H-Tree Write Dynamic Energy = " << TO_JOULE(bank->resetDynamicEnergy - bank->mat.resetDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< endl;
		else
			cout << " |--- Non-H-Tree Write Dynamic Energy = " << TO_JOULE(bank->resetDynamicEnergy - bank->mat.resetDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< endl;
		cout << " |--- Mat Dynamic Energy    = " << TO_JOULE(bank->mat.resetDynamicEnergy) << " per mat" << endl;
		cout << "    |--- Predecoder Dynamic Energy = " << TO_JOULE(bank->mat.writeDynamicEnergy - bank->mat.subarray.writeDynamicEnergy
															* bank->numActiveSubarrayPerRow * bank->numActiveSubarrayPerColumn)
															<< endl;
		cout << "    |--- Subarray Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.writeDynamicEnergy) << " per active subarray" << endl;
		cout << "       |--- Row Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.rowDecoder.writeDynamicEnergy) << endl;
		cout << "       |--- Mux Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.bitlineMuxDecoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1Decoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2Decoder.writeDynamicEnergy) << endl;
		cout << "       |--- Mux Dynamic Energy         = " << TO_JOULE(bank->mat.subarray.bitlineMux.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2.writeDynamicEnergy) << endl;
		cout << " - Programming Dynamic Energy = " << TO_JOULE(bank->setDynamicEnergy) << " per page" << endl;
		if (inputParameter->routingMode == h_tree)
			cout << " |--- H-Tree Write Dynamic Energy = " << TO_JOULE(bank->setDynamicEnergy - bank->mat.setDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< endl;
		else
			cout << " |--- Non-H-Tree Write Dynamic Energy = " << TO_JOULE(bank->setDynamicEnergy - bank->mat.setDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< endl;
		cout << " |--- Mat Dynamic Energy    = " << TO_JOULE(bank->mat.setDynamicEnergy) << " per mat" << endl;
		cout << "    |--- Predecoder Dynamic Energy = " << TO_JOULE(bank->mat.writeDynamicEnergy - bank->mat.subarray.writeDynamicEnergy
															* bank->numActiveSubarrayPerRow * bank->numActiveSubarrayPerColumn)
															<< endl;
		cout << "    |--- Subarray Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.writeDynamicEnergy) << " per active subarray" << endl;
		cout << "       |--- Row Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.rowDecoder.writeDynamicEnergy) << endl;
		cout << "       |--- Mux Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.bitlineMuxDecoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1Decoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2Decoder.writeDynamicEnergy) << endl;
		cout << "       |--- Mux Dynamic Energy         = " << TO_JOULE(bank->mat.subarray.bitlineMux.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2.writeDynamicEnergy) << endl;
	} else {
		cout << " - Write Dynamic Energy = " << TO_JOULE(bank->writeDynamicEnergy) << endl;
		if (inputParameter->routingMode == h_tree)
			cout << " |--- H-Tree Write Dynamic Energy = " << TO_JOULE(bank->writeDynamicEnergy - bank->mat.writeDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< endl;
		else
			cout << " |--- Non-H-Tree Write Dynamic Energy = " << TO_JOULE(bank->writeDynamicEnergy - bank->mat.writeDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< endl;
		cout << " |--- Mat Dynamic Energy    = " << TO_JOULE(bank->mat.writeDynamicEnergy) << " per mat" << endl;
		cout << "    |--- Predecoder Dynamic Energy = " << TO_JOULE(bank->mat.writeDynamicEnergy - bank->mat.subarray.writeDynamicEnergy
															* bank->numActiveSubarrayPerRow * bank->numActiveSubarrayPerColumn)
															<< endl;
		cout << "    |--- Subarray Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.writeDynamicEnergy) << " per active subarray" << endl;
		cout << "       |--- Row Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.rowDecoder.writeDynamicEnergy) << endl;
		cout << "       |--- Mux Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.bitlineMuxDecoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1Decoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2Decoder.writeDynamicEnergy) << endl;
		cout << "       |--- Mux Dynamic Energy         = " << TO_JOULE(bank->mat.subarray.bitlineMux.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2.writeDynamicEnergy) << endl;
		if (cell->memCellType == MRAM) {
			cout << "       |--- Bitline & Cell Write Energy= " << TO_JOULE(bank->mat.subarray.cellResetEnergy) << endl;
		}
	}

	//Qing: subarray buffer energy
	cout << "- Subarray Buf R/W Energy = " << TO_JOULE(bank->mat.subarray.subarrayBuffer.readDynamicEnergy) << endl;
	cout << "- Subarray Buf XOR Energy = " << TO_JOULE(bank->mat.subarray.subarrayBuffer.xorDynamicEnergy) << endl;
	//Qing.
	
	cout << " - Leakage Power = " << TO_WATT(bank->leakage) << endl;
	if (inputParameter->routingMode == h_tree)
		cout << " |--- H-Tree Leakage Power = " << TO_WATT(bank->leakage - bank->mat.leakage
													* bank->numColumnMat * bank->numRowMat)
													<< endl;
	else
		cout << " |--- Non-H-Tree Leakage Power = " << TO_WATT(bank->leakage - bank->mat.leakage
													* bank->numColumnMat * bank->numRowMat)
													<< endl;
	cout << " |--- Mat Leakage Power    = " << TO_WATT(bank->mat.leakage) << " per mat" << endl;
	if (cell->memCellType == eDRAM || cell->memCellType == eDRAM3T || cell->memCellType == eDRAM3T333) {
		// David Note: refresh period could be shorter than retention time 
        cout << " - Refresh Power = " << TO_WATT(bank->refreshDynamicEnergy / (cell->retentionTime)) << endl;
    }
}


void Result::printAsCache(Result &tagResult, CacheAccessMode cacheAccessMode) {
	if (bank->memoryType != dataT || tagResult.bank->memoryType != tag) {
		cout << "This is not a valid cache configuration." << endl;
		return;
	} else {
		double cacheHitLatency, cacheMissLatency, cacheWriteLatency;
		double cacheHitDynamicEnergy, cacheMissDynamicEnergy, cacheWriteDynamicEnergy;
		double cacheLeakage;
		double cacheArea;
		if (cacheAccessMode == normal_access_mode) {
			/* Calculate latencies */
			cacheMissLatency = tagResult.bank->readLatency;		/* only the tag access latency */
			cacheHitLatency = MAX(tagResult.bank->readLatency, bank->mat.readLatency);	/* access tag and activate data row in parallel */
			cacheHitLatency += bank->mat.subarray.columnDecoderLatency;		/* add column decoder latency after hit signal arrives */
			cacheHitLatency += bank->readLatency - bank->mat.readLatency;	/* H-tree in and out latency */
			cacheWriteLatency = MAX(tagResult.bank->writeLatency, bank->writeLatency);	/* Data and tag are written in parallel */
			/* Calculate power */
			cacheMissDynamicEnergy = tagResult.bank->readDynamicEnergy;	/* no matter what tag is always accessed */
			cacheMissDynamicEnergy += bank->readDynamicEnergy;	/* data is also partially accessed, TO-DO: not accurate here */
			cacheHitDynamicEnergy = tagResult.bank->readDynamicEnergy + bank->readDynamicEnergy;
			cacheWriteDynamicEnergy = tagResult.bank->writeDynamicEnergy + bank->writeDynamicEnergy;
		} else if (cacheAccessMode == fast_access_mode) {
			/* Calculate latencies */
			cacheMissLatency = tagResult.bank->readLatency;
			cacheHitLatency = MAX(tagResult.bank->readLatency, bank->readLatency);
			cacheWriteLatency = MAX(tagResult.bank->writeLatency, bank->writeLatency);
			/* Calculate power */
			cacheMissDynamicEnergy = tagResult.bank->readDynamicEnergy;	/* no matter what tag is always accessed */
			cacheMissDynamicEnergy += bank->readDynamicEnergy;	/* data is also partially accessed, TO-DO: not accurate here */
			cacheHitDynamicEnergy = tagResult.bank->readDynamicEnergy + bank->readDynamicEnergy;
			cacheWriteDynamicEnergy = tagResult.bank->writeDynamicEnergy + bank->writeDynamicEnergy;
		} else {		/* sequential access */
			/* Calculate latencies */
			cacheMissLatency = tagResult.bank->readLatency;
			cacheHitLatency = tagResult.bank->readLatency + bank->readLatency;
			cacheWriteLatency = MAX(tagResult.bank->writeLatency, bank->writeLatency);
			/* Calculate power */
			cacheMissDynamicEnergy = tagResult.bank->readDynamicEnergy;	/* no matter what tag is always accessed */
			cacheHitDynamicEnergy = tagResult.bank->readDynamicEnergy + bank->readDynamicEnergy;
			cacheWriteDynamicEnergy = tagResult.bank->writeDynamicEnergy + bank->writeDynamicEnergy;
		}
		/* Calculate leakage */
		cacheLeakage = tagResult.bank->leakage + bank->leakage;
		/* Calculate area */
		cacheArea = tagResult.bank->area + bank->area;	/* TO-DO: simply add them together here */

		/* start printing */
		cout << endl << "=======================" << endl << "CACHE DESIGN -- SUMMARY" << endl << "=======================" << endl;
		cout << "Access Mode: ";
		switch (cacheAccessMode) {
		case normal_access_mode:
			cout << "Normal" << endl;
			break;
		case fast_access_mode:
			cout << "Fast" << endl;
			break;
		default:	/* sequential */
			cout << "Sequential" << endl;
		}
		cout << "Area:" << endl;
		cout << " - Total Area = " << cacheArea * 1e6 << "mm^2" << endl;
		cout << " |--- Data Array Area = " << bank->height * 1e6 << "um x " << bank->width * 1e6 << "um = " << bank->area * 1e6 << "mm^2" << endl;
		cout << " |--- Tag Array Area  = " << tagResult.bank->height * 1e6 << "um x " << tagResult.bank->width * 1e6 << "um = " << tagResult.bank->area * 1e6 << "mm^2" << endl;
		cout << "Timing:" << endl;
		cout << " - Cache Hit Latency   = " << cacheHitLatency * 1e9 << "ns" << endl;
		cout << " - Cache Miss Latency  = " << cacheMissLatency * 1e9 << "ns" << endl;
		cout << " - Cache Write Latency = " << cacheWriteLatency * 1e9 << "ns" << endl;
        if (cell->memCellType == eDRAM) {
            cout << " - Cache Refresh Latency = " << MAX(tagResult.bank->refreshLatency, bank->refreshLatency) * 1e6 << "us per bank" << endl;
            cout << " - Cache Availability = " << ((cell->retentionTime - MAX(tagResult.bank->refreshLatency, bank->refreshLatency)) / cell->retentionTime) * 100.0 << "%" << endl;
        }
		cout << "Power:" << endl;
		cout << " - Cache Hit Dynamic Energy   = " << cacheHitDynamicEnergy * 1e9 << "nJ per access" << endl;
		cout << " - Cache Miss Dynamic Energy  = " << cacheMissDynamicEnergy * 1e9 << "nJ per access" << endl;
		cout << " - Cache Write Dynamic Energy = " << cacheWriteDynamicEnergy * 1e9 << "nJ per access" << endl;
        if (cell->memCellType == eDRAM) {
            cout << " - Cache Refresh Dynamic Energy = " << (tagResult.bank->refreshDynamicEnergy + bank->refreshDynamicEnergy) * 1e9 << "nJ per bank" << endl;
        }
		cout << " - Cache Total Leakage Power  = " << cacheLeakage * 1e3 << "mW" << endl;
		cout << " |--- Cache Data Array Leakage Power = " << bank->leakage * 1e3 << "mW" << endl;
		cout << " |--- Cache Tag Array Leakage Power  = " << tagResult.bank->leakage * 1e3 << "mW" << endl;
		if (cell->memCellType == eDRAM || cell->memCellType == eDRAM3T || cell->memCellType == eDRAM3T333) {
            cout << " - Cache Refresh Power = " << TO_WATT(bank->refreshDynamicEnergy / (cell->retentionTime)) << " per bank" << endl;
			cout << " - Cache Retention Time = " << (cell->retentionTime)*1e9 << "ns" << endl;
        }
		cout << endl << "CACHE DATA ARRAY";
		print();
		cout << endl << "CACHE TAG ARRAY";
		tagResult.print();
	}
}

YAML::Node Result::toYamlNode() {
	YAML::Node result;

	if(inputParameter->designTarget != cache){
		// Helper to convert DeviceRoadmap enums to string
		auto roadmapToString = [](DeviceRoadmap roadmap) -> std::string {
			switch (roadmap) {
				case HP: return "HP";
				case LSTP: return "LSTP";
				case LOP: return "LOP";
				default: return "ULP";
			}
		};

		// Memory cell type
		switch (cell->memCellType) {
			case SRAM: result["MemoryCell"]["MemoryCellType"] = "SRAM"; break;
			case DRAM: result["MemoryCell"]["MemoryCellType"] = "DRAM"; break;
			case eDRAM: result["MemoryCell"]["MemoryCellType"] = "eDRAM"; break;
			case eDRAM3T: result["MemoryCell"]["MemoryCellType"] = "3T eDRAM"; break;
			case eDRAM3T333: result["MemoryCell"]["MemoryCellType"] = "333eDRAM"; break;
			case MRAM: result["MemoryCell"]["MemoryCellType"] = "MRAM (Magnetoresistive)"; break;
			case PCRAM: result["MemoryCell"]["MemoryCellType"] = "PCRAM (Phase-Change)"; break;
			case memristor: result["MemoryCell"]["MemoryCellType"] = "RRAM (Memristor)"; break;
			case FBRAM: result["MemoryCell"]["MemoryCellType"] = "FBRAM (Floating Body)"; break;
			case SLCNAND: result["MemoryCell"]["MemoryCellType"] = "Single-Level Cell NAND Flash"; break;
			case MLCNAND: result["MemoryCell"]["MemoryCellType"] = "Multi-Level Cell NAND Flash"; break;
			case CTT: result["MemoryCell"]["MemoryCellType"] = "Single-Level Cell CTT"; break;
			case MLCCTT: result["MemoryCell"]["MemoryCellType"] = "Multi-Level Cell CTT"; break;
			case FeFET: result["MemoryCell"]["MemoryCellType"] = "Single-Level Cell FeFET"; break;
			case MLCFeFET: result["MemoryCell"]["MemoryCellType"] = "Multi-Level Cell FeFET"; break;
			case MLCRRAM: result["MemoryCell"]["MemoryCellType"] = "Multi-Level Cell RRAM (Memristor)"; break;
			default: result["MemoryCell"]["MemoryCellType"] = "Unknown"; break;
		}


		// Cell area
		result["MemoryCell"]["CellArea_F2"]  = cell->area;
		result["MemoryCell"]["CellArea_um2"] = cell->area / 1000000.0 * tech->featureSizeInNano * tech->featureSizeInNano;
		result["MemoryCell"]["AspectRatio"]  = cell->aspectRatio;

		// Resistive / Non-volatile memory
		if (cell->memCellType == PCRAM || cell->memCellType == MRAM || cell->memCellType == memristor ||
			cell->memCellType == FBRAM || cell->memCellType == FeFET || cell->memCellType == MLCFeFET ||
			cell->memCellType == MLCRRAM) {

			if (cell->resistanceOn < 1e3)
				result["MemoryCell"]["R_on_Ohm"] = cell->resistanceOn;
			else if (cell->resistanceOn < 1e6)
				result["MemoryCell"]["R_on_KOhm"] = cell->resistanceOn / 1e3;
			else
				result["MemoryCell"]["R_on_MOhm"] = cell->resistanceOn / 1e6;

			if (cell->resistanceOff < 1e3)
				result["MemoryCell"]["R_off_Ohm"] = cell->resistanceOff;
			else if (cell->resistanceOff < 1e6)
				result["MemoryCell"]["R_off_KOhm"] = cell->resistanceOff / 1e3;
			else
				result["MemoryCell"]["R_off_MOhm"] = cell->resistanceOff / 1e6;

			result["MemoryCell"]["ReadMode"]  = cell->readMode ? "Voltage-Sensing" : "Current-Sensing";
			if (cell->readCurrent > 0) result["MemoryCell"]["ReadCurrent_uA"] = cell->readCurrent * 1e6;
			if (cell->readVoltage > 0) result["MemoryCell"]["ReadVoltage_V"] = cell->readVoltage;

			result["MemoryCell"]["ResetMode"] = cell->resetMode ? "Voltage" : "Current";
			result["MemoryCell"]["ResetVoltage_V"] = cell->resetVoltage;
			result["MemoryCell"]["ResetCurrent_uA"] = cell->resetCurrent * 1e6;
			result["MemoryCell"]["ResetPulse_s"] = cell->resetPulse / 1e9;

			result["MemoryCell"]["SetMode"] = cell->setMode ? "Voltage" : "Current";
			result["MemoryCell"]["SetVoltage_V"] = cell->setVoltage;
			result["MemoryCell"]["SetCurrent_uA"] = cell->setCurrent * 1e6;
			result["MemoryCell"]["SetPulse_s"] = cell->setPulse / 1e9;

			switch (cell->accessType) {
				case CMOS_access: result["MemoryCell"]["AccessType"] = "CMOS"; break;
				case BJT_access: result["MemoryCell"]["AccessType"] = "BJT"; break;
				case diode_access: result["MemoryCell"]["AccessType"] = "Diode"; break;
				default: result["MemoryCell"]["AccessType"] = "None Access Device"; break;
			}
		}

		// SRAM
		if (cell->memCellType == SRAM) {
			result["MemoryCell"]["WidthAccessCMOS_F"]   = cell->widthAccessCMOS;
			result["MemoryCell"]["WidthSRAMCellNMOS_F"] = cell->widthSRAMCellNMOS;
			result["MemoryCell"]["WidthSRAMCellPMOS_F"] = cell->widthSRAMCellPMOS;
			result["MemoryCell"]["PeripheralRoadmap"]   = roadmapToString(tech->deviceRoadmap);
			result["MemoryCell"]["PeripheralNode_nm"]   = tech->featureSizeInNano;
			result["MemoryCell"]["VDD_V"]               = tech->vdd;
			result["MemoryCell"]["WWL_SWING"]           = tech->vdd;
			result["MemoryCell"]["Temperature_K"]       = cell->temperature;
		}

		// DRAM / eDRAM
		if (cell->memCellType == DRAM || cell->memCellType == eDRAM) {
			result["MemoryCell"]["WidthAccessCMOS_F"] = cell->widthAccessCMOS;
			result["MemoryCell"]["PeripheralRoadmap"] = roadmapToString(tech->deviceRoadmap);
			result["MemoryCell"]["PeripheralNode_nm"] = tech->featureSizeInNano;
			result["MemoryCell"]["VDD_V"] = tech->vdd;
			result["MemoryCell"]["WWL_SWING"] = tech->vpp;
			result["MemoryCell"]["Temperature_K"] = cell->temperature;
		}

		// 3T DRAM
		if (cell->memCellType == eDRAM3T || cell->memCellType == eDRAM3T333) {
			result["MemoryCell"]["WidthWriteAccessCMOS_F"] = cell->widthAccessCMOS;
			result["MemoryCell"]["WidthReadAccessCMOS_F"]  = cell->widthAccessCMOSR;
			result["MemoryCell"]["PeripheralRoadmap"]      = roadmapToString(tech->deviceRoadmap);
			result["MemoryCell"]["WriteAccessRoadmap"]     = roadmapToString(techW->deviceRoadmap);
			result["MemoryCell"]["ReadAccessRoadmap"]      = roadmapToString(techR->deviceRoadmap);
			result["MemoryCell"]["PeripheralNode_nm"]      = tech->featureSizeInNano;
			result["MemoryCell"]["WriteAccessNode_nm"]     = techW->featureSizeInNano;
			result["MemoryCell"]["ReadAccessNode_nm"]      = techR->featureSizeInNano;
			result["MemoryCell"]["VDD_V"]                  = tech->vdd;
			result["MemoryCell"]["WWL_SWING"]              = techW->vpp;
			result["MemoryCell"]["Temperature_K"]          = cell->temperature;
		}

		// SLC NAND Flash
		if (cell->memCellType == SLCNAND) {
			result["MemoryCell"]["PassVoltage_V"]     = cell->flashPassVoltage;
			result["MemoryCell"]["ProgramVoltage_V"]  = cell->flashProgramVoltage;
			result["MemoryCell"]["EraseVoltage_V"]    = cell->flashEraseVoltage;
			result["MemoryCell"]["ProgramTime_s"]     = cell->flashProgramTime / 1e9;
			result["MemoryCell"]["EraseTime_s"]       = cell->flashEraseTime / 1e9;
			result["MemoryCell"]["GateCouplingRatio"] = cell->gateCouplingRatio;
		}

		// Multi-level cells
		if (cell->memCellType == MLCCTT || cell->memCellType == MLCFeFET || cell->memCellType == MLCRRAM) {
			result["MemoryCell"]["NumberOfInputFingers"]   = cell->nFingers;
			result["MemoryCell"]["NumberOfLevelsPerCell"] = cell->nLvl;
		}

	}

	if(inputParameter->designTarget != cache){
		//Capacity
		if (inputParameter->capacity < 1024) {
			result["Capacity"]["Value"] = inputParameter->capacity;
			result["Capacity"]["Unit"] = "B";
		} else if (inputParameter->capacity < 1024 * 1024) {
			result["Capacity"]["Value"] = inputParameter->capacity / 1024;
			result["Capacity"]["Unit"] = "KB";
		} else if (inputParameter->capacity < 1024 * 1024 * 1024) {
			result["Capacity"]["Value"] = inputParameter->capacity / 1024 / 1024;
			result["Capacity"]["Unit"] = "MB";
		} else {
			result["Capacity"]["Value"] = inputParameter->capacity / 1024 / 1024 / 1024;
			result["Capacity"]["Unit"] = "GB";
		}
	


		switch (optimizationTarget) {
			case read_latency_optimized: result["OptimizationTarget"] = "ReadLatency"; break;
			case write_latency_optimized: result["OptimizationTarget"] = "WriteLatency"; break;
			case read_energy_optimized: result["OptimizationTarget"] = "ReadDynamicEnergy"; break;
			case write_energy_optimized: result["OptimizationTarget"] = "WriteDynamicEnergy"; break;
			case read_edp_optimized: result["OptimizationTarget"] = "ReadEDP"; break;
			case write_edp_optimized: result["OptimizationTarget"] = "WriteEDP"; break;
			case leakage_optimized: result["OptimizationTarget"] = "LeakagePower"; break;
			case area_optimized: result["OptimizationTarget"] = "Area"; break;
			default:                 result["OptimizationTarget"] = "Unknown";
		}
	}

    // Configuration
    result["Configuration"]["BankOrganization"]["Rows"] = bank->numRowMat;
    result["Configuration"]["BankOrganization"]["Columns"] = bank->numColumnMat;
    result["Configuration"]["BankOrganization"]["RowActivation"] = bank->numActiveMatPerColumn;
    result["Configuration"]["BankOrganization"]["TotalRows"] = bank->numRowMat;
    result["Configuration"]["BankOrganization"]["ColumnActivation"] = bank->numActiveMatPerRow;
    result["Configuration"]["BankOrganization"]["TotalColumns"] = bank->numColumnMat;
    
    result["Configuration"]["MatOrganization"]["Rows"] = bank->numRowSubarray;
    result["Configuration"]["MatOrganization"]["Columns"] = bank->numColumnSubarray;
    result["Configuration"]["MatOrganization"]["RowActivation"] = bank->numActiveSubarrayPerColumn;
    result["Configuration"]["MatOrganization"]["TotalRows"] = bank->numRowSubarray;
    result["Configuration"]["MatOrganization"]["ColumnActivation"] = bank->numActiveSubarrayPerRow;
    result["Configuration"]["MatOrganization"]["TotalColumns"] = bank->numColumnSubarray;
    result["Configuration"]["MatOrganization"]["SubarrayRows"] = bank->mat.subarray.numRow;
    result["Configuration"]["MatOrganization"]["SubarrayColumns"] = bank->mat.subarray.numColumn;
    
    result["Configuration"]["MuxLevels"]["SenseampMux"] = bank->muxSenseAmp;
    result["Configuration"]["MuxLevels"]["OutputLevel1Mux"] = bank->muxOutputLev1;
    result["Configuration"]["MuxLevels"]["OutputLevel2Mux"] = bank->muxOutputLev2;
    if (inputParameter->designTarget == cache)
        result["Configuration"]["MuxLevels"]["RowsPerSet"] = bank->numRowPerSet;
    
    // Local Wire
    switch (localWire->wireType) {
        case local_aggressive: result["Configuration"]["LocalWire"]["WireType"] = "LocalAggressive"; break;
        case local_conservative: result["Configuration"]["LocalWire"]["WireType"] = "LocalConservative"; break;
        case semi_aggressive: result["Configuration"]["LocalWire"]["WireType"] = "SemiAggressive"; break;
        case semi_conservative: result["Configuration"]["LocalWire"]["WireType"] = "SemiConservative"; break;
        case global_aggressive: result["Configuration"]["LocalWire"]["WireType"] = "GlobalAggressive"; break;
        case global_conservative: result["Configuration"]["LocalWire"]["WireType"] = "GlobalConservative"; break;
        default: result["Configuration"]["LocalWire"]["WireType"] = "DRAMWire";
    }
    switch (localWire->wireRepeaterType) {
        case repeated_none: result["Configuration"]["LocalWire"]["RepeaterType"] = "NoRepeaters"; break;
        case repeated_opt: result["Configuration"]["LocalWire"]["RepeaterType"] = "FullyOptimized"; break;
        case repeated_5: result["Configuration"]["LocalWire"]["RepeaterType"] = "Repeated5Percent"; break;
        case repeated_10: result["Configuration"]["LocalWire"]["RepeaterType"] = "Repeated10Percent"; break;
        case repeated_20: result["Configuration"]["LocalWire"]["RepeaterType"] = "Repeated20Percent"; break;
        case repeated_30: result["Configuration"]["LocalWire"]["RepeaterType"] = "Repeated30Percent"; break;
        case repeated_40: result["Configuration"]["LocalWire"]["RepeaterType"] = "Repeated40Percent"; break;
        case repeated_50: result["Configuration"]["LocalWire"]["RepeaterType"] = "Repeated50Percent"; break;
        default: result["Configuration"]["LocalWire"]["RepeaterType"] = "Unknown";
    }
    result["Configuration"]["LocalWire"]["LowSwing"] = localWire->isLowSwing ? "Yes" : "No";
    
    // Global Wire
    switch (globalWire->wireType) {
        case local_aggressive: result["Configuration"]["GlobalWire"]["WireType"] = "LocalAggressive"; break;
        case local_conservative: result["Configuration"]["GlobalWire"]["WireType"] = "LocalConservative"; break;
        case semi_aggressive: result["Configuration"]["GlobalWire"]["WireType"] = "SemiAggressive"; break;
        case semi_conservative: result["Configuration"]["GlobalWire"]["WireType"] = "SemiConservative"; break;
        case global_aggressive: result["Configuration"]["GlobalWire"]["WireType"] = "GlobalAggressive"; break;
        case global_conservative: result["Configuration"]["GlobalWire"]["WireType"] = "GlobalConservative"; break;
        default: result["Configuration"]["GlobalWire"]["WireType"] = "DRAMWire";
    }
    switch (globalWire->wireRepeaterType) {
        case repeated_none: result["Configuration"]["GlobalWire"]["RepeaterType"] = "NoRepeaters"; break;
        case repeated_opt: result["Configuration"]["GlobalWire"]["RepeaterType"] = "FullyOptimized"; break;
        case repeated_5: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Repeated5Percent"; break;
        case repeated_10: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Repeated10Percent"; break;
        case repeated_20: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Repeated20Percent"; break;
        case repeated_30: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Repeated30Percent"; break;
        case repeated_40: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Repeated40Percent"; break;
        case repeated_50: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Repeated50Percent"; break;
        default: result["Configuration"]["GlobalWire"]["RepeaterType"] = "Unknown";
    }
    result["Configuration"]["GlobalWire"]["LowSwing"] = globalWire->isLowSwing ? "Yes" : "No";
    
    switch (bank->areaOptimizationLevel) {
        case latency_first: result["Configuration"]["BufferDesignStyle"] = "LatencyOptimized"; break;
        case area_first: result["Configuration"]["BufferDesignStyle"] = "AreaOptimized"; break;
        default: result["Configuration"]["BufferDesignStyle"] = "Balanced";
    }
    
    // Area
    result["Results"]["Area"]["Total"]["Height_um"] = bank->height * 1e6;
    result["Results"]["Area"]["Total"]["Width_um"] = bank->width * 1e6;
    result["Results"]["Area"]["Total"]["Area_mm2"] = bank->area * 1e6;
    
    result["Results"]["Area"]["Mat"]["Height_um"] = bank->mat.height * 1e6;
    result["Results"]["Area"]["Mat"]["Width_um"] = bank->mat.width * 1e6;
    result["Results"]["Area"]["Mat"]["Area_mm2"] = bank->mat.area * 1e6;
    result["Results"]["Area"]["Mat"]["Efficiency_percent"] = 
        (cell->area * tech->featureSize * tech->featureSize * bank->capacity / 
         bank->numRowMat / bank->numColumnMat / bank->mat.area * 100);
    
    result["Results"]["Area"]["Subarray"]["Height_um"] = bank->mat.subarray.height * 1e6;
    result["Results"]["Area"]["Subarray"]["Width_um"] = bank->mat.subarray.width * 1e6;
    result["Results"]["Area"]["Subarray"]["Area_mm2"] = bank->mat.subarray.area * 1e6;
    result["Results"]["Area"]["Subarray"]["Efficiency_percent"] =
        (cell->area * tech->featureSize * tech->featureSize * bank->capacity / 
         bank->numRowMat / bank->numColumnMat / bank->numRowSubarray / 
         bank->numColumnSubarray / bank->mat.subarray.area * 100);
    
    result["Results"]["Area"]["AreaEfficiency_percent"] =
        (cell->area * tech->featureSize * tech->featureSize * bank->capacity / bank->area * 100);
    
    // Timing
    result["Results"]["Timing"]["Read"]["Latency_ns"] = bank->readLatency * 1e9;
    result["Results"]["Timing"]["Read"]["TreeLatency_ns"] = 
        (bank->readLatency - bank->mat.readLatency) * 1e9;
    result["Results"]["Timing"]["Read"]["MatLatency_ns"] = bank->mat.readLatency * 1e9;
    result["Results"]["Timing"]["Read"]["PredecoderLatency_ns"] = bank->mat.predecoderLatency * 1e9;
    result["Results"]["Timing"]["Read"]["SubarrayLatency_ns"] = bank->mat.subarray.readLatency * 1e9;
    result["Results"]["Timing"]["Read"]["RowDecoderLatency_ns"] = 
        bank->mat.subarray.rowDecoder.readLatency * 1e9;
	if (cell->memCellType == eDRAM3T333 || cell->memCellType == eDRAM3T) {
    	result["Results"]["Timing"]["Read"]["BitlineLatency_ns"] = bank->mat.subarray.bitlineDelayR * 1e9;
	} else {
		result["Results"]["Timing"]["Read"]["BitlineLatency_ns"] = bank->mat.subarray.bitlineDelay * 1e9;
	}
    if (inputParameter->internalSensing)
        result["Results"]["Timing"]["Read"]["SenseampLatency_ns"] = 
            bank->mat.subarray.senseAmp.readLatency * 1e9;
    result["Results"]["Timing"]["Read"]["MuxLatency_ns"] =
        (bank->mat.subarray.bitlineMux.readLatency + 
         bank->mat.subarray.senseAmpMuxLev1.readLatency +
         bank->mat.subarray.senseAmpMuxLev2.readLatency) * 1e9;
    result["Results"]["Timing"]["Read"]["PrechargeLatency_ns"] = 
        bank->mat.subarray.precharger.readLatency * 1e9;

    if (cell->memCellType == PCRAM || cell->memCellType == FBRAM || 
        cell->memCellType == FeFET || cell->memCellType == MLCFeFET || 
        cell->memCellType == MLCRRAM ||
        (cell->memCellType == memristor && (cell->accessType == CMOS_access || 
         cell->accessType == BJT_access))) {

        // RESET latency with proper TreeLatency calculation
        result["Results"]["Timing"]["Reset"]["Latency_ns"] = bank->resetLatency * 1e9;
        result["Results"]["Timing"]["Reset"]["TreeLatency_ns"] = 
            (bank->resetLatency - bank->mat.resetLatency) * 1e9;
        result["Results"]["Timing"]["Reset"]["MatLatency_ns"] = bank->mat.resetLatency * 1e9;
        result["Results"]["Timing"]["Reset"]["PulseDuration_ns"] = cell->resetPulse * 1e9;

        // SET latency with proper TreeLatency calculation
        result["Results"]["Timing"]["Set"]["Latency_ns"] = bank->setLatency * 1e9;
        result["Results"]["Timing"]["Set"]["TreeLatency_ns"] = 
            (bank->setLatency - bank->mat.setLatency) * 1e9;
        result["Results"]["Timing"]["Set"]["MatLatency_ns"] = bank->mat.setLatency * 1e9;
        result["Results"]["Timing"]["Set"]["PulseDuration_ns"] = cell->setPulse * 1e9;

    } else if (cell->memCellType == SLCNAND) {
        result["Results"]["Timing"]["Erase"]["Latency_ns"] = bank->resetLatency * 1e9;
        result["Results"]["Timing"]["Programming"]["Latency_ns"] = bank->setLatency * 1e9;

    } else {
        result["Results"]["Timing"]["Write"]["Latency_ns"] = bank->writeLatency * 1e9;
        result["Results"]["Timing"]["Write"]["TreeLatency_ns"] = 
            (bank->writeLatency - bank->mat.writeLatency) * 1e9;
        result["Results"]["Timing"]["Write"]["MatLatency_ns"] = bank->mat.writeLatency * 1e9;
		result["Results"]["Timing"]["Write"]["PredecoderLatency_ns"] = bank->mat.predecoderLatency * 1e9;
		result["Results"]["Timing"]["Write"]["SubarrayLatency_ns"] = bank->mat.subarray.readLatency * 1e9;
		result["Results"]["Timing"]["Write"]["RowDecoderLatency_ns"] = 
			bank->mat.subarray.rowDecoder.readLatency * 1e9;
		if (cell->memCellType == eDRAM3T333 || cell->memCellType == eDRAM3T) {
			result["Results"]["Timing"]["Write"]["BitlineLatency_ns"] = bank->mat.subarray.bitlineDelayW * 1e9;
		} else {
			result["Results"]["Timing"]["Write"]["BitlineLatency_ns"] = bank->mat.subarray.bitlineDelay * 1e9;
		}
    }

    double readBandwidth = (double)bank->blockSize /
        (bank->mat.subarray.readLatency - bank->mat.subarray.rowDecoder.readLatency
         + bank->mat.subarray.precharger.readLatency) / 8;
    if (cell->memCellType == MLCCTT || cell->memCellType == MLCFeFET || 
        cell->memCellType == MLCRRAM) {
        readBandwidth *= log2(cell->nLvl);
    }
    result["Results"]["Timing"]["ReadBandwidth_Bps"] = readBandwidth;

    double writeBandwidth = (double)bank->blockSize / (bank->mat.subarray.writeLatency) / 8;
    result["Results"]["Timing"]["WriteBandwidth_Bps"] = writeBandwidth;

    // Power
    result["Results"]["Power"]["Read"]["DynamicEnergy_pJ"] = bank->readDynamicEnergy * 1e12;
    result["Results"]["Power"]["Read"]["TreeDynamicEnergy_pJ"] =
        (bank->readDynamicEnergy - bank->mat.readDynamicEnergy * 
         bank->numActiveMatPerColumn * bank->numActiveMatPerRow) * 1e12;
    result["Results"]["Power"]["Read"]["MatDynamicEnergy_pJ"] = 
        bank->mat.readDynamicEnergy * 1e12;
    result["Results"]["Power"]["Read"]["SubarrayDynamicEnergy_pJ"] = 
        bank->mat.subarray.readDynamicEnergy * 1e12;

    if (cell->memCellType == PCRAM || cell->memCellType == FBRAM || 
        cell->memCellType == FeFET || cell->memCellType == MLCFeFET || 
        cell->memCellType == MLCRRAM ||
        (cell->memCellType == memristor && (cell->accessType == CMOS_access || 
         cell->accessType == BJT_access))) {

        result["Results"]["Power"]["Reset"]["DynamicEnergy_pJ"] = bank->resetDynamicEnergy * 1e12;
        result["Results"]["Power"]["Reset"]["CellResetEnergy_pJ"] = 
            bank->mat.subarray.cellResetEnergy * 1e12;

        result["Results"]["Power"]["Set"]["DynamicEnergy_pJ"] = bank->setDynamicEnergy * 1e12;
        result["Results"]["Power"]["Set"]["CellSetEnergy_pJ"] = 
            bank->mat.subarray.cellSetEnergy * 1e12;

    } else if (cell->memCellType == SLCNAND) {
        result["Results"]["Power"]["Erase"]["DynamicEnergy_pJ"] = bank->resetDynamicEnergy * 1e12;
        result["Results"]["Power"]["Programming"]["DynamicEnergy_pJ"] = bank->setDynamicEnergy * 1e12;

    } else {
        result["Results"]["Power"]["Write"]["DynamicEnergy_pJ"] = bank->writeDynamicEnergy * 1e12;
    }

    result["Results"]["Power"]["Leakage_mW"] = bank->leakage * 1e3;

    if (cell->memCellType == eDRAM || cell->memCellType == eDRAM3T || 
        cell->memCellType == eDRAM3T333) {
        result["Results"]["Power"]["RefreshPower_W"] = 
            (bank->refreshDynamicEnergy / cell->retentionTime);
    }

    return result;
}

YAML::Node Result::toYamlNodeAsCache(Result &tagResult, CacheAccessMode cacheAccessMode) {
    if (bank->memoryType != dataT || tagResult.bank->memoryType != tag) {
        cout << "This is not a valid cache configuration." << endl;
        return YAML::Node();
    }

    YAML::Node result;

	// Helper to convert DeviceRoadmap enums to string
	auto roadmapToString = [](DeviceRoadmap roadmap) -> std::string {
		switch (roadmap) {
			case HP: return "HP";
			case LSTP: return "LSTP";
			case LOP: return "LOP";
			default: return "ULP";
		}
	};

	// Memory cell type
	switch (cell->memCellType) {
		case SRAM: result["MemoryCell"]["MemoryCellType"] = "SRAM"; break;
		case DRAM: result["MemoryCell"]["MemoryCellType"] = "DRAM"; break;
		case eDRAM: result["MemoryCell"]["MemoryCellType"] = "eDRAM"; break;
		case eDRAM3T: result["MemoryCell"]["MemoryCellType"] = "3T eDRAM"; break;
		case eDRAM3T333: result["MemoryCell"]["MemoryCellType"] = "333 eDRAM"; break;
		case MRAM: result["MemoryCell"]["MemoryCellType"] = "MRAM (Magnetoresistive)"; break;
		case PCRAM: result["MemoryCell"]["MemoryCellType"] = "PCRAM (Phase-Change)"; break;
		case memristor: result["MemoryCell"]["MemoryCellType"] = "RRAM (Memristor)"; break;
		case FBRAM: result["MemoryCell"]["MemoryCellType"] = "FBRAM (Floating Body)"; break;
		case SLCNAND: result["MemoryCell"]["MemoryCellType"] = "Single-Level Cell NAND Flash"; break;
		case MLCNAND: result["MemoryCell"]["MemoryCellType"] = "Multi-Level Cell NAND Flash"; break;
		case CTT: result["MemoryCell"]["MemoryCellType"] = "Single-Level Cell CTT"; break;
		case MLCCTT: result["MemoryCell"]["MemoryCellType"] = "Multi-Level Cell CTT"; break;
		case FeFET: result["MemoryCell"]["MemoryCellType"] = "Single-Level Cell FeFET"; break;
		case MLCFeFET: result["MemoryCell"]["MemoryCellType"] = "Multi-Level Cell FeFET"; break;
		case MLCRRAM: result["MemoryCell"]["MemoryCellType"] = "Multi-Level Cell RRAM (Memristor)"; break;
		default: result["MemoryCell"]["MemoryCellType"] = "Unknown"; break;
	}

	// Cell area
	result["MemoryCell"]["CellArea_F2"]  = cell->area;
	result["MemoryCell"]["CellArea_um2"] = cell->area / 1000000.0 * tech->featureSizeInNano * tech->featureSizeInNano;
	result["MemoryCell"]["AspectRatio"]  = cell->aspectRatio;

	// Resistive / Non-volatile memory
	if (cell->memCellType == PCRAM || cell->memCellType == MRAM || cell->memCellType == memristor ||
		cell->memCellType == FBRAM || cell->memCellType == FeFET || cell->memCellType == MLCFeFET ||
		cell->memCellType == MLCRRAM) {

		if (cell->resistanceOn < 1e3)
			result["MemoryCell"]["R_on_Ohm"] = cell->resistanceOn;
		else if (cell->resistanceOn < 1e6)
			result["MemoryCell"]["R_on_KOhm"] = cell->resistanceOn / 1e3;
		else
			result["MemoryCell"]["R_on_MOhm"] = cell->resistanceOn / 1e6;

		if (cell->resistanceOff < 1e3)
			result["MemoryCell"]["R_off_Ohm"] = cell->resistanceOff;
		else if (cell->resistanceOff < 1e6)
			result["MemoryCell"]["R_off_KOhm"] = cell->resistanceOff / 1e3;
		else
			result["MemoryCell"]["R_off_MOhm"] = cell->resistanceOff / 1e6;

		result["MemoryCell"]["ReadMode"]  = cell->readMode ? "Voltage-Sensing" : "Current-Sensing";
		if (cell->readCurrent > 0) result["MemoryCell"]["ReadCurrent_uA"] = cell->readCurrent * 1e6;
		if (cell->readVoltage > 0) result["MemoryCell"]["ReadVoltage_V"] = cell->readVoltage;

		result["MemoryCell"]["ResetMode"] = cell->resetMode ? "Voltage" : "Current";
		result["MemoryCell"]["ResetVoltage_V"] = cell->resetVoltage;
		result["MemoryCell"]["ResetCurrent_uA"] = cell->resetCurrent * 1e6;
		result["MemoryCell"]["ResetPulse_s"] = cell->resetPulse / 1e9;

		result["MemoryCell"]["SetMode"] = cell->setMode ? "Voltage" : "Current";
		result["MemoryCell"]["SetVoltage_V"] = cell->setVoltage;
		result["MemoryCell"]["SetCurrent_uA"] = cell->setCurrent * 1e6;
		result["MemoryCell"]["SetPulse_s"] = cell->setPulse / 1e9;

		switch (cell->accessType) {
			case CMOS_access: result["MemoryCell"]["AccessType"] = "CMOS"; break;
			case BJT_access: result["MemoryCell"]["AccessType"] = "BJT"; break;
			case diode_access: result["MemoryCell"]["AccessType"] = "Diode"; break;
			default: result["MemoryCell"]["AccessType"] = "None Access Device"; break;
		}
	}

	// SRAM
	if (cell->memCellType == SRAM) {
		result["MemoryCell"]["WidthAccessCMOS_F"]   = cell->widthAccessCMOS;
		result["MemoryCell"]["WidthSRAMCellNMOS_F"] = cell->widthSRAMCellNMOS;
		result["MemoryCell"]["WidthSRAMCellPMOS_F"] = cell->widthSRAMCellPMOS;
		result["MemoryCell"]["PeripheralRoadmap"]   = roadmapToString(tech->deviceRoadmap);
		result["MemoryCell"]["PeripheralNode_nm"]   = tech->featureSizeInNano;
		result["MemoryCell"]["VDD_V"]               = tech->vdd;
		result["MemoryCell"]["Temperature_K"]       = cell->temperature;
	}

	// DRAM / eDRAM
	if (cell->memCellType == DRAM || cell->memCellType == eDRAM) {
		result["MemoryCell"]["WidthAccessCMOS_F"] = cell->widthAccessCMOS;
		result["MemoryCell"]["PeripheralRoadmap"] = roadmapToString(tech->deviceRoadmap);
		result["MemoryCell"]["PeripheralNode_nm"] = tech->featureSizeInNano;
		result["MemoryCell"]["VDD_V"] = tech->vdd;
		result["MemoryCell"]["WL_SWING"] = tech->vpp;
		result["MemoryCell"]["Temperature_K"] = cell->temperature;
	}

	// 3T DRAM
	if (cell->memCellType == eDRAM3T || cell->memCellType == eDRAM3T333) {
		result["MemoryCell"]["WidthWriteAccessCMOS_F"] = cell->widthAccessCMOS;
		result["MemoryCell"]["WidthReadAccessCMOS_F"]  = cell->widthAccessCMOSR;
		result["MemoryCell"]["PeripheralRoadmap"]      = roadmapToString(tech->deviceRoadmap);
		result["MemoryCell"]["WriteAccessRoadmap"]     = roadmapToString(techW->deviceRoadmap);
		result["MemoryCell"]["ReadAccessRoadmap"]      = roadmapToString(techR->deviceRoadmap);
		result["MemoryCell"]["PeripheralNode_nm"]      = tech->featureSizeInNano;
		result["MemoryCell"]["WriteAccessNode_nm"]     = techW->featureSizeInNano;
		result["MemoryCell"]["ReadAccessNode_nm"]      = techR->featureSizeInNano;
		result["MemoryCell"]["VDD_V"]                  = tech->vdd;
		result["MemoryCell"]["WWL_SWING"]              = techW->vpp;
		result["MemoryCell"]["Temperature_K"]          = cell->temperature;
	}

	// SLC NAND Flash
	if (cell->memCellType == SLCNAND) {
		result["MemoryCell"]["PassVoltage_V"]     = cell->flashPassVoltage;
		result["MemoryCell"]["ProgramVoltage_V"]  = cell->flashProgramVoltage;
		result["MemoryCell"]["EraseVoltage_V"]    = cell->flashEraseVoltage;
		result["MemoryCell"]["ProgramTime_s"]     = cell->flashProgramTime / 1e9;
		result["MemoryCell"]["EraseTime_s"]       = cell->flashEraseTime / 1e9;
		result["MemoryCell"]["GateCouplingRatio"] = cell->gateCouplingRatio;
	}

	// Multi-level cells
	if (cell->memCellType == MLCCTT || cell->memCellType == MLCFeFET || cell->memCellType == MLCRRAM) {
		result["MemoryCell"]["NumberOfInputFingers"]   = cell->nFingers;
		result["MemoryCell"]["NumberOfLevelsPerCell"] = cell->nLvl;
	}

    // Calculate cache metrics
    double cacheHitLatency, cacheMissLatency, cacheWriteLatency;
    double cacheHitDynamicEnergy, cacheMissDynamicEnergy, cacheWriteDynamicEnergy;
    double cacheLeakage;
    double cacheArea;

    if (cacheAccessMode == normal_access_mode) {
        cacheMissLatency = tagResult.bank->readLatency;
        cacheHitLatency = MAX(tagResult.bank->readLatency, bank->mat.readLatency);
        cacheHitLatency += bank->mat.subarray.columnDecoderLatency;
        cacheHitLatency += bank->readLatency - bank->mat.readLatency;
        cacheWriteLatency = MAX(tagResult.bank->writeLatency, bank->writeLatency);

        cacheMissDynamicEnergy = tagResult.bank->readDynamicEnergy + bank->readDynamicEnergy;
        cacheHitDynamicEnergy = tagResult.bank->readDynamicEnergy + bank->readDynamicEnergy;
        cacheWriteDynamicEnergy = tagResult.bank->writeDynamicEnergy + bank->writeDynamicEnergy;
    } else if (cacheAccessMode == fast_access_mode) {
        cacheMissLatency = tagResult.bank->readLatency;
        cacheHitLatency = MAX(tagResult.bank->readLatency, bank->readLatency);
        cacheWriteLatency = MAX(tagResult.bank->writeLatency, bank->writeLatency);

        cacheMissDynamicEnergy = tagResult.bank->readDynamicEnergy + bank->readDynamicEnergy;
        cacheHitDynamicEnergy = tagResult.bank->readDynamicEnergy + bank->readDynamicEnergy;
        cacheWriteDynamicEnergy = tagResult.bank->writeDynamicEnergy + bank->writeDynamicEnergy;
    } else {  // sequential_access_mode
        cacheMissLatency = tagResult.bank->readLatency;
        cacheHitLatency = tagResult.bank->readLatency + bank->readLatency;
        cacheWriteLatency = MAX(tagResult.bank->writeLatency, bank->writeLatency);

        cacheMissDynamicEnergy = tagResult.bank->readDynamicEnergy + bank->readDynamicEnergy;
        cacheHitDynamicEnergy = tagResult.bank->readDynamicEnergy + bank->readDynamicEnergy;
        cacheWriteDynamicEnergy = tagResult.bank->writeDynamicEnergy + bank->writeDynamicEnergy;
    }

    cacheLeakage = tagResult.bank->leakage + bank->leakage;
    cacheArea = tagResult.bank->area + bank->area;

    // Generate YAML
    switch (cacheAccessMode) {
        case normal_access_mode: result["CacheDesign"]["AccessMode"] = "Normal"; break;
        case fast_access_mode:   result["CacheDesign"]["AccessMode"] = "Fast"; break;
        default:                 result["CacheDesign"]["AccessMode"] = "Sequential";
    }

	switch (inputParameter->designTarget) {
		case cache: result["CacheDesign"]["DesignTarget"] = "Cache"; break;
		case RAM_chip: result["CacheDesign"]["DesignTarget"] = "RAMChip"; break;
		case CAM_chip: result["CacheDesign"]["DesignTarget"] = "CAMChip"; break;
		default: result["CacheDesign"]["DesignTarget"] = "Unknown"; break;
	}

	//Capacity
	if (inputParameter->capacity < 1024) {
		result["Capacity"]["Value"] = inputParameter->capacity;
		result["Capacity"]["Unit"] = "B";
	} else if (inputParameter->capacity < 1024 * 1024) {
		result["Capacity"]["Value"] = inputParameter->capacity / 1024;
		result["Capacity"]["Unit"] = "KB";
	} else if (inputParameter->capacity < 1024 * 1024 * 1024) {
		result["Capacity"]["Value"] = inputParameter->capacity / 1024 / 1024;
		result["Capacity"]["Unit"] = "MB";
	} else {
		result["Capacity"]["Value"] = inputParameter->capacity / 1024 / 1024 / 1024;
		result["Capacity"]["Unit"] = "GB";
	}

    switch (optimizationTarget) {
        case read_latency_optimized: result["CacheDesign"]["OptimizationTarget"] = "ReadLatency"; break;
        case write_latency_optimized: result["CacheDesign"]["OptimizationTarget"] = "WriteLatency"; break;
        case read_energy_optimized: result["CacheDesign"]["OptimizationTarget"] = "ReadDynamicEnergy"; break;
        case write_energy_optimized: result["CacheDesign"]["OptimizationTarget"] = "WriteDynamicEnergy"; break;
		case read_edp_optimized: result["CacheDesign"]["OptimizationTarget"] = "ReadEDP"; break;
		case write_edp_optimized: result["CacheDesign"]["OptimizationTarget"] = "WriteEDP"; break;
		case leakage_optimized: result["CacheDesign"]["OptimizationTarget"] = "LeakagePower"; break;
		case area_optimized: result["CacheDesign"]["OptimizationTarget"] = "Area"; break;
        default:                 result["CacheDesign"]["OptimizationTarget"] = "Unknown";
    }

    result["CacheDesign"]["Area"]["Total_mm2"] = cacheArea * 1e6;
    result["CacheDesign"]["Area"]["DataArray_mm2"] = bank->area * 1e6;
    result["CacheDesign"]["Area"]["TagArray_mm2"] = tagResult.bank->area * 1e6;

    result["CacheDesign"]["Timing"]["CacheHitLatency_ns"] = cacheHitLatency * 1e9;
    result["CacheDesign"]["Timing"]["CacheMissLatency_ns"] = cacheMissLatency * 1e9;
    result["CacheDesign"]["Timing"]["CacheWriteLatency_ns"] = cacheWriteLatency * 1e9;

    if (cell->memCellType == eDRAM) {
        result["CacheDesign"]["Timing"]["CacheRefreshLatency_us"] =
            MAX(tagResult.bank->refreshLatency, bank->refreshLatency) * 1e6;
        result["CacheDesign"]["Timing"]["CacheAvailability_percent"] =
            ((cell->retentionTime - MAX(tagResult.bank->refreshLatency, bank->refreshLatency)) /
             cell->retentionTime) * 100.0;
    }

    result["CacheDesign"]["Power"]["CacheHitDynamicEnergy_nJ"] = cacheHitDynamicEnergy * 1e9;
    result["CacheDesign"]["Power"]["CacheMissDynamicEnergy_nJ"] = cacheMissDynamicEnergy * 1e9;
    result["CacheDesign"]["Power"]["CacheWriteDynamicEnergy_nJ"] = cacheWriteDynamicEnergy * 1e9;

    if (cell->memCellType == eDRAM) {
        result["CacheDesign"]["Power"]["CacheRefreshDynamicEnergy_nJ"] =
            (tagResult.bank->refreshDynamicEnergy + bank->refreshDynamicEnergy) * 1e9;
    }

    result["CacheDesign"]["Power"]["CacheTotalLeakagePower_mW"] = cacheLeakage * 1e3;
    result["CacheDesign"]["Power"]["CacheDataArrayLeakagePower_mW"] = bank->leakage * 1e3;
    result["CacheDesign"]["Power"]["CacheTagArrayLeakagePower_mW"] = tagResult.bank->leakage * 1e3;

    if (cell->memCellType == eDRAM || cell->memCellType == eDRAM3T || 
        cell->memCellType == eDRAM3T333) {
        result["CacheDesign"]["Power"]["CacheRefreshPower_W"] =
            (bank->refreshDynamicEnergy / cell->retentionTime);
        result["CacheDesign"]["Power"]["CacheRetentionTime_ns"] = cell->retentionTime * 1e9;
    }

    // Add reset and set pulse durations (using global cell pointer safely)
    if (cell) {
        result["CacheDesign"]["Timing"]["Reset"]["PulseDuration_ns"] =
            MAX(cell->resetPulse, cell->resetPulse) * 1e9;
        result["CacheDesign"]["Timing"]["Set"]["PulseDuration_ns"] =
            MAX(cell->setPulse, cell->setPulse) * 1e9;
    } else {
        result["CacheDesign"]["Timing"]["Reset"]["PulseDuration_ns"] = 0;
        result["CacheDesign"]["Timing"]["Set"]["PulseDuration_ns"] = 0;
    }

    // Add data and tag details
    result["DataArray"] = toYamlNode();
    result["TagArray"] = tagResult.toYamlNode();

    return result;
}



void Result::printToYamlFile(ofstream &outputFile) {
    YAML::Node node = toYamlNode();
    outputFile << node << endl;
}

void Result::printAsCacheToYamlFile(Result &tagResult, CacheAccessMode cacheAccessMode, ofstream &outputFile) {
    YAML::Node node = toYamlNodeAsCache(tagResult, cacheAccessMode);
    outputFile << node << endl;
}

