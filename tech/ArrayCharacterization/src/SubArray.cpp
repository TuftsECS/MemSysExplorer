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


#include "SubArray.hpp"

#include "formula.hpp"
#include "global.hpp"
#include "constant.hpp"
#include "cmos.hpp"
#include "cell/types.hpp"
#include "cell/visitors.hpp"

#include <math.h>
#include <iomanip>
#include <cassert>
#include <memory>

void SubArray::Initialize(long long _numRow, long long _numColumn, bool _multipleRowPerSet, bool _split,
		int _muxSenseAmp, bool _internalSenseAmp, int _muxOutputLev1, int _muxOutputLev2,
		BufferDesignTarget _areaOptimizationLevel) {
	if (initialized)
		std::cout << "[Subarray] Warning: Already initialized!" << std::endl;

	numRow = _numRow;
	numColumn = _numColumn;
	multipleRowPerSet = _multipleRowPerSet;
	split = _split;
	muxSenseAmp = _muxSenseAmp;
	muxOutputLev1 = _muxOutputLev1;
	muxOutputLev2 = _muxOutputLev2;
	internalSenseAmp = _internalSenseAmp;
	areaOptimizationLevel = _areaOptimizationLevel;

	double maxWordlineCurrent = 0;
	double maxBitlineCurrent = 0;

	/* Check if the configuration is legal */
	if (gInputParameter.designTarget == cache && gInputParameter.cacheAccessMode != sequential_access_mode) {
		/* In these cases, each column should hold part of data in all the ways */
		if (numColumn < gInputParameter.associativity) {
			invalid = true;
			initialized = true;
			return;
		}
	}

	if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM || gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
		if (muxSenseAmp > 1) {
			/* DRAM does not allow muxed bitline because of its destructive readout */
			invalid = true;
			initialized = true;
			return;
		}
	}

	if (gCell.memCellType == SLCNAND) {
		if (numRow < gInputParameter.flashBlockSize / gInputParameter.pageSize) {
			/* SLC NAND does not have enough rows to hold the page count */
			invalid = true;
			initialized = true;
			return;
		}
		if (internalSenseAmp && muxSenseAmp < 2) {
			/* There is no way to put the sense amp */
			invalid = true;
			initialized = true;
			return;
		}
	}

	if (gCell.memCellType == memristor || gCell.memCellType == FBRAM || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
		if (internalSenseAmp && muxSenseAmp < 2) {
			/* There is no way to put the sense amp */
			invalid = true;
			initialized = true;
			return;
		}
	}

	if (gCell.memCellType == FBRAM) {
		if (gCell.resistanceOff / gCell.resistanceOn < numRow / BITLINE_LEAKAGE_TOLERANCE) {
			/* bitline too long */
			invalid = true;
			initialized = true;
			return;
		}
		maxBitlineCurrent = std::max(gCell.resetCurrent, gCell.setCurrent) + gCell.leakageCurrentAccessDevice * (numRow - 1);
	}

	if (gCell.memCellType == MRAM || gCell.memCellType == PCRAM || gCell.memCellType == memristor || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
		if (gCell.accessType == CMOS_access){
			if (gTech.currentOnNmos[gInputParameter.temperature - 300]
									/ gTech.currentOffNmos[gInputParameter.temperature - 300] < numRow / BITLINE_LEAKAGE_TOLERANCE) {
				/* bitline too long */
				invalid = true;
				initialized = true;
				return;
			}
			maxBitlineCurrent = std::max(gCell.resetCurrent, gCell.setCurrent) + gCell.leakageCurrentAccessDevice * (numRow - 1);
		} else { //non-CMOS access
		//	if (!gCell.readFloating) { // conventional half select read scheme
		//		if ((2 * gCell.resistanceOnAtHalfReadVoltage / (numRow - 1)) < (gCell.resistanceOffAtReadVoltage / BITLINE_LEAKAGE_TOLERANCE)){
		//			/* bitline too long */
		//			invalid = true;
		//			initialized = true;
		//			return;
		//		}
		//	} else { //Floating wordline and bitline to reduce bypass leakage */
		//		double r, c; // number of rows and columns in a memristor array of which wordline voltage is to be calculated
		//		r = numRow;
		//		c = numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
		//		double equResistanceOn = gCell.GetMemristance((c - 1) / (r + c - 1)); //Solved Wordline Voltage is (c-1)/(r+c-1) * Vread
		//		if (((c - 1) / (r + c - 1) * equResistanceOn / (numRow - 1)) < (gCell.resistanceOffAtReadVoltage / BITLINE_LEAKAGE_TOLERANCE)){
		//			/* bitline too long */
		//			invalid = true;
		//			initialized = true;
		//			return;
		//		}
		//	}
			/* Write half select problem limit the array size */
			double resetCurrent;
			if (gCell.resetCurrent == 0) {
				resetCurrent = (fabs (gCell.resetVoltage) - gCell.voltageDropAccessDevice) / gCell.resistanceOnAtResetVoltage;
			} else
				resetCurrent = gCell.resetCurrent;
			int numSelectedColumnPerRow = numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
			if (gCell.accessType == none_access) {
				maxWordlineCurrent = resetCurrent * numSelectedColumnPerRow + resetCurrent * gCell.resistanceOnAtResetVoltage
						/ 2 / gCell.resistanceOnAtHalfResetVoltage * (numColumn - numSelectedColumnPerRow);
			} else { //diode or BJT
				maxWordlineCurrent = resetCurrent * numSelectedColumnPerRow + gCell.leakageCurrentAccessDevice
						* (numColumn - numSelectedColumnPerRow);
			}
			double minWordlineDriverWidth = maxWordlineCurrent / gTech.currentOnNmos[gInputParameter.temperature - 300];
			if (minWordlineDriverWidth > gInputParameter.maxNmosSize * gTech.featureSize) {
				invalid = true;
				return;
			}
			if (gCell.accessType == none_access) {
				maxBitlineCurrent = resetCurrent + resetCurrent * gCell.resistanceOnAtResetVoltage / 2
						/ gCell.resistanceOnAtHalfResetVoltage * (numRow - 1);
			} else { //diode or BJT
				maxBitlineCurrent = resetCurrent + gCell.leakageCurrentAccessDevice * (numRow - 1);
			}
		}
	}

	double minBitlineMuxWidth = std::max(MIN_NMOS_SIZE * gTech.featureSize, maxBitlineCurrent / gTech.currentOnNmos[gInputParameter.temperature - 300]);
	if (minBitlineMuxWidth > gInputParameter.maxNmosSize * gTech.featureSize) {
		invalid = true;
		return;
	}

	if (internalSenseAmp) {
		if (gCell.memCellType == SRAM || gCell.memCellType == DRAM || gCell.memCellType == eDRAM || gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
			/* SRAM, DRAM, and eDRAM all use voltage sensing */
			voltageSense = true;
		} else if (gCell.memCellType == MRAM || gCell.memCellType == PCRAM || gCell.memCellType == memristor || gCell.memCellType == FBRAM || gCell.memCellType == CTT || gCell.memCellType == MLCCTT || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM)  {
			voltageSense = gCell.readMode;
		} else {/* NAND flash */
			voltageSense = true;
		}
	} else if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM || gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
		std::cout << "[Subarray] Error: DRAM does not support external sense amplifiers!" << std::endl;
		exit(-1);
	}

	if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
		senseVoltage = gTech.vdd / 2 * gCell.capDRAMCell / (gCell.capDRAMCell + capBitline);
		if (senseVoltage < gCell.minSenseVoltage) {		/* Bitline is too long */
			invalid = true;
			initialized = true;
			return;
		}
	} else if (gCell.memCellType == SLCNAND){
		/* suppose the reference voltage is 0.5Vdd, the initial bitline voltage is 0.6Vdd
		 * if the bitline drops to 0.4Vdd, the senseamp can tell which data is stored */
		senseVoltage = std::max(gCell.minSenseVoltage, 0.2 * gTech.vdd);
	} else {
		/* TO-DO: different memory technology might have different values here */
		senseVoltage = gCell.minSenseVoltage;
	}

	/* Derived parameters */
	numSenseAmp = numColumn / muxSenseAmp;
	lenWordline = (double)numColumn * gCell.widthInFeatureSize * gTech.featureSize;
	lenBitline = (double)numRow * gCell.heightInFeatureSize * gTech.featureSize;
	/* Add stitching overhead if necessary */
	if (gCell.stitching) {
		lenWordline += ((numColumn - 1) / gCell.stitching + 1) * STITCHING_OVERHEAD * gTech.featureSize;
	}
	/* Add select transistors into the length calculation */
	if (gCell.memCellType == SLCNAND) {
		int pageCount = gInputParameter.flashBlockSize / gInputParameter.pageSize;
		/* Two select transistor including contacts have total length of 5F */
		lenBitline += (numRow / pageCount) * 5 * gTech.featureSize;
	}

    // Add access transistor for CTT and MLCCTT 
    if ((gCell.memCellType == CTT || gCell.memCellType == MLCCTT)) {
        lenBitline += 5 * gTech.featureSize;
    }



	/* Calculate wire resistance/capacitance */
	capWordline = lenWordline * gLocalWire.capWirePerUnit;
	resWordline = lenWordline * gLocalWire.resWirePerUnit;
	capBitline = lenBitline * gLocalWire.capWirePerUnit;
	resBitline = lenBitline * gLocalWire.resWirePerUnit;

	/* Caclulate the load resistance and capacitance for Mux Decoders */
    double resMuxLoad = resWordline;
    double capMuxLoad = CalculateGateCap(minBitlineMuxWidth, gTech) * numColumn + capWordline;

	if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
		senseVoltage = gTech.vdd / 2 * gCell.capDRAMCell / (gCell.capDRAMCell + capBitline);
		if (senseVoltage < gCell.minSenseVoltage) {		/* Bitline is too long */
			invalid = true;
			initialized = true;
			return;
		}
	} else if (gCell.memCellType == SLCNAND){
		/* suppose the reference voltage is 0.5Vdd, the initial bitline voltage is 0.6Vdd
		 * if the bitline drops to 0.4Vdd, the senseamp can tell which data is stored */
		senseVoltage = std::max(gCell.minSenseVoltage, 0.2 * gTech.vdd);
	} else {
		/* TO-DO: different memory technology might have different values here */
		senseVoltage = gCell.minSenseVoltage;
	}

    mse::cell::MemoryCell* cellPtr = mse::variantBasePtr<mse::cell::MemoryCell>(cell);
    const mse::cell::MemoryCellTopology& topology = cellPtr->topology();
    mse::unit::Farad wordlineMaxCapacitance;
    for (unsigned a = 0; a < topology.wordlineCount(); ++a) {
        if (mse::unit::Farad cap = topology.wordlineCapacitance(a); cap > wordlineMaxCapacitance) {
            wordlineMaxCapacitance = cap;
        }
    }
    capWordline += wordlineMaxCapacitance.value() * numColumn;
    mse::unit::Farad bitlineMaxCapacitance;
    for (unsigned a = 0; a < topology.bitlineCount(); ++a) {
        if (mse::unit::Farad cap = topology.bitlineCapacitance(a); cap > bitlineMaxCapacitance) {
            bitlineMaxCapacitance = cap;
        }
    }
    capBitline += bitlineMaxCapacitance.value() * numRow * (cellPtr->properties().sharedContact ? 0.5 : 1.0);
    voltagePrecharge = cellPtr->properties().prechargeVoltage.value();

	/* Initialize sub-component */

	precharger.Initialize(gTech.vdd, numColumn, capBitline, resBitline);
	precharger.CalculateRC();

	rowDecoder.Initialize(numRow, capWordline, resWordline, multipleRowPerSet, areaOptimizationLevel, maxWordlineCurrent);
	if (rowDecoder.invalid) {
		invalid = true;
		return;
	}
	rowDecoder.CalculateRC();

	if (!invalid) {
		bitlineMuxDecoder.Initialize(muxSenseAmp, capMuxLoad, resMuxLoad /* TO-DO: need to fix */, false, areaOptimizationLevel, 0);
		if (bitlineMuxDecoder.invalid)
			invalid = true;
		else
			bitlineMuxDecoder.CalculateRC();
	}

	if (!invalid) {
		senseAmpMuxLev1Decoder.Initialize(muxOutputLev1, capMuxLoad, resMuxLoad /* TO-DO: need to fix */, false, areaOptimizationLevel, 0);
		if (senseAmpMuxLev1Decoder.invalid)
			invalid = true;
		else
			senseAmpMuxLev1Decoder.CalculateRC();
	}

	if (!invalid) {
		senseAmpMuxLev2Decoder.Initialize(muxOutputLev2, capMuxLoad, resMuxLoad /* TO-DO: need to fix */, false, areaOptimizationLevel, 0);
		if (senseAmpMuxLev2Decoder.invalid)
			invalid = true;
		else
			senseAmpMuxLev2Decoder.CalculateRC();
	}

	senseAmpMuxLev2.Initialize(muxOutputLev2, numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2, 0, 0 /* TO-DO: need to fix */, maxBitlineCurrent);
	senseAmpMuxLev2.CalculateRC();

	senseAmpMuxLev1.Initialize(muxOutputLev1, numColumn / muxSenseAmp / muxOutputLev1,
			senseAmpMuxLev2.capForPreviousDelayCalculation, senseAmpMuxLev2.capForPreviousPowerCalculation, maxBitlineCurrent);
	senseAmpMuxLev1.CalculateRC();
	if (internalSenseAmp) {
		if (!invalid) {
			senseAmp.Initialize(numSenseAmp, !voltageSense, senseVoltage, lenWordline / numColumn * muxSenseAmp, mse::variantBasePtr<mse::cell::MemoryCell>(cell)->properties().isMLC, gCell.nLvl, gCell.nFingers);
			if (senseAmp.invalid)
				invalid = true;
			else
				senseAmp.CalculateRC();
		}
		if (!invalid) {
			bitlineMux.Initialize(muxSenseAmp, numColumn / muxSenseAmp, senseAmp.capLoad, senseAmp.capLoad, maxBitlineCurrent);
		}
	} else {
		if (!invalid) {
			bitlineMux.Initialize(muxSenseAmp, numColumn / muxSenseAmp,
					senseAmpMuxLev1.capForPreviousDelayCalculation, senseAmpMuxLev1.capForPreviousPowerCalculation, maxBitlineCurrent);
		}
	}

	if (!invalid) {
		bitlineMux.CalculateRC();
	}

	//Qing: initialize subarray buffer,
	//do not consider buffering control signals for now
	subarrayBuffer.Initialize(1, numColumn);
	subarrayBuffer.CalculateArea();
	subarrayBuffer.CalculateLatency();
	subarrayBuffer.CalculatePower();
	//Qing.

	initialized = true;
}

