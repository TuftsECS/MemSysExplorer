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


#include "Result.hpp"
#include "global.hpp"
#include "formula.hpp"
#include "macros.hpp"
#include "enuminfo.hpp"

#include "yaml-cpp/yaml.h"

#include <iostream>
#include <fstream>

Result::Result() {
	// TODO Auto-generated constructor stub
	if (gInputParameter.routingMode == h_tree)
        bank = std::make_unique<BankWithHtree>();
	else
        bank = std::make_unique<BankWithoutHtree>();
	localWire = std::make_unique<Wire>();
	globalWire = std::make_unique<Wire>();

	/* initialize the worst case */
    reset();
}

Result::Result(const Result& other) : Result() {
	*bank = *(other.bank);
	*localWire = *(other.localWire);
	*globalWire = *(other.globalWire);
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

void Result::compareAndUpdate(Result& newResult) {
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
	std::cout << "\n=============\n"
              << "CONFIGURATION\n"
              << "=============\n";

	std::cout << "Bank Organization: " << bank->numRowMat << " x " << bank->numColumnMat << "\n";
	std::cout << " - Row Activation   : " << bank->numActiveMatPerColumn << " / " << bank->numRowMat << "\n";
	std::cout << " - Column Activation: " << bank->numActiveMatPerRow << " / " << bank->numColumnMat << "\n";
	std::cout << "Mat Organization: " << bank->numRowSubarray << " x " << bank->numColumnSubarray << "\n";
	std::cout << " - Row Activation   : " << bank->numActiveSubarrayPerColumn << " / " << bank->numRowSubarray << "\n";
	std::cout << " - Column Activation: " << bank->numActiveSubarrayPerRow << " / " << bank->numColumnSubarray << "\n";
	std::cout << " - Subarray Size    : " << bank->mat.subarray.numRow << " Rows x " << bank->mat.subarray.numColumn << " Columns\n";
	std::cout << "Mux Level:\n";
	std::cout << " - Senseamp Mux      : " << bank->muxSenseAmp << "\n";
	std::cout << " - Output Level-1 Mux: " << bank->muxOutputLev1 << "\n";
	std::cout << " - Output Level-2 Mux: " << bank->muxOutputLev2 << "\n";
	if (gInputParameter.designTarget == cache)
		std::cout << " - One set is partitioned into " << bank->numRowPerSet << " rows\n";
	std::cout << "Local Wire:\n";
	std::cout << " - Wire Type : " << localWire->wireType << "\n";
	std::cout << " - Repeater Type: " << localWire->wireRepeaterType << "\n";
	std::cout << " - Low Swing : ";
	if (localWire->isLowSwing)
		std::cout << "Yes\n";
	else
		std::cout << "No\n";
	std::cout << "Global Wire:\n";
	std::cout << " - Wire Type : " << globalWire->wireType << "\n";
	std::cout << " - Repeater Type: " << globalWire->wireRepeaterType << "\n";
	std::cout << " - Low Swing : ";
	if (globalWire->isLowSwing)
		std::cout << "Yes\n";
	else
		std::cout << "No\n";
	std::cout << "Buffer Design Style: " << bank->areaOptimizationLevel << "\n";

	std::cout << "=============\n"
              << "   RESULT\n"
              << "=============\n";

	std::cout << "Area:\n";

	std::cout << " - Total Area = " << TO_METER(bank->height) << " x " << TO_METER(bank->width)
			<< " = " << TO_SQM(bank->area) << "\n";
	std::cout << " |--- Mat Area      = " << TO_METER(bank->mat.height) << " x " << TO_METER(bank->mat.width)
			<< " = " << TO_SQM(bank->mat.area) << "   (" << gCell.area * gTech.featureSize * gTech.featureSize
			* bank->capacity / bank->numRowMat / bank->numColumnMat / bank->mat.area * 100 << "%)\n";
	std::cout << " |--- Subarray Area = " << TO_METER(bank->mat.subarray.height) << " x "
			<< TO_METER(bank->mat.subarray.width) << " = " << TO_SQM(bank->mat.subarray.area) << "   ("
			<< gCell.area * gTech.featureSize * gTech.featureSize * bank->capacity / bank->numRowMat
			/ bank->numColumnMat / bank->numRowSubarray / bank->numColumnSubarray
			/ bank->mat.subarray.area * 100 << "%)\n";
	//Qing: subarray buffer area
	std::cout << " |--- Subarray Buffer Area = " << TO_METER(bank->mat.subarray.subarrayBuffer.height) << " x "
			<< TO_METER(bank->mat.subarray.subarrayBuffer.width) << " = " << TO_SQM(bank->mat.subarray.subarrayBuffer.area) << "\n";
	//Qing.
	std::cout << " - Area Efficiency = " << gCell.area * gTech.featureSize * gTech.featureSize
			* bank->capacity / bank->area * 100 << "%\n";

	std::cout << "Timing:\n";

	std::cout << " -  Read Latency = " << TO_SECOND(bank->readLatency) << "\n";
	if (gInputParameter.routingMode == h_tree)
		std::cout << " |--- H-Tree Latency = " << TO_SECOND(bank->readLatency - bank->mat.readLatency) << "\n";
	else
		std::cout << " |--- Non-H-Tree Latency = " << TO_SECOND(bank->readLatency - bank->mat.readLatency) << "\n";
	std::cout << " |--- Mat Latency    = " << TO_SECOND(bank->mat.readLatency) << "\n";
	std::cout << "    |--- Predecoder Latency = " << TO_SECOND(bank->mat.predecoderLatency) << "\n";
	std::cout << "    |--- Subarray Latency   = " << TO_SECOND(bank->mat.subarray.readLatency) << "\n";
	std::cout << "       |--- Row Decoder Latency = " << TO_SECOND(bank->mat.subarray.rowDecoder.readLatency) << "\n";
	if (gCell.memCellType != eDRAM3T333 && gCell.memCellType != eDRAM3T)
		std::cout << "       |--- Bitline Latency     = " << TO_SECOND(bank->mat.subarray.bitlineDelay) << "\n";
	else
		std::cout << "       |--- Bitline Latency     = " << TO_SECOND(bank->mat.subarray.bitlineDelayR) << "\n";
	if (gInputParameter.internalSensing)
		std::cout << "       |--- Senseamp Latency    = " << TO_SECOND(bank->mat.subarray.senseAmp.readLatency) << "\n";
	std::cout << "       |--- Mux Latency         = " << TO_SECOND(bank->mat.subarray.bitlineMux.readLatency
													+ bank->mat.subarray.senseAmpMuxLev1.readLatency
													+ bank->mat.subarray.senseAmpMuxLev2.readLatency) << "\n";
	std::cout << "       |--- Precharge Latency   = " << TO_SECOND(bank->mat.subarray.precharger.readLatency) << "\n";
	if (bank->mat.memoryType == tag && bank->mat.internalSenseAmp)
		std::cout << "    |--- Comparator Latency  = " << TO_SECOND(bank->mat.comparator.readLatency) << "\n";

	if (gCell.memCellType == PCRAM || gCell.memCellType == FBRAM || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM ||
			(gCell.memCellType == memristor && (gCell.accessType == CMOS_access || gCell.accessType == BJT_access))) {
		std::cout << " - RESET Latency = " << TO_SECOND(bank->resetLatency) << "\n";
		if (gInputParameter.routingMode == h_tree)
			std::cout << " |--- H-Tree Latency = " << TO_SECOND(bank->resetLatency - bank->mat.resetLatency) << "\n";
		else
			std::cout << " |--- Non-H-Tree Latency = " << TO_SECOND(bank->resetLatency - bank->mat.resetLatency) << "\n";
		std::cout << " |--- Mat Latency    = " << TO_SECOND(bank->mat.resetLatency) << "\n";
		std::cout << "    |--- Predecoder Latency = " << TO_SECOND(bank->mat.predecoderLatency) << "\n";
		std::cout << "    |--- Subarray Latency   = " << TO_SECOND(bank->mat.subarray.resetLatency) << "\n";
		std::cout << "       |--- RESET Pulse Duration = " << TO_SECOND(gCell.resetPulse) << "\n";
		std::cout << "       |--- Row Decoder Latency  = " << TO_SECOND(bank->mat.subarray.rowDecoder.writeLatency) << "\n";
		std::cout << "       |--- Charge Latency   = " << TO_SECOND(bank->mat.subarray.chargeLatency) << "\n";
		std::cout << " - SET Latency   = " << TO_SECOND(bank->setLatency) << "\n";
		if (gInputParameter.routingMode == h_tree)
			std::cout << " |--- H-Tree Latency = " << TO_SECOND(bank->setLatency - bank->mat.setLatency) << "\n";
		else
			std::cout << " |--- Non-H-Tree Latency = " << TO_SECOND(bank->setLatency - bank->mat.setLatency) << "\n";
		std::cout << " |--- Mat Latency    = " << TO_SECOND(bank->mat.setLatency) << "\n";
		std::cout << "    |--- Predecoder Latency = " << TO_SECOND(bank->mat.predecoderLatency) << "\n";
		std::cout << "    |--- Subarray Latency   = " << TO_SECOND(bank->mat.subarray.setLatency) << "\n";
		std::cout << "       |--- SET Pulse Duration   = " << TO_SECOND(gCell.setPulse) << "\n";
		std::cout << "       |--- Row Decoder Latency  = " << TO_SECOND(bank->mat.subarray.rowDecoder.writeLatency) << "\n";
		std::cout << "       |--- Charger Latency      = " << TO_SECOND(bank->mat.subarray.chargeLatency) << "\n";
	} else if (gCell.memCellType == SLCNAND) {
		std::cout << " - Erase Latency = " << TO_SECOND(bank->resetLatency) << "\n";
		if (gInputParameter.routingMode == h_tree)
			std::cout << " |--- H-Tree Latency = " << TO_SECOND(bank->resetLatency - bank->mat.resetLatency) << "\n";
		else
			std::cout << " |--- Non-H-Tree Latency = " << TO_SECOND(bank->resetLatency - bank->mat.resetLatency) << "\n";
		std::cout << " |--- Mat Latency    = " << TO_SECOND(bank->mat.resetLatency) << "\n";
		std::cout << " - Programming Latency   = " << TO_SECOND(bank->setLatency) << "\n";
		if (gInputParameter.routingMode == h_tree)
			std::cout << " |--- H-Tree Latency = " << TO_SECOND(bank->setLatency - bank->mat.setLatency) << "\n";
		else
			std::cout << " |--- Non-H-Tree Latency = " << TO_SECOND(bank->setLatency - bank->mat.setLatency) << "\n";
		std::cout << " |--- Mat Latency    = " << TO_SECOND(bank->mat.setLatency) << "\n";
	} else {
		std::cout << " - Write Latency = " << TO_SECOND(bank->writeLatency) << "\n";
		if (gInputParameter.routingMode == h_tree)
			std::cout << " |--- H-Tree Latency = " << TO_SECOND(bank->writeLatency - bank->mat.writeLatency) << "\n";
		else
			std::cout << " |--- Non-H-Tree Latency = " << TO_SECOND(bank->writeLatency - bank->mat.writeLatency) << "\n";
		std::cout << " |--- Mat Latency    = " << TO_SECOND(bank->mat.writeLatency) << "\n";
		std::cout << "    |--- Predecoder Latency = " << TO_SECOND(bank->mat.predecoderLatency) << "\n";
		std::cout << "    |--- Subarray Latency   = " << TO_SECOND(bank->mat.subarray.writeLatency) << "\n";
		if (gCell.memCellType == MRAM)
			std::cout << "       |--- Write Pulse Duration = " << TO_SECOND(gCell.resetPulse) << "\n";	// MRAM reset/set is equal
		std::cout << "       |--- Row Decoder Latency = " << TO_SECOND(bank->mat.subarray.rowDecoder.writeLatency) << "\n";
		std::cout << "       |--- Charge Latency      = " << TO_SECOND(bank->mat.subarray.chargeLatency) << "\n";
		if (gCell.memCellType != eDRAM3T333 && gCell.memCellType != eDRAM3T)
			std::cout << "       |--- Bitline Latency     = " << TO_SECOND(bank->mat.subarray.bitlineDelay) << "\n";
		else
			std::cout << "       |--- Bitline Latency     = " << TO_SECOND(bank->mat.subarray.bitlineDelayW) << "\n";
		}

	//Qing: subarray buffer latency
	std::cout << "- Subarray Buf R/W Latency  = " << TO_SECOND(bank->mat.subarray.subarrayBuffer.readLatency) << "\n";
	std::cout << "- Subarray Buf XOR Latency  = " << TO_SECOND(bank->mat.subarray.subarrayBuffer.xorLatency) << "\n";
	//Qing.
    //bank->mat.subarray.PrintProperty();	
	double readBandwidth = (double)bank->blockSize /
			(bank->mat.subarray.readLatency - bank->mat.subarray.rowDecoder.readLatency
			+ bank->mat.subarray.precharger.readLatency) / 8;
	if (gCell.memCellType == MLCCTT || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
            readBandwidth *= log2(gCell.nLvl);
        }
        std::cout << " - Read Bandwidth  = " << TO_BPS(readBandwidth) << "\n";

	double writeBandwidth = (double)bank->blockSize /
			(bank->mat.subarray.writeLatency) / 8;
	std::cout << " - Write Bandwidth = " << TO_BPS(writeBandwidth) << "\n";

	std::cout << "Power:\n";

	std::cout << " -  Read Dynamic Energy = " << TO_JOULE(bank->readDynamicEnergy) << "\n";
	if (gInputParameter.routingMode == h_tree)
		std::cout << " |--- H-Tree Read Dynamic Energy = " << TO_JOULE(bank->readDynamicEnergy - bank->mat.readDynamicEnergy
													* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
													<< "\n";
	else
		std::cout << " |--- Non-H-Tree Read Dynamic Energy = " << TO_JOULE(bank->readDynamicEnergy - bank->mat.readDynamicEnergy
													* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
													<< "\n";
	std::cout << " |--- Mat Dynamic Energy    = " << TO_JOULE(bank->mat.readDynamicEnergy) << " per mat\n";
	std::cout << "    |--- Predecoder Dynamic Energy = " << TO_JOULE(bank->mat.readDynamicEnergy - bank->mat.subarray.readDynamicEnergy
														* bank->numActiveSubarrayPerRow * bank->numActiveSubarrayPerColumn)
														<< "\n";
	std::cout << "    |--- Subarray Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.readDynamicEnergy) << " per active subarray\n";
	std::cout << "       |--- Row Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.rowDecoder.readDynamicEnergy) << "\n";
	std::cout << "       |--- Mux Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.bitlineMuxDecoder.readDynamicEnergy
													+ bank->mat.subarray.senseAmpMuxLev1Decoder.readDynamicEnergy
													+ bank->mat.subarray.senseAmpMuxLev2Decoder.readDynamicEnergy) << "\n";
	if (gCell.memCellType == PCRAM || gCell.memCellType == FBRAM || gCell.memCellType == MRAM || gCell.memCellType == memristor || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
		std::cout << "       |--- Bitline & Cell Read Energy = " << TO_JOULE(bank->mat.subarray.cellReadEnergy) << "\n";
	}
	if (gInputParameter.internalSensing)
		std::cout << "       |--- Senseamp Dynamic Energy    = " << TO_JOULE(bank->mat.subarray.senseAmp.readDynamicEnergy) << "\n";
	std::cout << "       |--- Mux Dynamic Energy         = " << TO_JOULE(bank->mat.subarray.bitlineMux.readDynamicEnergy
													+ bank->mat.subarray.senseAmpMuxLev1.readDynamicEnergy
													+ bank->mat.subarray.senseAmpMuxLev2.readDynamicEnergy) << "\n";
	std::cout << "       |--- Precharge Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.precharger.readDynamicEnergy) << "\n";

	if (gCell.memCellType == PCRAM || gCell.memCellType == FBRAM || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM ||
			(gCell.memCellType == memristor && (gCell.accessType == CMOS_access || gCell.accessType == BJT_access))) {
		std::cout << " - RESET Dynamic Energy = " << TO_JOULE(bank->resetDynamicEnergy) << "\n";
		if (gInputParameter.routingMode == h_tree)
			std::cout << " |--- H-Tree Write Dynamic Energy = " << TO_JOULE(bank->resetDynamicEnergy - bank->mat.resetDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< "\n";
		else
			std::cout << " |--- H-Tree Write Dynamic Energy = " << TO_JOULE(bank->resetDynamicEnergy - bank->mat.resetDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< "\n";
		std::cout << " |--- Mat Dynamic Energy    = " << TO_JOULE(bank->mat.resetDynamicEnergy) << " per mat\n";
		std::cout << "    |--- Predecoder Dynamic Energy = " << TO_JOULE(bank->mat.writeDynamicEnergy - bank->mat.subarray.writeDynamicEnergy
															* bank->numActiveSubarrayPerRow * bank->numActiveSubarrayPerColumn)
															<< "\n";
		std::cout << "    |--- Subarray Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.writeDynamicEnergy) << " per active subarray\n";
		std::cout << "       |--- Row Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.rowDecoder.writeDynamicEnergy) << "\n";
		std::cout << "       |--- Mux Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.bitlineMuxDecoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1Decoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2Decoder.writeDynamicEnergy) << "\n";
		std::cout << "       |--- Mux Dynamic Energy         = " << TO_JOULE(bank->mat.subarray.bitlineMux.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2.writeDynamicEnergy) << "\n";
		std::cout << "       |--- Cell RESET Dynamic Energy  = " << TO_JOULE(bank->mat.subarray.cellResetEnergy) << "\n";
		std::cout << " - SET Dynamic Energy = " << TO_JOULE(bank->setDynamicEnergy) << "\n";
		if (gInputParameter.routingMode == h_tree)
			std::cout << " |--- H-Tree Write Dynamic Energy = " << TO_JOULE(bank->setDynamicEnergy - bank->mat.setDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< "\n";
		else
			std::cout << " |--- Non-H-Tree Write Dynamic Energy = " << TO_JOULE(bank->setDynamicEnergy - bank->mat.setDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< "\n";
		std::cout << " |--- Mat Dynamic Energy    = " << TO_JOULE(bank->mat.setDynamicEnergy) << " per mat\n";
		std::cout << "    |--- Predecoder Dynamic Energy = " << TO_JOULE(bank->mat.writeDynamicEnergy - bank->mat.subarray.writeDynamicEnergy
															* bank->numActiveSubarrayPerRow * bank->numActiveSubarrayPerColumn)
															<< "\n";
		std::cout << "    |--- Subarray Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.writeDynamicEnergy) << " per active subarray\n";
		std::cout << "       |--- Row Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.rowDecoder.writeDynamicEnergy) << "\n";
		std::cout << "       |--- Mux Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.bitlineMuxDecoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1Decoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2Decoder.writeDynamicEnergy) << "\n";
		std::cout << "       |--- Mux Dynamic Energy         = " << TO_JOULE(bank->mat.subarray.bitlineMux.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2.writeDynamicEnergy) << "\n";
		std::cout << "       |--- Cell SET Dynamic Energy    = " << TO_JOULE(bank->mat.subarray.cellSetEnergy) << "\n";
	} else if (gCell.memCellType == SLCNAND) {
		std::cout << " - Erase Dynamic Energy = " << TO_JOULE(bank->resetDynamicEnergy) << " per block\n";
		if (gInputParameter.routingMode == h_tree)
			std::cout << " |--- H-Tree Write Dynamic Energy = " << TO_JOULE(bank->resetDynamicEnergy - bank->mat.resetDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< "\n";
		else
			std::cout << " |--- Non-H-Tree Write Dynamic Energy = " << TO_JOULE(bank->resetDynamicEnergy - bank->mat.resetDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< "\n";
		std::cout << " |--- Mat Dynamic Energy    = " << TO_JOULE(bank->mat.resetDynamicEnergy) << " per mat\n";
		std::cout << "    |--- Predecoder Dynamic Energy = " << TO_JOULE(bank->mat.writeDynamicEnergy - bank->mat.subarray.writeDynamicEnergy
															* bank->numActiveSubarrayPerRow * bank->numActiveSubarrayPerColumn)
															<< "\n";
		std::cout << "    |--- Subarray Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.writeDynamicEnergy) << " per active subarray\n";
		std::cout << "       |--- Row Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.rowDecoder.writeDynamicEnergy) << "\n";
		std::cout << "       |--- Mux Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.bitlineMuxDecoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1Decoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2Decoder.writeDynamicEnergy) << "\n";
		std::cout << "       |--- Mux Dynamic Energy         = " << TO_JOULE(bank->mat.subarray.bitlineMux.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2.writeDynamicEnergy) << "\n";
		std::cout << " - Programming Dynamic Energy = " << TO_JOULE(bank->setDynamicEnergy) << " per page\n";
		if (gInputParameter.routingMode == h_tree)
			std::cout << " |--- H-Tree Write Dynamic Energy = " << TO_JOULE(bank->setDynamicEnergy - bank->mat.setDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< "\n";
		else
			std::cout << " |--- Non-H-Tree Write Dynamic Energy = " << TO_JOULE(bank->setDynamicEnergy - bank->mat.setDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< "\n";
		std::cout << " |--- Mat Dynamic Energy    = " << TO_JOULE(bank->mat.setDynamicEnergy) << " per mat\n";
		std::cout << "    |--- Predecoder Dynamic Energy = " << TO_JOULE(bank->mat.writeDynamicEnergy - bank->mat.subarray.writeDynamicEnergy
															* bank->numActiveSubarrayPerRow * bank->numActiveSubarrayPerColumn)
															<< "\n";
		std::cout << "    |--- Subarray Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.writeDynamicEnergy) << " per active subarray\n";
		std::cout << "       |--- Row Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.rowDecoder.writeDynamicEnergy) << "\n";
		std::cout << "       |--- Mux Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.bitlineMuxDecoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1Decoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2Decoder.writeDynamicEnergy) << "\n";
		std::cout << "       |--- Mux Dynamic Energy         = " << TO_JOULE(bank->mat.subarray.bitlineMux.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2.writeDynamicEnergy) << "\n";
	} else {
		std::cout << " - Write Dynamic Energy = " << TO_JOULE(bank->writeDynamicEnergy) << "\n";
		if (gInputParameter.routingMode == h_tree)
			std::cout << " |--- H-Tree Write Dynamic Energy = " << TO_JOULE(bank->writeDynamicEnergy - bank->mat.writeDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< "\n";
		else
			std::cout << " |--- Non-H-Tree Write Dynamic Energy = " << TO_JOULE(bank->writeDynamicEnergy - bank->mat.writeDynamicEnergy
														* bank->numActiveMatPerColumn * bank->numActiveMatPerRow)
														<< "\n";
		std::cout << " |--- Mat Dynamic Energy    = " << TO_JOULE(bank->mat.writeDynamicEnergy) << " per mat\n";
		std::cout << "    |--- Predecoder Dynamic Energy = " << TO_JOULE(bank->mat.writeDynamicEnergy - bank->mat.subarray.writeDynamicEnergy
															* bank->numActiveSubarrayPerRow * bank->numActiveSubarrayPerColumn)
															<< "\n";
		std::cout << "    |--- Subarray Dynamic Energy   = " << TO_JOULE(bank->mat.subarray.writeDynamicEnergy) << " per active subarray\n";
		std::cout << "       |--- Row Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.rowDecoder.writeDynamicEnergy) << "\n";
		std::cout << "       |--- Mux Decoder Dynamic Energy = " << TO_JOULE(bank->mat.subarray.bitlineMuxDecoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1Decoder.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2Decoder.writeDynamicEnergy) << "\n";
		std::cout << "       |--- Mux Dynamic Energy         = " << TO_JOULE(bank->mat.subarray.bitlineMux.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev1.writeDynamicEnergy
														+ bank->mat.subarray.senseAmpMuxLev2.writeDynamicEnergy) << "\n";
		if (gCell.memCellType == MRAM) {
			std::cout << "       |--- Bitline & Cell Write Energy= " << TO_JOULE(bank->mat.subarray.cellResetEnergy) << "\n";
		}
	}

	//Qing: subarray buffer energy
	std::cout << "- Subarray Buf R/W Energy = " << TO_JOULE(bank->mat.subarray.subarrayBuffer.readDynamicEnergy) << "\n";
	std::cout << "- Subarray Buf XOR Energy = " << TO_JOULE(bank->mat.subarray.subarrayBuffer.xorDynamicEnergy) << "\n";
	//Qing.

	std::cout << " - Leakage Power = " << TO_WATT(bank->leakage) << "\n";
	if (gInputParameter.routingMode == h_tree)
		std::cout << " |--- H-Tree Leakage Power = " << TO_WATT(bank->leakage - bank->mat.leakage
													* bank->numColumnMat * bank->numRowMat)
													<< "\n";
	else
		std::cout << " |--- Non-H-Tree Leakage Power = " << TO_WATT(bank->leakage - bank->mat.leakage
													* bank->numColumnMat * bank->numRowMat)
													<< "\n";
	std::cout << " |--- Mat Leakage Power    = " << TO_WATT(bank->mat.leakage) << " per mat\n";
	if (gCell.memCellType == eDRAM || gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
		// David Note: refresh period could be shorter than retention time 
        std::cout << " - Refresh Power = " << TO_WATT(bank->refreshDynamicEnergy / (gCell.retentionTime)) << "\n";
    }
}


void Result::printAsCache(Result& tagResult, CacheAccessMode cacheAccessMode) {
	if (bank->memoryType != dataT || tagResult.bank->memoryType != tag) {
		std::cout << "This is not a valid cache configuration.\n";
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
		std::cout << "\n=======================\n"
                  << "CACHE DESIGN -- SUMMARY\n"
                  << "=======================\n";

		std::cout << "Access Mode: " << cacheAccessMode << "\n";
		std::cout << "Area:" << std::endl;
		std::cout << " - Total Area = " << cacheArea * 1e6 << "mm^2" << std::endl;
		std::cout << " |--- Data Array Area = " << bank->height * 1e6 << "um x " << bank->width * 1e6 << "um = " << bank->area * 1e6 << "mm^2" << std::endl;
		std::cout << " |--- Tag Array Area  = " << tagResult.bank->height * 1e6 << "um x " << tagResult.bank->width * 1e6 << "um = " << tagResult.bank->area * 1e6 << "mm^2" << std::endl;
		std::cout << "Timing:" << std::endl;
		std::cout << " - Cache Hit Latency   = " << cacheHitLatency * 1e9 << "ns" << std::endl;
		std::cout << " - Cache Miss Latency  = " << cacheMissLatency * 1e9 << "ns" << std::endl;
		std::cout << " - Cache Write Latency = " << cacheWriteLatency * 1e9 << "ns" << std::endl;
        if (gCell.memCellType == eDRAM) {
            std::cout << " - Cache Refresh Latency = " << MAX(tagResult.bank->refreshLatency, bank->refreshLatency) * 1e6 << "us per bank" << std::endl;
            std::cout << " - Cache Availability = " << ((gCell.retentionTime - MAX(tagResult.bank->refreshLatency, bank->refreshLatency)) / gCell.retentionTime) * 100.0 << "%" << std::endl;
        }
		std::cout << "Power:" << std::endl;
		std::cout << " - Cache Hit Dynamic Energy   = " << cacheHitDynamicEnergy * 1e9 << "nJ per access" << std::endl;
		std::cout << " - Cache Miss Dynamic Energy  = " << cacheMissDynamicEnergy * 1e9 << "nJ per access" << std::endl;
		std::cout << " - Cache Write Dynamic Energy = " << cacheWriteDynamicEnergy * 1e9 << "nJ per access" << std::endl;
        if (gCell.memCellType == eDRAM) {
            std::cout << " - Cache Refresh Dynamic Energy = " << (tagResult.bank->refreshDynamicEnergy + bank->refreshDynamicEnergy) * 1e9 << "nJ per bank" << std::endl;
        }
		std::cout << " - Cache Total Leakage Power  = " << cacheLeakage * 1e3 << "mW" << std::endl;
		std::cout << " |--- Cache Data Array Leakage Power = " << bank->leakage * 1e3 << "mW" << std::endl;
		std::cout << " |--- Cache Tag Array Leakage Power  = " << tagResult.bank->leakage * 1e3 << "mW" << std::endl;
		if (gCell.memCellType == eDRAM || gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
            std::cout << " - Cache Refresh Power = " << TO_WATT(bank->refreshDynamicEnergy / (gCell.retentionTime)) << " per bank" << std::endl;
			std::cout << " - Cache Retention Time = " << (gCell.retentionTime)*1e9 << "ns" << std::endl;
        }
		std::cout << std::endl << "CACHE DATA ARRAY";
		print();
		std::cout << std::endl << "CACHE TAG ARRAY";
		tagResult.print();
	}
}

YAML::Node Result::toYamlNode() {
	YAML::Node result;

	if(gInputParameter.designTarget != cache){
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
		switch (gCell.memCellType) {
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
		result["MemoryCell"]["CellArea_F2"]  = gCell.area;
		result["MemoryCell"]["CellArea_um2"] = gCell.area / 1000000.0 * gTech.featureSizeInNano * gTech.featureSizeInNano;
		result["MemoryCell"]["AspectRatio"]  = gCell.aspectRatio;

		// Resistive / Non-volatile memory
		if (gCell.memCellType == PCRAM || gCell.memCellType == MRAM || gCell.memCellType == memristor ||
			gCell.memCellType == FBRAM || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET ||
			gCell.memCellType == MLCRRAM) {

			if (gCell.resistanceOn < 1e3)
				result["MemoryCell"]["R_on_Ohm"] = gCell.resistanceOn;
			else if (gCell.resistanceOn < 1e6)
				result["MemoryCell"]["R_on_KOhm"] = gCell.resistanceOn / 1e3;
			else
				result["MemoryCell"]["R_on_MOhm"] = gCell.resistanceOn / 1e6;

			if (gCell.resistanceOff < 1e3)
				result["MemoryCell"]["R_off_Ohm"] = gCell.resistanceOff;
			else if (gCell.resistanceOff < 1e6)
				result["MemoryCell"]["R_off_KOhm"] = gCell.resistanceOff / 1e3;
			else
				result["MemoryCell"]["R_off_MOhm"] = gCell.resistanceOff / 1e6;

			result["MemoryCell"]["ReadMode"]  = gCell.readMode ? "Voltage-Sensing" : "Current-Sensing";
			if (gCell.readCurrent > 0) result["MemoryCell"]["ReadCurrent_uA"] = gCell.readCurrent * 1e6;
			if (gCell.readVoltage > 0) result["MemoryCell"]["ReadVoltage_V"] = gCell.readVoltage;

			result["MemoryCell"]["ResetMode"] = gCell.resetMode ? "Voltage" : "Current";
			result["MemoryCell"]["ResetVoltage_V"] = gCell.resetVoltage;
			result["MemoryCell"]["ResetCurrent_uA"] = gCell.resetCurrent * 1e6;
			result["MemoryCell"]["ResetPulse_s"] = gCell.resetPulse / 1e9;

			result["MemoryCell"]["SetMode"] = gCell.setMode ? "Voltage" : "Current";
			result["MemoryCell"]["SetVoltage_V"] = gCell.setVoltage;
			result["MemoryCell"]["SetCurrent_uA"] = gCell.setCurrent * 1e6;
			result["MemoryCell"]["SetPulse_s"] = gCell.setPulse / 1e9;

			switch (gCell.accessType) {
				case CMOS_access: result["MemoryCell"]["AccessType"] = "CMOS"; break;
				case BJT_access: result["MemoryCell"]["AccessType"] = "BJT"; break;
				case diode_access: result["MemoryCell"]["AccessType"] = "Diode"; break;
				default: result["MemoryCell"]["AccessType"] = "None Access Device"; break;
			}
		}

		// SRAM
		if (gCell.memCellType == SRAM) {
			result["MemoryCell"]["WidthAccessCMOS_F"]   = gCell.widthAccessCMOS;
			result["MemoryCell"]["WidthSRAMCellNMOS_F"] = gCell.widthSRAMCellNMOS;
			result["MemoryCell"]["WidthSRAMCellPMOS_F"] = gCell.widthSRAMCellPMOS;
			result["MemoryCell"]["PeripheralRoadmap"]   = roadmapToString(gTech.deviceRoadmap);
			result["MemoryCell"]["PeripheralNode_nm"]   = gTech.featureSizeInNano;
			result["MemoryCell"]["VDD_V"]               = gTech.vdd;
			result["MemoryCell"]["WWL_SWING"]           = gTech.vdd;
			result["MemoryCell"]["Temperature_K"]       = gCell.temperature;
		}

		// DRAM / eDRAM
		if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
			result["MemoryCell"]["WidthAccessCMOS_F"] = gCell.widthAccessCMOS;
			result["MemoryCell"]["PeripheralRoadmap"] = roadmapToString(gTech.deviceRoadmap);
			result["MemoryCell"]["PeripheralNode_nm"] = gTech.featureSizeInNano;
			result["MemoryCell"]["VDD_V"] = gTech.vdd;
			result["MemoryCell"]["WWL_SWING"] = gTech.vpp;
			result["MemoryCell"]["Temperature_K"] = gCell.temperature;
		}

		// 3T DRAM
		if (gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
			result["MemoryCell"]["WidthWriteAccessCMOS_F"] = gCell.widthAccessCMOS;
			result["MemoryCell"]["WidthReadAccessCMOS_F"]  = gCell.widthAccessCMOSR;
			result["MemoryCell"]["PeripheralRoadmap"]      = roadmapToString(gTech.deviceRoadmap);
			result["MemoryCell"]["WriteAccessRoadmap"]     = roadmapToString(gTechW.deviceRoadmap);
			result["MemoryCell"]["ReadAccessRoadmap"]      = roadmapToString(gTechR.deviceRoadmap);
			result["MemoryCell"]["PeripheralNode_nm"]      = gTech.featureSizeInNano;
			result["MemoryCell"]["WriteAccessNode_nm"]     = gTechW.featureSizeInNano;
			result["MemoryCell"]["ReadAccessNode_nm"]      = gTechR.featureSizeInNano;
			result["MemoryCell"]["VDD_V"]                  = gTech.vdd;
			result["MemoryCell"]["WWL_SWING"]              = gTechW.vpp;
			result["MemoryCell"]["Temperature_K"]          = gCell.temperature;
		}

		// SLC NAND Flash
		if (gCell.memCellType == SLCNAND) {
			result["MemoryCell"]["PassVoltage_V"]     = gCell.flashPassVoltage;
			result["MemoryCell"]["ProgramVoltage_V"]  = gCell.flashProgramVoltage;
			result["MemoryCell"]["EraseVoltage_V"]    = gCell.flashEraseVoltage;
			result["MemoryCell"]["ProgramTime_s"]     = gCell.flashProgramTime / 1e9;
			result["MemoryCell"]["EraseTime_s"]       = gCell.flashEraseTime / 1e9;
			result["MemoryCell"]["GateCouplingRatio"] = gCell.gateCouplingRatio;
		}

		// Multi-level cells
		if (gCell.memCellType == MLCCTT || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
			result["MemoryCell"]["NumberOfInputFingers"]   = gCell.nFingers;
			result["MemoryCell"]["NumberOfLevelsPerCell"] = gCell.nLvl;
		}

	}

	if(gInputParameter.designTarget != cache){
		//Capacity
		if (gInputParameter.capacity < 1024) {
			result["Capacity"]["Value"] = gInputParameter.capacity;
			result["Capacity"]["Unit"] = "B";
		} else if (gInputParameter.capacity < 1024 * 1024) {
			result["Capacity"]["Value"] = gInputParameter.capacity / 1024;
			result["Capacity"]["Unit"] = "KB";
		} else if (gInputParameter.capacity < 1024 * 1024 * 1024) {
			result["Capacity"]["Value"] = gInputParameter.capacity / 1024 / 1024;
			result["Capacity"]["Unit"] = "MB";
		} else {
			result["Capacity"]["Value"] = gInputParameter.capacity / 1024 / 1024 / 1024;
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
    if (gInputParameter.designTarget == cache)
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
        (gCell.area * gTech.featureSize * gTech.featureSize * bank->capacity / 
         bank->numRowMat / bank->numColumnMat / bank->mat.area * 100);

    result["Results"]["Area"]["Subarray"]["Height_um"] = bank->mat.subarray.height * 1e6;
    result["Results"]["Area"]["Subarray"]["Width_um"] = bank->mat.subarray.width * 1e6;
    result["Results"]["Area"]["Subarray"]["Area_mm2"] = bank->mat.subarray.area * 1e6;
    result["Results"]["Area"]["Subarray"]["Efficiency_percent"] =
        (gCell.area * gTech.featureSize * gTech.featureSize * bank->capacity / 
         bank->numRowMat / bank->numColumnMat / bank->numRowSubarray / 
         bank->numColumnSubarray / bank->mat.subarray.area * 100);

    result["Results"]["Area"]["AreaEfficiency_percent"] =
        (gCell.area * gTech.featureSize * gTech.featureSize * bank->capacity / bank->area * 100);

    // Timing
    result["Results"]["Timing"]["Read"]["Latency_ns"] = bank->readLatency * 1e9;
    result["Results"]["Timing"]["Read"]["TreeLatency_ns"] = 
        (bank->readLatency - bank->mat.readLatency) * 1e9;
    result["Results"]["Timing"]["Read"]["MatLatency_ns"] = bank->mat.readLatency * 1e9;
    result["Results"]["Timing"]["Read"]["PredecoderLatency_ns"] = bank->mat.predecoderLatency * 1e9;
    result["Results"]["Timing"]["Read"]["SubarrayLatency_ns"] = bank->mat.subarray.readLatency * 1e9;
    result["Results"]["Timing"]["Read"]["RowDecoderLatency_ns"] = 
        bank->mat.subarray.rowDecoder.readLatency * 1e9;
	if (gCell.memCellType == eDRAM3T333 || gCell.memCellType == eDRAM3T) {
    	result["Results"]["Timing"]["Read"]["BitlineLatency_ns"] = bank->mat.subarray.bitlineDelayR * 1e9;
	} else {
		result["Results"]["Timing"]["Read"]["BitlineLatency_ns"] = bank->mat.subarray.bitlineDelay * 1e9;
	}
    if (gInputParameter.internalSensing)
        result["Results"]["Timing"]["Read"]["SenseampLatency_ns"] = 
            bank->mat.subarray.senseAmp.readLatency * 1e9;
    result["Results"]["Timing"]["Read"]["MuxLatency_ns"] =
        (bank->mat.subarray.bitlineMux.readLatency + 
         bank->mat.subarray.senseAmpMuxLev1.readLatency +
         bank->mat.subarray.senseAmpMuxLev2.readLatency) * 1e9;
    result["Results"]["Timing"]["Read"]["PrechargeLatency_ns"] = 
        bank->mat.subarray.precharger.readLatency * 1e9;

    if (gCell.memCellType == PCRAM || gCell.memCellType == FBRAM || 
        gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || 
        gCell.memCellType == MLCRRAM ||
        (gCell.memCellType == memristor && (gCell.accessType == CMOS_access || 
         gCell.accessType == BJT_access))) {

        // RESET latency with proper TreeLatency calculation
        result["Results"]["Timing"]["Reset"]["Latency_ns"] = bank->resetLatency * 1e9;
        result["Results"]["Timing"]["Reset"]["TreeLatency_ns"] = 
            (bank->resetLatency - bank->mat.resetLatency) * 1e9;
        result["Results"]["Timing"]["Reset"]["MatLatency_ns"] = bank->mat.resetLatency * 1e9;
        result["Results"]["Timing"]["Reset"]["PulseDuration_ns"] = gCell.resetPulse * 1e9;

        // SET latency with proper TreeLatency calculation
        result["Results"]["Timing"]["Set"]["Latency_ns"] = bank->setLatency * 1e9;
        result["Results"]["Timing"]["Set"]["TreeLatency_ns"] = 
            (bank->setLatency - bank->mat.setLatency) * 1e9;
        result["Results"]["Timing"]["Set"]["MatLatency_ns"] = bank->mat.setLatency * 1e9;
        result["Results"]["Timing"]["Set"]["PulseDuration_ns"] = gCell.setPulse * 1e9;

    } else if (gCell.memCellType == SLCNAND) {
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
		if (gCell.memCellType == eDRAM3T333 || gCell.memCellType == eDRAM3T) {
			result["Results"]["Timing"]["Write"]["BitlineLatency_ns"] = bank->mat.subarray.bitlineDelayW * 1e9;
		} else {
			result["Results"]["Timing"]["Write"]["BitlineLatency_ns"] = bank->mat.subarray.bitlineDelay * 1e9;
		}
    }

    double readBandwidth = (double)bank->blockSize /
        (bank->mat.subarray.readLatency - bank->mat.subarray.rowDecoder.readLatency
         + bank->mat.subarray.precharger.readLatency) / 8;
    if (gCell.memCellType == MLCCTT || gCell.memCellType == MLCFeFET || 
        gCell.memCellType == MLCRRAM) {
        readBandwidth *= log2(gCell.nLvl);
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

    if (gCell.memCellType == PCRAM || gCell.memCellType == FBRAM || 
        gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || 
        gCell.memCellType == MLCRRAM ||
        (gCell.memCellType == memristor && (gCell.accessType == CMOS_access || 
         gCell.accessType == BJT_access))) {

        result["Results"]["Power"]["Reset"]["DynamicEnergy_pJ"] = bank->resetDynamicEnergy * 1e12;
        result["Results"]["Power"]["Reset"]["CellResetEnergy_pJ"] = 
            bank->mat.subarray.cellResetEnergy * 1e12;

        result["Results"]["Power"]["Set"]["DynamicEnergy_pJ"] = bank->setDynamicEnergy * 1e12;
        result["Results"]["Power"]["Set"]["CellSetEnergy_pJ"] = 
            bank->mat.subarray.cellSetEnergy * 1e12;

    } else if (gCell.memCellType == SLCNAND) {
        result["Results"]["Power"]["Erase"]["DynamicEnergy_pJ"] = bank->resetDynamicEnergy * 1e12;
        result["Results"]["Power"]["Programming"]["DynamicEnergy_pJ"] = bank->setDynamicEnergy * 1e12;

    } else {
        result["Results"]["Power"]["Write"]["DynamicEnergy_pJ"] = bank->writeDynamicEnergy * 1e12;
    }

    result["Results"]["Power"]["Leakage_mW"] = bank->leakage * 1e3;

    if (gCell.memCellType == eDRAM || gCell.memCellType == eDRAM3T || 
        gCell.memCellType == eDRAM3T333) {
        result["Results"]["Power"]["RefreshPower_W"] = 
            (bank->refreshDynamicEnergy / gCell.retentionTime);
    }

    return result;
}

YAML::Node Result::toYamlNodeAsCache(Result& tagResult, CacheAccessMode cacheAccessMode) {
    if (bank->memoryType != dataT || tagResult.bank->memoryType != tag) {
        std::cout << "This is not a valid cache configuration." << std::endl;
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
	switch (gCell.memCellType) {
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
	result["MemoryCell"]["CellArea_F2"]  = gCell.area;
	result["MemoryCell"]["CellArea_um2"] = gCell.area / 1000000.0 * gTech.featureSizeInNano * gTech.featureSizeInNano;
	result["MemoryCell"]["AspectRatio"]  = gCell.aspectRatio;

	// Resistive / Non-volatile memory
	if (gCell.memCellType == PCRAM || gCell.memCellType == MRAM || gCell.memCellType == memristor ||
		gCell.memCellType == FBRAM || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET ||
		gCell.memCellType == MLCRRAM) {

		if (gCell.resistanceOn < 1e3)
			result["MemoryCell"]["R_on_Ohm"] = gCell.resistanceOn;
		else if (gCell.resistanceOn < 1e6)
			result["MemoryCell"]["R_on_KOhm"] = gCell.resistanceOn / 1e3;
		else
			result["MemoryCell"]["R_on_MOhm"] = gCell.resistanceOn / 1e6;

		if (gCell.resistanceOff < 1e3)
			result["MemoryCell"]["R_off_Ohm"] = gCell.resistanceOff;
		else if (gCell.resistanceOff < 1e6)
			result["MemoryCell"]["R_off_KOhm"] = gCell.resistanceOff / 1e3;
		else
			result["MemoryCell"]["R_off_MOhm"] = gCell.resistanceOff / 1e6;

		result["MemoryCell"]["ReadMode"]  = gCell.readMode ? "Voltage-Sensing" : "Current-Sensing";
		if (gCell.readCurrent > 0) result["MemoryCell"]["ReadCurrent_uA"] = gCell.readCurrent * 1e6;
		if (gCell.readVoltage > 0) result["MemoryCell"]["ReadVoltage_V"] = gCell.readVoltage;

		result["MemoryCell"]["ResetMode"] = gCell.resetMode ? "Voltage" : "Current";
		result["MemoryCell"]["ResetVoltage_V"] = gCell.resetVoltage;
		result["MemoryCell"]["ResetCurrent_uA"] = gCell.resetCurrent * 1e6;
		result["MemoryCell"]["ResetPulse_s"] = gCell.resetPulse / 1e9;

		result["MemoryCell"]["SetMode"] = gCell.setMode ? "Voltage" : "Current";
		result["MemoryCell"]["SetVoltage_V"] = gCell.setVoltage;
		result["MemoryCell"]["SetCurrent_uA"] = gCell.setCurrent * 1e6;
		result["MemoryCell"]["SetPulse_s"] = gCell.setPulse / 1e9;

		switch (gCell.accessType) {
			case CMOS_access: result["MemoryCell"]["AccessType"] = "CMOS"; break;
			case BJT_access: result["MemoryCell"]["AccessType"] = "BJT"; break;
			case diode_access: result["MemoryCell"]["AccessType"] = "Diode"; break;
			default: result["MemoryCell"]["AccessType"] = "None Access Device"; break;
		}
	}

	// SRAM
	if (gCell.memCellType == SRAM) {
		result["MemoryCell"]["WidthAccessCMOS_F"]   = gCell.widthAccessCMOS;
		result["MemoryCell"]["WidthSRAMCellNMOS_F"] = gCell.widthSRAMCellNMOS;
		result["MemoryCell"]["WidthSRAMCellPMOS_F"] = gCell.widthSRAMCellPMOS;
		result["MemoryCell"]["PeripheralRoadmap"]   = roadmapToString(gTech.deviceRoadmap);
		result["MemoryCell"]["PeripheralNode_nm"]   = gTech.featureSizeInNano;
		result["MemoryCell"]["VDD_V"]               = gTech.vdd;
		result["MemoryCell"]["Temperature_K"]       = gCell.temperature;
	}

	// DRAM / eDRAM
	if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
		result["MemoryCell"]["WidthAccessCMOS_F"] = gCell.widthAccessCMOS;
		result["MemoryCell"]["PeripheralRoadmap"] = roadmapToString(gTech.deviceRoadmap);
		result["MemoryCell"]["PeripheralNode_nm"] = gTech.featureSizeInNano;
		result["MemoryCell"]["VDD_V"] = gTech.vdd;
		result["MemoryCell"]["WL_SWING"] = gTech.vpp;
		result["MemoryCell"]["Temperature_K"] = gCell.temperature;
	}

	// 3T DRAM
	if (gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
		result["MemoryCell"]["WidthWriteAccessCMOS_F"] = gCell.widthAccessCMOS;
		result["MemoryCell"]["WidthReadAccessCMOS_F"]  = gCell.widthAccessCMOSR;
		result["MemoryCell"]["PeripheralRoadmap"]      = roadmapToString(gTech.deviceRoadmap);
		result["MemoryCell"]["WriteAccessRoadmap"]     = roadmapToString(gTechW.deviceRoadmap);
		result["MemoryCell"]["ReadAccessRoadmap"]      = roadmapToString(gTechR.deviceRoadmap);
		result["MemoryCell"]["PeripheralNode_nm"]      = gTech.featureSizeInNano;
		result["MemoryCell"]["WriteAccessNode_nm"]     = gTechW.featureSizeInNano;
		result["MemoryCell"]["ReadAccessNode_nm"]      = gTechR.featureSizeInNano;
		result["MemoryCell"]["VDD_V"]                  = gTech.vdd;
		result["MemoryCell"]["WWL_SWING"]              = gTechW.vpp;
		result["MemoryCell"]["Temperature_K"]          = gCell.temperature;
	}

	// SLC NAND Flash
	if (gCell.memCellType == SLCNAND) {
		result["MemoryCell"]["PassVoltage_V"]     = gCell.flashPassVoltage;
		result["MemoryCell"]["ProgramVoltage_V"]  = gCell.flashProgramVoltage;
		result["MemoryCell"]["EraseVoltage_V"]    = gCell.flashEraseVoltage;
		result["MemoryCell"]["ProgramTime_s"]     = gCell.flashProgramTime / 1e9;
		result["MemoryCell"]["EraseTime_s"]       = gCell.flashEraseTime / 1e9;
		result["MemoryCell"]["GateCouplingRatio"] = gCell.gateCouplingRatio;
	}

	// Multi-level cells
	if (gCell.memCellType == MLCCTT || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
		result["MemoryCell"]["NumberOfInputFingers"]   = gCell.nFingers;
		result["MemoryCell"]["NumberOfLevelsPerCell"] = gCell.nLvl;
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

	switch (gInputParameter.designTarget) {
		case cache: result["CacheDesign"]["DesignTarget"] = "Cache"; break;
		case RAM_chip: result["CacheDesign"]["DesignTarget"] = "RAMChip"; break;
		case CAM_chip: result["CacheDesign"]["DesignTarget"] = "CAMChip"; break;
		default: result["CacheDesign"]["DesignTarget"] = "Unknown"; break;
	}

	//Capacity
	if (gInputParameter.capacity < 1024) {
		result["Capacity"]["Value"] = gInputParameter.capacity;
		result["Capacity"]["Unit"] = "B";
	} else if (gInputParameter.capacity < 1024 * 1024) {
		result["Capacity"]["Value"] = gInputParameter.capacity / 1024;
		result["Capacity"]["Unit"] = "KB";
	} else if (gInputParameter.capacity < 1024 * 1024 * 1024) {
		result["Capacity"]["Value"] = gInputParameter.capacity / 1024 / 1024;
		result["Capacity"]["Unit"] = "MB";
	} else {
		result["Capacity"]["Value"] = gInputParameter.capacity / 1024 / 1024 / 1024;
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

    if (gCell.memCellType == eDRAM) {
        result["CacheDesign"]["Timing"]["CacheRefreshLatency_us"] =
            MAX(tagResult.bank->refreshLatency, bank->refreshLatency) * 1e6;
        result["CacheDesign"]["Timing"]["CacheAvailability_percent"] =
            ((gCell.retentionTime - MAX(tagResult.bank->refreshLatency, bank->refreshLatency)) /
             gCell.retentionTime) * 100.0;
    }

    result["CacheDesign"]["Power"]["CacheHitDynamicEnergy_nJ"] = cacheHitDynamicEnergy * 1e9;
    result["CacheDesign"]["Power"]["CacheMissDynamicEnergy_nJ"] = cacheMissDynamicEnergy * 1e9;
    result["CacheDesign"]["Power"]["CacheWriteDynamicEnergy_nJ"] = cacheWriteDynamicEnergy * 1e9;

    if (gCell.memCellType == eDRAM) {
        result["CacheDesign"]["Power"]["CacheRefreshDynamicEnergy_nJ"] =
            (tagResult.bank->refreshDynamicEnergy + bank->refreshDynamicEnergy) * 1e9;
    }

    result["CacheDesign"]["Power"]["CacheTotalLeakagePower_mW"] = cacheLeakage * 1e3;
    result["CacheDesign"]["Power"]["CacheDataArrayLeakagePower_mW"] = bank->leakage * 1e3;
    result["CacheDesign"]["Power"]["CacheTagArrayLeakagePower_mW"] = tagResult.bank->leakage * 1e3;

    if (gCell.memCellType == eDRAM || gCell.memCellType == eDRAM3T || 
        gCell.memCellType == eDRAM3T333) {
        result["CacheDesign"]["Power"]["CacheRefreshPower_W"] =
            (bank->refreshDynamicEnergy / gCell.retentionTime);
        result["CacheDesign"]["Power"]["CacheRetentionTime_ns"] = gCell.retentionTime * 1e9;
    }

    // Add reset and set pulse durations (using global gCell pointer safely)
    //if (gCell) {
        result["CacheDesign"]["Timing"]["Reset"]["PulseDuration_ns"] =
            MAX(gCell.resetPulse, gCell.resetPulse) * 1e9;
        result["CacheDesign"]["Timing"]["Set"]["PulseDuration_ns"] =
            MAX(gCell.setPulse, gCell.setPulse) * 1e9;
    //} else {
        //result["CacheDesign"]["Timing"]["Reset"]["PulseDuration_ns"] = 0;
        //result["CacheDesign"]["Timing"]["Set"]["PulseDuration_ns"] = 0;
    //}

    // Add data and tag details
    result["DataArray"] = toYamlNode();
    result["TagArray"] = tagResult.toYamlNode();

    return result;
}



void Result::printToYamlFile(std::ofstream& outputFile) {
    YAML::Node node = toYamlNode();
    outputFile << node << std::endl;
}

void Result::printAsCacheToYamlFile(Result& tagResult, CacheAccessMode cacheAccessMode, std::ofstream& outputFile) {
    YAML::Node node = toYamlNodeAsCache(tagResult, cacheAccessMode);
    outputFile << node << std::endl;
}

