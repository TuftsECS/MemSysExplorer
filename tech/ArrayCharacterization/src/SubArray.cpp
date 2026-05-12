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

#include <math.h>
#include <iomanip>

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
		maxBitlineCurrent = MAX(gCell.resetCurrent, gCell.setCurrent) + gCell.leakageCurrentAccessDevice * (numRow - 1);
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
			maxBitlineCurrent = MAX(gCell.resetCurrent, gCell.setCurrent) + gCell.leakageCurrentAccessDevice * (numRow - 1);
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

	double minBitlineMuxWidth = maxBitlineCurrent / gTech.currentOnNmos[gInputParameter.temperature - 300];
	minBitlineMuxWidth = MAX(MIN_NMOS_SIZE * gTech.featureSize, minBitlineMuxWidth);
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
		senseVoltage = MAX(gCell.minSenseVoltage, 0.2 * gTech.vdd);
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
	double capMuxLoad, resMuxLoad;
        resMuxLoad = resWordline;
        capMuxLoad = CalculateGateCap(minBitlineMuxWidth, gTech) * numColumn;
        capMuxLoad += capWordline;

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
		senseVoltage = MAX(gCell.minSenseVoltage, 0.2 * gTech.vdd);
	} else {
		/* TO-DO: different memory technology might have different values here */
		senseVoltage = gCell.minSenseVoltage;
	}

	/* Add transistor resistance/capacitance */
	if (gCell.memCellType == SRAM) {
		/* SRAM has two access transistors */
		resCellAccess = CalculateOnResistance(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, NMOS, gInputParameter.temperature, gTech);
		capCellAccess = CalculateDrainCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, NMOS, gCell.widthInFeatureSize * gTech.featureSize, gTech);
		capWordline += 2 * CalculateGateCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, gTech) * numColumn;
		capBitline  += capCellAccess * numRow / 2;	/* Due to shared contact */
		voltagePrecharge = gTech.vdd / 2;	/* SRAM read voltage is always half of vdd */
	} else if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
		/* DRAM and eDRAM only has one access transistors */
		resCellAccess = CalculateOnResistance(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, NMOS, gInputParameter.temperature, gTech);
		capCellAccess = CalculateDrainCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, NMOS, gCell.widthInFeatureSize * gTech.featureSize, gTech);
		capWordline += CalculateGateCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, gTech) * numColumn;
		capBitline  += capCellAccess * numRow / 2;	/* Due to shared contact */
		voltagePrecharge = gTech.vdd / 2;	/* DRAM read voltage is always half of vdd */
	} else if (gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
		/* DRAM and eDRAM only has one access transistors */
		resCellAccessW = CalculateOnResistance(((gTechW.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTechW.featureSize, NMOS, gInputParameter.temperature, gTechW);
		resCellAccessR = CalculateOnResistance(((gTechR.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOSR * gTechR.featureSize, NMOS, gInputParameter.temperature, gTechR);
		capCellAccessW = CalculateDrainCap(((gTechW.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTechW.featureSize, NMOS, gCell.widthInFeatureSize * gTech.featureSize, gTechW);
		capCellAccessR = CalculateDrainCap(((gTechR.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOSR * gTechR.featureSize, NMOS, gCell.widthInFeatureSize * gTech.featureSize, gTechR);
		capWordline += MAX(CalculateGateCap(((gTechR.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOSR * gTechR.featureSize, gTechR), CalculateGateCap(((gTechW.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTechW.featureSize, gTechW)) * numColumn;
		capBitline  += MAX(capCellAccessW, capCellAccessR) * numRow;
		voltagePrecharge = gTech.vdd;	/* DRAM read voltage is always half of vdd */
	} else if (gCell.memCellType == FBRAM){ /* Floating Body RAM */
		resCellAccess = 0;
		capCellAccess = CalculateFBRAMDrainCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthSOIDevice * gTech.featureSize, gTech);
		capWordline += CalculateFBRAMGateCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthSOIDevice * gTech.featureSize, gCell.gateOxThicknessFactor, gTech) * numColumn;
		capBitline  += capCellAccess * numRow / 2;	/* Due to shared contact */
		resMemCellOff = gCell.resistanceOff;
		resMemCellOn = gCell.resistanceOn;
		if (gCell.readMode) {						/* voltage-sensing */
			if (gCell.readVoltage == 0) {  /* Current-in voltage sensing */
				voltageMemCellOff = gCell.readCurrent * resMemCellOff;
				voltageMemCellOn = gCell.readCurrent * resMemCellOn;
				voltagePrecharge = (voltageMemCellOff + voltageMemCellOn) / 2;
				voltagePrecharge = MIN(gTech.vdd, voltagePrecharge);  /* TO-DO: we can have charge bump to increase SA working point */
				if ((voltagePrecharge - voltageMemCellOn) <= senseVoltage) {
					std::cout <<"Error[Subarray]: Read current too large or too small that no reasonable precharge voltage existing" << std::endl;
					invalid = true;
					return;
				}
			} else {   /*Voltage-divider sensing */
				resInSerialForSenseAmp = sqrt(resMemCellOn * resMemCellOff);
				resEquivalentOn = resMemCellOn * resInSerialForSenseAmp / (resMemCellOn + resInSerialForSenseAmp);
				resEquivalentOff = resMemCellOff * resInSerialForSenseAmp / (resMemCellOff + resInSerialForSenseAmp);
				voltageMemCellOff = gCell.readVoltage * resMemCellOff / (resMemCellOff + resInSerialForSenseAmp);
				voltageMemCellOn = gCell.readVoltage * resMemCellOn / (resMemCellOn + resInSerialForSenseAmp);
				voltagePrecharge = (voltageMemCellOff + voltageMemCellOn) / 2;
				voltagePrecharge = MIN(gTech.vdd, voltagePrecharge);  /* TO-DO: we can have charge bump to increase SA working point */
				if ((voltagePrecharge - voltageMemCellOn) <= senseVoltage) {
					std::cout <<"Error[Subarray]: Read Voltage too large or too small that no reasonable precharge voltage existing" << std::endl;
					invalid = true;
					return;
				}
			}
		}
	} else if (gCell.memCellType == MRAM || gCell.memCellType == PCRAM || gCell.memCellType == memristor || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
		/* MRAM, PCRAM, and memristor have three types of access devices: CMOS, BJT, and diode */
		if (gCell.accessType == CMOS_access) {
			resCellAccess = CalculateOnResistance(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, NMOS, gInputParameter.temperature, gTech);
			capCellAccess = CalculateDrainCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, NMOS, gCell.widthInFeatureSize * gTech.featureSize, gTech);
                        if (gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET){			
                            capWordline += CalculateFeFETGateCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, gTech) * numColumn;
                        } else {
			    capWordline += CalculateGateCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, gTech) * numColumn;
                        }
			capBitline  += capCellAccess * numRow / 2;	/* Due to shared contact */
		} else if (gCell.accessType == BJT_access) {
			// TO-DO
	/*	} else if (gCell.accessType == diode_access){
			if (gCell.readVoltage == 0) {
				resCellAccess = gCell.voltageDropAccessDevice / gCell.readCurrent;
			} else {
				if (gCell.readMode == false) {
					resCellAccess = gCell.voltageDropAccessDevice / (gCell.readVoltage
							- gCell.voltageDropAccessDevice) * gCell.resistanceOn;
				} else {
					std::cout<<"Error[Subarray]: Diode access do not support voltage-input voltage sensing" <<endl;
					exit(-1);
				}
			}
			capCellAccess = MAX(gCell.capacitanceOn, gCell.capacitanceOff);
			capWordline += MAX(gCell.capacitanceOff, gCell.capacitanceOn) * numColumn;
			capBitline += MAX(gCell.capacitanceOff, gCell.capacitanceOn) * numRow;      */
		} else { // none_access || diode_access
			resCellAccess = 0;
			capCellAccess = MAX(gCell.capacitanceOn, gCell.capacitanceOff);
			capWordline += MAX(gCell.capacitanceOff, gCell.capacitanceOn) * numColumn;  //TO-DO: choose the right capacitance
			capBitline += MAX(gCell.capacitanceOff, gCell.capacitanceOn) * numRow;      //TO-DO: choose the right capacitance
		}
		resMemCellOff = resCellAccess + gCell.resistanceOff;
		resMemCellOn = resCellAccess + gCell.resistanceOn;
		if (gCell.readMode) {						/* voltage-sensing */
			if (gCell.readVoltage == 0) {  /* Current-in voltage sensing */
				voltageMemCellOff = gCell.readCurrent * resMemCellOff;
				voltageMemCellOn = gCell.readCurrent * resMemCellOn;
				voltagePrecharge = (voltageMemCellOff + voltageMemCellOn) / 2;
				voltagePrecharge = MIN(gTech.vdd, voltagePrecharge);  /* TO-DO: we can have charge bump to increase SA working point */
				if ((voltagePrecharge - voltageMemCellOn) <= senseVoltage) {
					std::cout <<"Error[Subarray]: Read current too large or too small that no reasonable precharge voltage existing" << std::endl;
					invalid = true;
					return;
				}
			} else {   /*Voltage-in voltage sensing */
				resInSerialForSenseAmp = sqrt(resMemCellOn * resMemCellOff);
				resEquivalentOn = resMemCellOn * resInSerialForSenseAmp / (resMemCellOn + resInSerialForSenseAmp);
				resEquivalentOff = resMemCellOff * resInSerialForSenseAmp / (resMemCellOff + resInSerialForSenseAmp);
				voltageMemCellOff = gCell.readVoltage * resMemCellOff / (resMemCellOff + resInSerialForSenseAmp);
				voltageMemCellOn = gCell.readVoltage * resMemCellOn / (resMemCellOn + resInSerialForSenseAmp);
				voltagePrecharge = (voltageMemCellOff + voltageMemCellOn) / 2;
				voltagePrecharge = MIN(gTech.vdd, voltagePrecharge);  /* TO-DO: we can have charge bump to increase SA working point */
				if ((voltagePrecharge - voltageMemCellOn) <= senseVoltage) {
					std::cout <<"Error[Subarray]: Read Voltage too large or too small that no reasonable precharge voltage existing" << std::endl;
					invalid = true;
					return;
				}
			}
		}
	} else if (gCell.memCellType == SLCNAND) {
		/* Calculate the NAND flash string length, which is the page count per block plus 2 (two select transistors) */
		int pageCount = gInputParameter.flashBlockSize / gInputParameter.pageSize;
		int stringLength = pageCount + 2;
		resCellAccess = CalculateOnResistance(((gTech.featureSize <= 14*1e-9)? 2:1)*gTech.featureSize, NMOS, gInputParameter.temperature, gTech) * stringLength;
		capCellAccess = CalculateDrainCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gTech.featureSize, NMOS, gCell.widthInFeatureSize * gTech.featureSize, gTech);
		/* The capacitance of each gCell at the gate terminal is the series of C_control_gate | C_floating_gate */
		capWordline += CalculateGateCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gTech.featureSize, gTech) * numColumn * gCell.gateCouplingRatio / (gCell.gateCouplingRatio + 1);
		capBitline  += capCellAccess * (numRow / pageCount) / 2;	/* 2 is due to shared contact and the effective row count is numRow/pageCount */
		voltagePrecharge = gTech.vdd * 0.6;	/* SLC NAND flash bitline precharge voltage is assumed to 0.6Vdd */
        } else if (gCell.memCellType == CTT || gCell.memCellType == MLCCTT) {
		resCellAccess = CalculateOnResistance(((gTech.featureSize <= 14*1e-9)? 2:1)*gTech.featureSize, NMOS, gInputParameter.temperature, gTech);
		capCellAccess = CalculateDrainCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gTech.featureSize, NMOS, gCell.widthInFeatureSize * gTech.featureSize, gTech);		
                capWordline += CalculateGateCap(((gTech.featureSize <= 14*1e-9)? 2:1)*gTech.featureSize, gTech) * numColumn;
		capBitline  += capCellAccess * numRow;	/* 2 is due to shared contact and the effective row count is numRow/pageCount */
		voltagePrecharge = gTech.vdd * 0.5;	/* SLC NAND flash bitline precharge voltage is assumed to 0.6Vdd */ 

        } else {	/* MLC NAND flash */
		// TO-DO
	}

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
    bool mlc = false;
	if (internalSenseAmp) {
        if (gCell.memCellType == MLCCTT || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
            mlc = true;
        }
		if (!invalid) {
			senseAmp.Initialize(numSenseAmp, !voltageSense, senseVoltage, lenWordline / numColumn * muxSenseAmp, mlc, gCell.nLvl, gCell.nFingers);
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
		addWidth = MAX(addWidth, bitlineMuxDecoder.width);
		senseAmpMuxLev1Decoder.CalculateArea();
		addWidth = MAX(addWidth, senseAmpMuxLev1Decoder.width);
		senseAmpMuxLev2Decoder.CalculateArea();
		addWidth = MAX(addWidth, senseAmpMuxLev2Decoder.width);

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
			width = MAX(width, widthPeripherals);
			height = MAX(height, heightPeripherals);
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
		columnDecoderLatency = MAX(MAX(bitlineMuxDecoder.readLatency, senseAmpMuxLev1Decoder.readLatency), senseAmpMuxLev2Decoder.readLatency);
		double decoderLatency = MAX(rowDecoder.readLatency, columnDecoderLatency);
		/*need a second thought on this equation*/
		double capPassTransistor = bitlineMux.capNMOSPassTransistor +
				senseAmpMuxLev1.capNMOSPassTransistor + senseAmpMuxLev2.capNMOSPassTransistor;
		double resPassTransistor = bitlineMux.resNMOSPassTransistor +
				senseAmpMuxLev1.resNMOSPassTransistor + senseAmpMuxLev2.resNMOSPassTransistor;
		double tauChargeLatency = resPassTransistor * (capPassTransistor + capBitline) + resBitline * capBitline / 2;
		chargeLatency = horowitz(tauChargeLatency, 0, 1e20, nullptr);

		if (gCell.memCellType == SRAM) {
			/* Codes below calculate the bitline latency */
			double resPullDown = CalculateOnResistance(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthSRAMCellNMOS * gTech.featureSize, NMOS,
					gInputParameter.temperature, gTech);
			double tau = (resCellAccess + resPullDown) * (capCellAccess + capBitline + bitlineMux.capForPreviousDelayCalculation)
					+ resBitline * (bitlineMux.capForPreviousDelayCalculation + capBitline / 2);
			tau *= log(voltagePrecharge / (voltagePrecharge - senseVoltage / 2));	/* one signal raises and the other drops, so senseVoltage/2 is enough */
			double gm = CalculateTransconductance(((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, NMOS, gTech);
			double beta = 1 / (resPullDown * gm);
			double bitlineRamp = 0;
			bitlineDelay = horowitz(tau, beta, rowDecoder.rampOutput, &bitlineRamp);
			bitlineMux.CalculateLatency(bitlineRamp);
			if (internalSenseAmp) {
				senseAmp.CalculateLatency();
				senseAmpMuxLev1.CalculateLatency(1e20);
				senseAmpMuxLev2.CalculateLatency(senseAmpMuxLev1.rampOutput);
			} else {
				senseAmpMuxLev1.CalculateLatency(bitlineMux.rampOutput);
				senseAmpMuxLev2.CalculateLatency(senseAmpMuxLev1.rampOutput);
			}
			readLatency = decoderLatency + bitlineDelay + bitlineMux.readLatency + senseAmp.readLatency
					+ senseAmpMuxLev1.readLatency + senseAmpMuxLev2.readLatency;
			/* assume symmetric read/write for SRAM bitline delay */
			writeLatency = readLatency;
		} else if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
			double cap = (capCellAccess + gCell.capDRAMCell) * (capBitline + bitlineMux.capForPreviousDelayCalculation)
					/ (capCellAccess + gCell.capDRAMCell + capBitline + bitlineMux.capForPreviousDelayCalculation);
			double res = resBitline + resCellAccess;
			double tau = 2.3 * res * cap;
			double bitlineRamp = 0;
			bitlineDelay = horowitz(tau, 0, rowDecoder.rampOutput, &bitlineRamp);
			senseAmp.CalculateLatency();
			senseAmpMuxLev1.CalculateLatency(1e20);
			senseAmpMuxLev2.CalculateLatency(senseAmpMuxLev1.rampOutput);

            /* Refresh operation does not pass sense amplifier. */
            refreshLatency = decoderLatency + bitlineDelay + senseAmp.readLatency;
            refreshLatency *= numRow; // TOTAL refresh latency for subarray
			readLatency = decoderLatency + bitlineDelay + senseAmp.readLatency
					+ senseAmpMuxLev1.readLatency + senseAmpMuxLev2.readLatency;
			/* assume symmetric read/write for DRAM/eDRAM bitline delay */
			writeLatency = readLatency;
		} else if (gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
			// Write path
			double totalCapW = capCellAccessW + gCell.capDRAMCell + capBitline + bitlineMux.capForPreviousDelayCalculation;
			double tauW = 2.3*(resCellAccessW * totalCapW
						+ resBitline * (bitlineMux.capForPreviousDelayCalculation + capBitline / 2.0));
			// Read path
			double totalCapR = capCellAccessR + gCell.capDRAMCell + capBitline + bitlineMux.capForPreviousDelayCalculation;
			double tauR = 2.3*(resCellAccessR * totalCapR
						+ resBitline * (bitlineMux.capForPreviousDelayCalculation + capBitline / 2.0));
			double bitlineRamp = 0;
			bitlineDelayW = horowitz(tauW, 0, rowDecoder.rampOutput, &bitlineRamp);			
			bitlineDelayR = horowitz(tauR, 0, rowDecoder.rampOutput, &bitlineRamp);
			senseAmp.CalculateLatency();
			senseAmpMuxLev1.CalculateLatency(1e20);
			senseAmpMuxLev2.CalculateLatency(senseAmpMuxLev1.rampOutput);

            /* Refresh operation does not pass sense amplifier. */
            refreshLatency = decoderLatency + bitlineDelayW + senseAmp.readLatency;
            refreshLatency *= numRow; // TOTAL refresh latency for subarray
			readLatency = decoderLatency + bitlineDelayR + senseAmp.readLatency
					+ senseAmpMuxLev1.readLatency + senseAmpMuxLev2.readLatency;
			writeLatency = decoderLatency + bitlineDelayW;
		} else if (gCell.memCellType == MRAM || gCell.memCellType == PCRAM || gCell.memCellType == memristor || gCell.memCellType == FBRAM || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
			double bitlineRamp = 0;
			if (gCell.readMode == false) {	/* current-sensing */
				/* Use ICCAD 2009 model */
				double tau = resBitline * capBitline / 2 * (resMemCellOff + resBitline / 3) / (resMemCellOff + resBitline);
				bitlineDelay = horowitz(tau, 0, rowDecoder.rampOutput, &bitlineRamp);
			} else {						/* voltage-sensing */
				if (gCell.readVoltage == 0) {  /* Current-in voltage sensing */
					double tau = resMemCellOn * (capCellAccess + capBitline + bitlineMux.capForPreviousDelayCalculation)
							+ resBitline * (bitlineMux.capForPreviousDelayCalculation + capBitline / 2); /* time constant of LRS */
					bitlineDelayOn = tau * log((voltagePrecharge - voltageMemCellOn)/(voltagePrecharge - voltageMemCellOn - senseVoltage));  /* BitlineDelay of HRS */
					tau = resMemCellOff * (capCellAccess + capBitline + bitlineMux.capForPreviousDelayCalculation)
							+ resBitline * (bitlineMux.capForPreviousDelayCalculation + capBitline / 2);  /* time constant of HRS */
					bitlineDelayOff = tau * log((voltageMemCellOff - voltagePrecharge)/(voltageMemCellOff - voltagePrecharge - senseVoltage));
					bitlineDelay = MAX(bitlineDelayOn, bitlineDelayOff);
				} else {   /*Voltage-in voltage sensing */
					double tau = resEquivalentOn * (capCellAccess + capBitline + bitlineMux.capForPreviousDelayCalculation)
							+ resBitline * (bitlineMux.capForPreviousDelayCalculation + capBitline / 2); /* time constant of LRS */
					bitlineDelayOn = tau * log((voltagePrecharge - voltageMemCellOn)/(voltagePrecharge - voltageMemCellOn - senseVoltage));  /* BitlineDelay of HRS */

					tau = resEquivalentOff * (capCellAccess + capBitline + bitlineMux.capForPreviousDelayCalculation)
							+ resBitline * (bitlineMux.capForPreviousDelayCalculation + capBitline / 2);  /* time constant of HRS */
					bitlineDelayOff = tau * log((voltageMemCellOff - voltagePrecharge)/(voltageMemCellOff - voltagePrecharge - senseVoltage));
					bitlineDelay = MAX(bitlineDelayOn, bitlineDelayOff);
				}
			}
			bitlineMux.CalculateLatency(bitlineRamp);
			if (internalSenseAmp) {
				senseAmp.CalculateLatency();
				senseAmpMuxLev1.CalculateLatency(1e20);
				senseAmpMuxLev2.CalculateLatency(senseAmpMuxLev1.rampOutput);
			} else {
				senseAmpMuxLev1.CalculateLatency(bitlineMux.rampOutput);
				senseAmpMuxLev2.CalculateLatency(senseAmpMuxLev1.rampOutput);
			}
			readLatency = decoderLatency + bitlineDelay + bitlineMux.readLatency + senseAmp.readLatency
					+ senseAmpMuxLev1.readLatency + senseAmpMuxLev2.readLatency;

			if (gCell.memCellType == PCRAM) {
				if (gInputParameter.writeScheme == write_and_verify) {
					/*TO-DO: write and verify programming */
				} else {
					writeLatency = MAX(rowDecoder.writeLatency, columnDecoderLatency + chargeLatency);	/* TO-DO: why not directly use precharger latency? */
					resetLatency = writeLatency + gCell.resetPulse;
					setLatency = writeLatency + gCell.setPulse;
					writeLatency += MAX(gCell.resetPulse, gCell.setPulse);
				}
			} else if (gCell.memCellType == FBRAM) {
				writeLatency = MAX(rowDecoder.writeLatency, columnDecoderLatency + chargeLatency);
				resetLatency = writeLatency + gCell.resetPulse;
				setLatency = writeLatency + gCell.setPulse;
				writeLatency += MAX(gCell.resetPulse, gCell.setPulse);
			} else { //memristor and MRAM and FeFET
				if (gCell.accessType == diode_access || gCell.accessType == none_access) {
					if (gInputParameter.writeScheme == erase_before_reset || gInputParameter.writeScheme == erase_before_set)
						writeLatency = MAX(rowDecoder.writeLatency, chargeLatency);
					else
						writeLatency = MAX(rowDecoder.writeLatency, columnDecoderLatency + chargeLatency);
					writeLatency += chargeLatency;
					writeLatency += gCell.resetPulse + gCell.setPulse;
				} else { // CMOS or Bipolar access
					writeLatency = MAX(rowDecoder.writeLatency, columnDecoderLatency + chargeLatency);
					resetLatency = writeLatency + gCell.resetPulse;
					setLatency = writeLatency + gCell.setPulse;
					writeLatency += MAX(gCell.resetPulse, gCell.setPulse);
				}
			}
		} else if (gCell.memCellType == SLCNAND) {
			/* Calculate the NAND flash string length, which is the page count per block plus 2 (two select transistors) */
			int pageCount = gInputParameter.flashBlockSize / gInputParameter.pageSize;
			int stringLength = pageCount + 2;
			/* Codes below calculate the bitline latency */
			double resPullDown = CalculateOnResistance(((gTech.featureSize <= 14*1e-9)? 2:1)*gTech.featureSize, NMOS, gInputParameter.temperature, gTech)
					* stringLength;
			double tau = resPullDown * (capCellAccess + capBitline + bitlineMux.capForPreviousDelayCalculation)
					+ resBitline * (bitlineMux.capForPreviousDelayCalculation + capBitline / 2);
			/* in one case the bitline is unchanged, and in the other case the bitline drops from 0.6V to 0.4V */
			tau *= log((voltagePrecharge)/ (voltagePrecharge - senseVoltage));
			double gm = CalculateTransconductance(((gTech.featureSize <= 14*1e-9)? 2:1)*gTech.featureSize, NMOS, gTech);	/* minimum size transistor */
			double beta = 1 / (resPullDown * gm);
			double bitlineRamp = 0;
			bitlineDelay = horowitz(tau, beta, rowDecoder.rampOutput, &bitlineRamp);
			/* to correct unnecessary horowitz calculation, TO-DO: need to revisit */
			bitlineDelay = MAX(bitlineDelay, tau * 20);
			bitlineMux.CalculateLatency(bitlineRamp);
			if (internalSenseAmp) {
				senseAmp.CalculateLatency();
				senseAmpMuxLev1.CalculateLatency(1e20);
				senseAmpMuxLev2.CalculateLatency(senseAmpMuxLev1.rampOutput);
			} else {
				senseAmpMuxLev1.CalculateLatency(bitlineMux.rampOutput);
				senseAmpMuxLev2.CalculateLatency(senseAmpMuxLev1.rampOutput);
			}
			readLatency = decoderLatency + bitlineDelay + bitlineMux.readLatency + senseAmp.readLatency
					+ senseAmpMuxLev1.readLatency + senseAmpMuxLev2.readLatency;
			/* calculate the erase time, a.k.a. reset here */
			resetLatency = MAX(rowDecoder.readLatency, columnDecoderLatency + chargeLatency) + gCell.flashEraseTime;
			/* calculate the programming time, a.k.a. set here */
			setLatency = MAX(rowDecoder.readLatency, columnDecoderLatency + chargeLatency) + gCell.flashProgramTime;
			/* use the programming latency as the write latency for SLC NAND*/
			writeLatency = setLatency;

                } else if (gCell.memCellType == CTT || gCell.memCellType == MLCCTT) {
                        /* Calculate the NAND flash string length, which is the page count per block plus 2 (two select transistors) */
			double resPullDown = CalculateOnResistance(((gTech.featureSize <= 14*1e-9)? 2:1)*gTech.featureSize, NMOS, gInputParameter.temperature, gTech) * 2;
			double tau = resPullDown * (capCellAccess + capBitline + bitlineMux.capForPreviousDelayCalculation)
					+ resBitline * (bitlineMux.capForPreviousDelayCalculation + capBitline / 2);
			/* in one case the bitline is unchanged, and in the other case the bitline drops from 0.6V to 0.4V */
			tau *= log((voltagePrecharge)/ (voltagePrecharge - senseVoltage));
			double gm = CalculateTransconductance(((gTech.featureSize <= 14*1e-9)? 2:1)*gTech.featureSize, NMOS, gTech);	/* minimum size transistor */
			double beta = 1 / (resPullDown * gm);
			double bitlineRamp = 0;
			bitlineDelay = horowitz(tau, beta, rowDecoder.rampOutput, &bitlineRamp);
			/* to correct unnecessary horowitz calculation, TO-DO: need to revisit */
			bitlineDelay = MAX(bitlineDelay, tau * 20);
			bitlineMux.CalculateLatency(bitlineRamp);
			if (internalSenseAmp) {
				senseAmp.CalculateLatency();
				senseAmpMuxLev1.CalculateLatency(1e20);
				senseAmpMuxLev2.CalculateLatency(senseAmpMuxLev1.rampOutput);
			} else {
				senseAmpMuxLev1.CalculateLatency(bitlineMux.rampOutput);
				senseAmpMuxLev2.CalculateLatency(senseAmpMuxLev1.rampOutput);
			}
			readLatency = decoderLatency + bitlineDelay + bitlineMux.readLatency + senseAmp.readLatency
					+ senseAmpMuxLev1.readLatency + senseAmpMuxLev2.readLatency;
			/* calculate the erase time, a.k.a. reset here */
			resetLatency = MAX(rowDecoder.readLatency, columnDecoderLatency + chargeLatency);
			/* calculate the programming time, a.k.a. set here */
			setLatency = MAX(rowDecoder.readLatency, columnDecoderLatency + chargeLatency);
			/* use the programming latency as the write latency for SLC NAND*/
			writeLatency = setLatency;

        } else {	/* MLC NAND */
			/* TO-DO */
		}
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

		if (gCell.memCellType == SRAM) {
			/* Codes below calculate the SRAM bitline power */
			readDynamicEnergy = (capCellAccess + capBitline + bitlineMux.capForPreviousPowerCalculation)
					* voltagePrecharge * voltagePrecharge * numColumn;
			writeDynamicEnergy = (capCellAccess + capBitline + bitlineMux.capForPreviousPowerCalculation)
					* voltagePrecharge * voltagePrecharge * numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
			leakage = CalculateGateLeakage(INV, 1, ((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthSRAMCellNMOS * gTech.featureSize,
					((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthSRAMCellPMOS * gTech.featureSize, gInputParameter.temperature, gTech)
					* gTech.vdd * 2;	/* two inverters per SRAM gCell */
			leakage += CalculateGateLeakage(INV, 1, ((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, 0,
					gInputParameter.temperature, gTech) * gTech.vdd;	/* two accesses NMOS, but combined as one with vdd crossed */
			leakage *= numRow * numColumn;
		} else if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
			/* Codes below calculate the DRAM bitline power */
			readDynamicEnergy = (capCellAccess + capBitline + bitlineMux.capForPreviousPowerCalculation) * senseVoltage * gTech.vdd * numColumn;
			double writeVoltage = gTech.vpp;	/* should also equal to setVoltage, for DRAM, it is Vdd */
			writeDynamicEnergy = (capBitline + bitlineMux.capForPreviousPowerCalculation) * writeVoltage * writeVoltage * numColumn;
            refreshDynamicEnergy = readDynamicEnergy + writeDynamicEnergy;
			leakage += CalculateGateLeakage(INV, 1, ((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, 0,
					gInputParameter.temperature, gTech) * gTech.vdd;
			leakage *= numColumn;
		} else if (gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333) {
			/* Codes below calculate the DRAM bitline power */
			readDynamicEnergy = (capCellAccessR + capBitline + bitlineMux.capForPreviousPowerCalculation) * senseVoltage * gTechR.vdd * numColumn;
			double writeVoltage = gTechW.vpp;	/* should also equal to setVoltage, for DRAM, it is Vdd */
			writeDynamicEnergy = (capBitline + bitlineMux.capForPreviousPowerCalculation) * writeVoltage * writeVoltage * numColumn;
            refreshDynamicEnergy = readDynamicEnergy + writeDynamicEnergy;
			leakage = CalculateGateLeakage(INV, 1, ((gTech.featureSize <= 14*1e-9)? 2:1)*gCell.widthAccessCMOS * gTech.featureSize, 0,
					gInputParameter.temperature, gTech) * gTech.vdd;
			leakage *= numColumn;
		} else if (gCell.memCellType == MRAM || gCell.memCellType == PCRAM || gCell.memCellType == memristor || gCell.memCellType == FBRAM || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM) {
			if (gCell.readMode == false) {	/* current-sensing */
				/* Use ICCAD 2009 model */
				double resBitlineMux = bitlineMux.resNMOSPassTransistor;
				double vpreMin = gCell.readVoltage * resBitlineMux / (resBitlineMux + resBitline +resMemCellOn);
				double vpreMax = gCell.readVoltage * (resBitlineMux + resBitline) / (resBitlineMux + resBitline + resMemCellOn);
				readDynamicEnergy = capCellAccess * vpreMax * vpreMax + bitlineMux.capForPreviousPowerCalculation
						* vpreMin * vpreMin + capBitline * (vpreMax * vpreMax + vpreMin * vpreMin + vpreMax * vpreMin) / 3;
				readDynamicEnergy *= numColumn;
			} else {						/* voltage-sensing */
				readDynamicEnergy = (capCellAccess + capBitline + bitlineMux.capForPreviousPowerCalculation) *
						(voltagePrecharge * voltagePrecharge - voltageMemCellOn * voltageMemCellOn ) * numColumn;
			}

			if (gCell.readPower == 0) 
				cellReadEnergy = 2 * gCell.CalculateReadPower() * senseAmp.readLatency; /* x2 is because of the reference cell */
			else
				cellReadEnergy = 2 * gCell.readPower * senseAmp.readLatency;
			cellReadEnergy *= numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;

			/* Ignore the dynamic transition during the SET/RESET operation */
			/* Assume that the cell resistance keeps high for worst-case power estimation */
			gCell.CalculateWriteEnergy();

			double resetEnergyPerBit = gCell.resetEnergy;
			double setEnergyPerBit = gCell.setEnergy;
			if (gCell.setMode)
				setEnergyPerBit += (capCellAccess + capBitline + bitlineMux.capForPreviousPowerCalculation) * gCell.setVoltage * gCell.setVoltage;
			else
				setEnergyPerBit += (capCellAccess + capBitline + bitlineMux.capForPreviousPowerCalculation) * gTech.vdd * gTech.vdd;
			if (gCell.resetMode)
				resetEnergyPerBit += (capCellAccess + capBitline + bitlineMux.capForPreviousPowerCalculation) * gCell.resetVoltage * gCell.resetVoltage;
			else
				resetEnergyPerBit += (capCellAccess + capBitline + bitlineMux.capForPreviousPowerCalculation) * gTech.vdd * gTech.vdd;

			if (gCell.memCellType == PCRAM) { //PCRAM write energy
				if (gInputParameter.writeScheme == write_and_verify) {
					/*TO-DO: write and verify programming */
				} else {
					cellResetEnergy = resetEnergyPerBit * numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
					cellSetEnergy = setEnergyPerBit * numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
					cellResetEnergy /= SHAPER_EFFICIENCY_CONSERVATIVE;
					cellSetEnergy /= SHAPER_EFFICIENCY_CONSERVATIVE;  /* Due to the shaper inefficiency */
					writeDynamicEnergy = MAX(cellResetEnergy, cellSetEnergy);
				}
			} else if (gCell.memCellType == FBRAM){ //FBRAM write energy
				cellResetEnergy = resetEnergyPerBit * numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
				cellSetEnergy = setEnergyPerBit * numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
				cellResetEnergy /= SHAPER_EFFICIENCY_AGGRESSIVE;
				cellSetEnergy /= SHAPER_EFFICIENCY_AGGRESSIVE;  /* Due to the shaper inefficiency */
				writeDynamicEnergy = MAX(cellResetEnergy, cellSetEnergy);
			} else { //MRAM and memristor write energy
				if (gCell.accessType == diode_access || gCell.accessType == none_access) {
					if (gInputParameter.writeScheme == erase_before_reset || gInputParameter.writeScheme == erase_before_set) {
						cellResetEnergy = resetEnergyPerBit * numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
						cellSetEnergy = setEnergyPerBit * numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
						writeDynamicEnergy = cellResetEnergy + cellSetEnergy;	/* TO-DO: bug here, did you consider the write pattern? */
					} else { /* write scheme = set_before_reset or reset_before_set */
						cellResetEnergy = resetEnergyPerBit * numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
						cellSetEnergy = setEnergyPerBit * numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
						writeDynamicEnergy = MAX(cellResetEnergy, cellSetEnergy);
					}
				} else {
					cellResetEnergy = resetEnergyPerBit * numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
					cellSetEnergy = setEnergyPerBit * numColumn / muxSenseAmp / muxOutputLev1 / muxOutputLev2;
					writeDynamicEnergy = MAX(cellResetEnergy, cellSetEnergy);
				}
				cellResetEnergy /= SHAPER_EFFICIENCY_AGGRESSIVE;
				cellSetEnergy /= SHAPER_EFFICIENCY_AGGRESSIVE;  /* Due to the shaper inefficiency */
				writeDynamicEnergy /= SHAPER_EFFICIENCY_AGGRESSIVE;
			}
			leakage = 0;                       //TO-DO: cell leaks during read/write operation
		} else if (gCell.memCellType == SLCNAND) {
			/* Calculate the NAND flash string length, which is the page count per block plus 2 (two select transistors) */
			int pageCount = gInputParameter.flashBlockSize / gInputParameter.pageSize;
			int stringLength = pageCount + 2;

			/* === READ energy === */
			/* only the selected bitline is charged during the read operation, bitline is charged to Vpre */
			readDynamicEnergy = (capCellAccess + capBitline + bitlineMux.capForPreviousPowerCalculation)
					* voltagePrecharge * voltagePrecharge * numColumn;
			/* tricky thing here!
			 * In SLC NAND operation, SSL, GSL, and unselected wordlines in a block are charged to Vpass,
			 * but the selected wordline is not charged, which is totally different from the other cases.
			 */
			rowDecoder.resetDynamicEnergy = rowDecoder.readDynamicEnergy;
			rowDecoder.setDynamicEnergy = rowDecoder.readDynamicEnergy;
			double actualWordlineReadEnergy = rowDecoder.readDynamicEnergy / gTech.vdd / gTech.vdd
					* gCell.flashPassVoltage * gCell.flashPassVoltage;	/* approximate calculate, the wordline is charged to Vpass instead of Vdd */
			actualWordlineReadEnergy = actualWordlineReadEnergy * (numRow / pageCount * stringLength - 1);	/* except the selected wordline itself */
			rowDecoder.readDynamicEnergy = actualWordlineReadEnergy;	/* update the correct value */

			/* === Programming (SET) energy === */
			/* first calculate the source line energy (charged to Vdd), which is a part of "bitline" in this scenario */
			setDynamicEnergy = (capCellAccess + capBitline + bitlineMux.capForPreviousPowerCalculation)
					* gCell.flashProgramVoltage * gCell.flashProgramVoltage * numColumn;
			/* add tunneling current */
			/* originally it should be multiplied by numColumn/muxSenseAmp/muxOutputLev1/muxOutputLev2,
			 * but it is multiplied by numColumn here because all the unselected bitlines also need to precharge to Vdd
			 */
			setDynamicEnergy += DELTA_V_TH * TUNNEL_CURRENT_FLOW * gCell.area
					* gTech.featureSize * gTech.featureSize * gCell.flashProgramTime * numColumn;
			/* in programming, the SSL is precharged to Vdd, which is equal to the original value calculated
			 * from row decoder
			 */
			double actualWordlineSetEnergy = rowDecoder.setDynamicEnergy;
			/* however, the unselected wordlines in the same block have to precharge to Vpass */
			actualWordlineSetEnergy += rowDecoder.setDynamicEnergy / gTech.vdd / gTech.vdd
					* gCell.flashPassVoltage * gCell.flashPassVoltage * (numRow / pageCount * stringLength - 1);
			/* And the selected wordline is precharged to Vpgm */
			actualWordlineSetEnergy += rowDecoder.setDynamicEnergy / gTech.vdd / gTech.vdd
					* gCell.flashProgramVoltage * gCell.flashProgramVoltage;
			rowDecoder.setDynamicEnergy = actualWordlineSetEnergy;	/* update the correct value */

			/* === Erase (RESET) energy === */
			/* in erase, all the bitlines (selected or unselected) and the sourceline are precharged to Vera-Vbi */

			resetDynamicEnergy = (capCellAccess + capBitline + bitlineMux.capForPreviousPowerCalculation)
					* (gCell.flashEraseVoltage - gTech.buildInPotential) * (gCell.flashEraseVoltage - gTech.buildInPotential);
			resetDynamicEnergy *= (numColumn + 1);	/* plus 1 is due to the source line */
			/* the p-well shared by the selected block is precharged to Vera */
			double wellJunctionCap = gTech.capJunction * gCell.area * gTech.featureSize * gTech.featureSize;
			wellJunctionCap *= gInputParameter.flashBlockSize;	/* one block shares the same well */
			resetDynamicEnergy += wellJunctionCap * gCell.flashEraseVoltage * gCell.flashEraseVoltage;
			/* in erase, all the wordlines, SSL, and GSL in unselected block are precharged to Vera * beta
			 * in selected block, SSL and GSL are precharged to Vera * beta
			 * here beta is fixed at 0.8
			 */
			double beta = 0.8;
			double actualWordlineResetEnergy = rowDecoder.resetDynamicEnergy / gTech.vdd / gTech.vdd
					* (gCell.flashEraseVoltage * beta) * (gCell.flashEraseVoltage * beta);
			actualWordlineResetEnergy *= (numRow / pageCount * stringLength - pageCount);
			rowDecoder.resetDynamicEnergy = actualWordlineResetEnergy;

			/* let write energy to be the average energy per page*/
			rowDecoder.writeDynamicEnergy = (rowDecoder.setDynamicEnergy + rowDecoder.resetDynamicEnergy / pageCount) / 2;
			writeDynamicEnergy = (setDynamicEnergy + resetDynamicEnergy / pageCount) / 2;

			/* Assume NAND flash cell does not consume any leakage */
			leakage = 0;
		} else {	/* MLC NAND */
			/* TO-DO */
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
	std::cout << "bitlineDelay: " << bitlineDelay*1e12 << "ps" << std::endl;
	std::cout << "chargeLatency: " << chargeLatency*1e12 << "ps" << std::endl;
	std::cout << "columnDecoderLatency: " << columnDecoderLatency*1e12 << "ps" << std::endl;
}