void SubArray::CalculateArea() {
	if (!initialized) {
		std::cout << "[Subarray] Error: Require initialization first!" << std::endl;
	} else if (invalid) {
		height = width = area = invalid_value;
	} else {
		double addWidth = 0, addHeight = 0;
		double widthPeripherals = 0, heightPeripherals = 0;

		width = lenWordline;
		height = lenBitline;

		rowDecoder.CalculateArea();
		if (rowDecoder.height > height) {
			/* assume magic folding */
			addWidth = rowDecoder.area / height;
		} else {
			/* allow white space */
			addWidth = rowDecoder.width;
		}

		precharger.CalculateArea();
		if (precharger.width > width) {
			/* assume magic folding */
			addHeight = precharger.area / precharger.width;
		} else {
			/* allow white space */
			addHeight = precharger.height;
		}

		bitlineMux.CalculateArea();
		addHeight += bitlineMux.height;

		if (internalSenseAmp) {
			senseAmp.CalculateArea();
			if (senseAmp.width > width * 1.001) {
				/* should never happen */
				std::cout << "[ERROR] Sense Amplifier area calculation is wrong!" << std::endl;
			} else {
                //cout << addHeight*1E6 << " -> ";
				addHeight += senseAmp.height;
                //cout << addHeight*1E6 << std::endl;
			}
		}

		senseAmpMuxLev1.CalculateArea();
		addHeight += senseAmpMuxLev1.height;

		senseAmpMuxLev2.CalculateArea();
		addHeight += senseAmpMuxLev2.height;

		bitlineMuxDecoder.CalculateArea();
		addWidth = std::max(addWidth, bitlineMuxDecoder.width);
		senseAmpMuxLev1Decoder.CalculateArea();
		addWidth = std::max(addWidth, senseAmpMuxLev1Decoder.width);
		senseAmpMuxLev2Decoder.CalculateArea();
		addWidth = std::max(addWidth, senseAmpMuxLev2Decoder.width);

		//Qing: add subarray buffer's height
		//assume the buffer has the same width as the subarray
		addHeight += subarrayBuffer.height;
		//Qing.

		width += addWidth;
		height += addHeight;

		if (gCell.memCellType == eDRAM3T333) {
			// monolithic 3D cells fit above peripherals
			widthPeripherals = (width - lenWordline);
			heightPeripherals = (height - lenBitline);
			width = std::max(width, widthPeripherals);
			height = std::max(height, heightPeripherals);
		}
		area = width * height;
	}
}

