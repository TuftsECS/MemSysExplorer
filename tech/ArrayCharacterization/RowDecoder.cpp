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


#include "RowDecoder.h"
#include "formula.h"
#include "global.h"

void RowDecoder::Initialize(int _numRow, double _capLoad, double _resLoad,
		bool _multipleRowPerSet, BufferDesignTarget _areaOptimizationLevel, double _minDriverCurrent) {
	if (initialized)
		cout << "[Row Decoder] Warning: Already initialized!" << endl;

	numRow = _numRow;
	capLoad = _capLoad;
	resLoad = _resLoad;
	multipleRowPerSet = _multipleRowPerSet;
	areaOptimizationLevel = _areaOptimizationLevel;
	minDriverCurrent = _minDriverCurrent;

	if (numRow <= 8) {	/* The predecoder output is used directly */
		if (multipleRowPerSet)
			numNandInput = 2;	/* NAND way-select with predecoder output */
		else
			numNandInput = 0;	/* no circuit needed */
	} else {
		if (multipleRowPerSet)
			numNandInput = 3;	/* NAND way-select with two predecoder outputs */
		else
			numNandInput = 2;	/* just NAND two predecoder outputs */
	}

	if (numNandInput > 0) {
		double logicEffortNand;
		double capNand;
		if (numNandInput == 2) {	/* NAND2 */
			widthNandN = 2 * MIN_NMOS_SIZE * gTech.featureSize;
			logicEffortNand = (2+gTech.pnSizeRatio) / (1+gTech.pnSizeRatio);
		} else {					/* NAND3 */
			widthNandN = 3 * MIN_NMOS_SIZE * gTech.featureSize;
			logicEffortNand = (3+gTech.pnSizeRatio) / (1+gTech.pnSizeRatio);
		}
		widthNandP = gTech.pnSizeRatio * MIN_NMOS_SIZE * gTech.featureSize;
		capNand = CalculateGateCap(widthNandN, gTech) + CalculateGateCap(widthNandP, gTech);
		outputDriver.Initialize(logicEffortNand, capNand, capLoad, resLoad, true, areaOptimizationLevel, minDriverCurrent);
	} else {
		/* we only need an 1-level output buffer to driver the wordline */
		double capInv;
		widthNandN = MIN_NMOS_SIZE * gTech.featureSize;
		widthNandP = gTech.pnSizeRatio * MIN_NMOS_SIZE * gTech.featureSize;
		capInv = CalculateGateCap(widthNandN, gTech) + CalculateGateCap(widthNandP, gTech);
		outputDriver.Initialize(1, capInv, capLoad, resLoad, true, areaOptimizationLevel, minDriverCurrent);
	}

	if (outputDriver.invalid) {
		invalid = true;
		return;
	}

	initialized = true;
}

void RowDecoder::CalculateArea() {
	if (!initialized) {
		cout << "[Row Decoder Area] Error: Require initialization first!" << endl;
	} else {
		outputDriver.CalculateArea();
		if (numNandInput == 0) {	/* no circuit needed, use predecoder outputs directly */
			height = outputDriver.height;
			width = outputDriver.width;
		} else {
			double hNand, wNand;
			CalculateGateArea(NAND, numNandInput, widthNandN, widthNandP, gTech.featureSize*40, gTech, &hNand, &wNand);
			height = MAX(hNand, outputDriver.height);
			width = wNand + outputDriver.width;
		}
		height *= numRow;
		area = height * width;
	}
}

void RowDecoder::CalculateRC() {
	if (!initialized) {
		cout << "[Row Decoder RC] Error: Require initialization first!" << endl;
	} else {
		outputDriver.CalculateRC();
		if (numNandInput == 0) {	/* no circuit needed, use predecoder outputs directly */
			capNandInput = capNandOutput = 0;
		} else {
            CalculateGateCapacitance(NAND, numNandInput, widthNandN, widthNandP, gTech.featureSize * MAX_TRANSISTOR_HEIGHT, gTech, &capNandInput, &capNandOutput);
		}
	}
}

void RowDecoder::CalculateLatency(double _rampInput) {
	if (!initialized) {
		cout << "[Row Decoder Latency] Error: Require initialization first!" << endl;
	} else {
		if (numNandInput == 0) {	/* no circuit needed, use predecoder outputs directly */
			outputDriver.CalculateLatency(_rampInput);
			readLatency = writeLatency = outputDriver.readLatency;
			rampOutput = outputDriver.rampOutput;
		} else {
			rampInput = _rampInput;

			double resPullDown;
			double capLoad;
			double tr;	/* time constant */
			double gm;	/* transconductance */
			double beta;	/* for horowitz calculation */
			double rampInputForDriver;

			resPullDown = CalculateOnResistance(widthNandN, NMOS, gInputParameter.temperature, gTech) * numNandInput;
			capLoad = capNandOutput + outputDriver.capInput[0];
			tr = resPullDown * capLoad;
			gm = CalculateTransconductance(widthNandN, NMOS, gTech);
			beta = 1 / (resPullDown * gm);
			readLatency = horowitz(tr, beta, rampInput, &rampInputForDriver);

			outputDriver.CalculateLatency(rampInputForDriver);
			readLatency += outputDriver.readLatency;
			writeLatency = readLatency;
			rampOutput = outputDriver.rampOutput;
		}
	}
}

void RowDecoder::CalculatePower() {
	if (!initialized) {
		cout << "[Row Decoder Power] Error: Require initialization first!" << endl;
	} else {
		outputDriver.CalculatePower();
		leakage = outputDriver.leakage;
		if (numNandInput == 0) {	/* no circuit needed, use predecoder outputs directly */
			readDynamicEnergy = writeDynamicEnergy = outputDriver.readDynamicEnergy;
		} else {
			/* Leakage power */
			leakage += CalculateGateLeakage(NAND, numNandInput, widthNandN, widthNandP,
					gInputParameter.temperature, gTech) * gTech.vdd;
			/* Dynamic energy */
			double capLoad = capNandOutput + outputDriver.capInput[0];
            /* For DRAM types account for overdriven wordline. */
            if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
                readDynamicEnergy = capLoad * gTech.vpp * gTech.vpp;
            } else {
			readDynamicEnergy = capLoad * gTech.vdd * gTech.vdd;
            }
			readDynamicEnergy += outputDriver.readDynamicEnergy;
			readDynamicEnergy *= 1;	/* only one row is activated each time */
			writeDynamicEnergy = readDynamicEnergy;
		}
		leakage *= numRow;
	}
}

void RowDecoder::PrintProperty() {
	cout << "Row Decoder Properties:" << endl;
	FunctionUnit::PrintProperty();
}