void SubArray::CalculateLatency(double _rampInput) {
	if (!initialized) {
		std::cout << "[Subarray] Error: Require initialization first!" << std::endl;
	} else if (invalid) {
		readLatency = writeLatency = invalid_value;
	} else {
		precharger.CalculateLatency(_rampInput);
		rowDecoder.CalculateLatency(_rampInput);
		bitlineMuxDecoder.CalculateLatency(_rampInput);
		senseAmpMuxLev1Decoder.CalculateLatency(_rampInput);
		senseAmpMuxLev2Decoder.CalculateLatency(_rampInput);
		columnDecoderLatency = std::max({bitlineMuxDecoder.readLatency, senseAmpMuxLev1Decoder.readLatency, senseAmpMuxLev2Decoder.readLatency});
		double decoderLatency = std::max(rowDecoder.readLatency, columnDecoderLatency);
		/*need a second thought on this equation*/
		double capPassTransistor = bitlineMux.capNMOSPassTransistor +
				senseAmpMuxLev1.capNMOSPassTransistor + senseAmpMuxLev2.capNMOSPassTransistor;
		double resPassTransistor = bitlineMux.resNMOSPassTransistor +
				senseAmpMuxLev1.resNMOSPassTransistor + senseAmpMuxLev2.resNMOSPassTransistor;
		double tauChargeLatency = resPassTransistor * (capPassTransistor + capBitline) + resBitline * capBitline / 2;
		chargeLatency = horowitz(tauChargeLatency, 0, 1e20, nullptr);

        if (internalSenseAmp) {
            senseAmp.CalculateLatency();
        }
        senseAmpMuxLev1.CalculateLatency();
        senseAmpMuxLev2.CalculateLatency();

        bitlineMux.CalculateLatency();

        mse::cell::LatencyVisitor visitor(*this);
        mse::cell::MemoryCellLatency latency = std::visit(visitor, cell);

        // TODO this is for legacy compat
        mse::GenericVisitor v {
            [this](const mse::cell::Sram6tCell::Latency& l) {
                bitlineDelayR = l.bitline.value();
                bitlineDelayW = bitlineDelayR;
                readLatency = l.read.value();
                writeLatency = l.write.value();
            },
            [this](const mse::cell::DramCell::Latency& l) {
                bitlineDelayR = l.bitline.value();
                bitlineDelayW = bitlineDelayR;
                readLatency = l.read.value();
                writeLatency = l.write.value();
                refreshLatency = l.refresh.value();
            },
            [this](const mse::cell::EdramCell::Latency& l) {
                bitlineDelayR = l.bitline.value();
                bitlineDelayW = bitlineDelayR;
                readLatency = l.read.value();
                writeLatency = l.write.value();
                refreshLatency = l.refresh.value();
            },
            [this](const mse::cell::Edram3tCell::Latency& l) {
                bitlineDelayR = l.bitlineR.value();
                bitlineDelayW = l.bitlineW.value();
                readLatency = l.read.value();
                writeLatency = l.write.value();
                refreshLatency = l.refresh.value();
            },
            [this](const mse::cell::Edram3t333Cell::Latency& l) {
                bitlineDelayR = l.bitlineR.value();
                bitlineDelayW = l.bitlineW.value();
                readLatency = l.read.value();
                writeLatency = l.write.value();
                refreshLatency = l.refresh.value();
            }
        };
        std::visit(v, latency);
        readLatency += decoderLatency + 0 + senseAmp.readLatency + senseAmpMuxLev1.readLatency + senseAmpMuxLev2.readLatency;
	}
}

void SubArray::CalculatePower() {
	if (!initialized) {
		std::cout << "[Subarray] Error: Require initialization first!" << std::endl;
	} else if (invalid) {
		readDynamicEnergy = writeDynamicEnergy = leakage = invalid_value;
	} else {
		precharger.CalculatePower();
		rowDecoder.CalculatePower();
		bitlineMuxDecoder.CalculatePower();
		senseAmpMuxLev1Decoder.CalculatePower();
		senseAmpMuxLev2Decoder.CalculatePower();
		bitlineMux.CalculatePower();
		if (internalSenseAmp) {
			senseAmp.CalculatePower();
		}
		senseAmpMuxLev1.CalculatePower();
		senseAmpMuxLev2.CalculatePower();

        mse::cell::EnergyVisitor visitor(*this);
        mse::cell::MemoryCellEnergy energy = std::visit(visitor, cell);

        mse::GenericVisitor v {
            [this](const mse::cell::Sram6tCell::Energy& e) {
                readDynamicEnergy = e.read.value();
                writeDynamicEnergy = e.write.value();
            },
            [this](const mse::cell::DramCell::Energy& e) {
                readDynamicEnergy = e.read.value();
                writeDynamicEnergy = e.write.value();
                refreshDynamicEnergy = e.refresh.value();
            },
            [this](const mse::cell::EdramCell::Energy& e) {
                readDynamicEnergy = e.read.value();
                writeDynamicEnergy = e.write.value();
                refreshDynamicEnergy = e.refresh.value();
            },
            [this](const mse::cell::Edram3tCell::Energy& e) {
                readDynamicEnergy = e.read.value();
                writeDynamicEnergy = e.write.value();
                refreshDynamicEnergy = e.refresh.value();
            },
            [this](const mse::cell::Edram3t333Cell::Energy& e) {
                readDynamicEnergy = e.read.value();
                writeDynamicEnergy = e.write.value();
                refreshDynamicEnergy = e.refresh.value();
            }
        };
        std::visit(v, energy);

        auto ptr = mse::variantBasePtr<mse::cell::MemoryCell>(cell);
        leakage = ptr->topology().leakageCurrent().value() * gTech.vdd * numColumn;
        if (ptr->properties().columnLeak) {
            leakage *= numRow;
        }

		if (gInputParameter.designTarget == cache && gInputParameter.cacheAccessMode != sequential_access_mode) {
			cellResetEnergy /= gInputParameter.associativity;
			cellSetEnergy /= gInputParameter.associativity;
			writeDynamicEnergy /= gInputParameter.associativity;
			resetDynamicEnergy /= gInputParameter.associativity;
			setDynamicEnergy /= gInputParameter.associativity;
		}

		readDynamicEnergy += cellReadEnergy + rowDecoder.readDynamicEnergy + bitlineMuxDecoder.readDynamicEnergy + senseAmpMuxLev1Decoder.readDynamicEnergy
				+ senseAmpMuxLev2Decoder.readDynamicEnergy + precharger.readDynamicEnergy + bitlineMux.readDynamicEnergy
				+ senseAmp.readDynamicEnergy + senseAmpMuxLev1.readDynamicEnergy + senseAmpMuxLev2.readDynamicEnergy;
		writeDynamicEnergy += rowDecoder.writeDynamicEnergy + bitlineMuxDecoder.writeDynamicEnergy + senseAmpMuxLev1Decoder.writeDynamicEnergy
				+ senseAmpMuxLev2Decoder.writeDynamicEnergy + bitlineMux.writeDynamicEnergy
				+ senseAmp.writeDynamicEnergy + senseAmpMuxLev1.writeDynamicEnergy + senseAmpMuxLev2.writeDynamicEnergy;

        /* Read all column energy + row decoder + sense amp + precharger is enough for one subarray row REF. */
        refreshDynamicEnergy += rowDecoder.readDynamicEnergy + precharger.readDynamicEnergy
                             + senseAmp.readDynamicEnergy;
        refreshDynamicEnergy *= numRow; // Energy for this entire subarray 

		/* for assymetric RESET and SET latency calculation only */
		setDynamicEnergy += cellSetEnergy + rowDecoder.setDynamicEnergy + bitlineMuxDecoder.writeDynamicEnergy + senseAmpMuxLev1Decoder.writeDynamicEnergy
				+ senseAmpMuxLev2Decoder.writeDynamicEnergy + bitlineMux.writeDynamicEnergy
				+ senseAmp.writeDynamicEnergy + senseAmpMuxLev1.writeDynamicEnergy + senseAmpMuxLev2.writeDynamicEnergy;
		resetDynamicEnergy += setDynamicEnergy + rowDecoder.resetDynamicEnergy + bitlineMuxDecoder.writeDynamicEnergy + senseAmpMuxLev1Decoder.writeDynamicEnergy
				+ senseAmpMuxLev2Decoder.writeDynamicEnergy + bitlineMux.writeDynamicEnergy
				+ senseAmp.writeDynamicEnergy + senseAmpMuxLev1.writeDynamicEnergy + senseAmpMuxLev2.writeDynamicEnergy;

		if (gCell.accessType == diode_access || gCell.accessType == none_access) {
			writeDynamicEnergy += bitlineMux.writeDynamicEnergy + senseAmp.writeDynamicEnergy
					+ senseAmpMuxLev1.writeDynamicEnergy + senseAmpMuxLev2.writeDynamicEnergy;
		}
		leakage += rowDecoder.leakage + bitlineMuxDecoder.leakage + senseAmpMuxLev1Decoder.leakage
				+ senseAmpMuxLev2Decoder.leakage + precharger.leakage + bitlineMux.leakage
				+ senseAmp.leakage + senseAmpMuxLev1.leakage + senseAmpMuxLev2.leakage
				//Qing: subarray buffer leakage
				+ subarrayBuffer.leakage + subarrayBuffer.xorLeakage;
	}
}

void SubArray::PrintProperty() {
	std::cout << "Subarray Properties:" << std::endl;
	FunctionUnit::PrintProperty();
	std::cout << "numRow:" << numRow << " numColumn:" << numColumn << std::endl;
	std::cout << "lenWordline * lenBitline = " << lenWordline*1e6 << "um * " << lenBitline*1e6 << "um = " << lenWordline * lenBitline * 1e6 << "mm^2" << std::endl;
	std::cout << "Row Decoder Area:" << rowDecoder.height*1e6 << "um x " << rowDecoder.width*1e6 << "um = " << rowDecoder.area*1e6 << "mm^2" << std::endl;
	std::cout << "Sense Amplifier Area:" << std::scientific << senseAmp.height*1e6 << "um x " << senseAmp.width*1e6 << "um = " << senseAmp.area*1e6 << "mm^2" << std::fixed << std::endl;
	std::cout << "Subarray Area Efficiency = " << lenWordline * lenBitline / area * 100 <<"%" << std::endl;
	std::cout << "bitlineDelay: " << bitlineDelayR*1e12 << "ps" << std::endl;
	std::cout << "chargeLatency: " << chargeLatency*1e12 << "ps" << std::endl;
	std::cout << "columnDecoderLatency: " << columnDecoderLatency*1e12 << "ps" << std::endl;
}
