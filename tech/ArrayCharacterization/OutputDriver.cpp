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


#include "OutputDriver.h"
#include "global.h"
#include "formula.h"
#include <math.h>

void OutputDriver::Initialize(double _logicEffort, double _inputCap, double _outputCap, double _outputRes,
		bool _inv, BufferDesignTarget _areaOptimizationLevel, double _minDriverCurrent) {
	if (initialized)
		std::cout << "[Output Driver] Warning: Already initialized!" << std::endl;

	logicEffort = _logicEffort;
	inputCap = _inputCap;
	outputCap = _outputCap;
	outputRes = _outputRes;
	inv = _inv;
	areaOptimizationLevel = _areaOptimizationLevel;
	minDriverCurrent = _minDriverCurrent;

	double minNMOSDriverWidth = minDriverCurrent / gTech.currentOnNmos[gInputParameter.temperature - 300];
	minNMOSDriverWidth = MAX(MIN_NMOS_SIZE * gTech.featureSize, minNMOSDriverWidth);

	if (minNMOSDriverWidth > gInputParameter.maxNmosSize * gTech.featureSize) {
		invalid = true;
		return;
	}

	int optimalNumStage;

	if (areaOptimizationLevel == latency_first) {
		double F = MAX(1, logicEffort * outputCap / inputCap);	/* Total logic effort */
		optimalNumStage = MAX(0, (int)(log(F) / log(OPT_F) + 0.5) - 1);

		if ((optimalNumStage % 2) ^ inv)	/* If odd, add 1 */
			optimalNumStage += 1;

		if (optimalNumStage > MAX_INV_CHAIN_LEN) {/* Exceed maximum stages */
			if (WARNING)
				std::cout << "[WARNING] Exceed maximum inverter chain length!" << std::endl;
			optimalNumStage = MAX_INV_CHAIN_LEN;
		}

		numStage = optimalNumStage;

		double f = pow(F, 1.0 / (optimalNumStage + 1));	/* Logic effort per stage */
		double inputCapLast = outputCap / f;

		widthNMOS[optimalNumStage-1] = MAX(MIN_NMOS_SIZE * gTech.featureSize,
				inputCapLast / CalculateGateCap(1/*meter*/, gTech) / (1.0 + gTech.pnSizeRatio));

		if (widthNMOS[optimalNumStage-1] > gInputParameter.maxNmosSize * gTech.featureSize) {
			if (WARNING)
				std::cout << "[WARNING] Exceed maximum NMOS size!" << std::endl;
			widthNMOS[optimalNumStage-1] = gInputParameter.maxNmosSize * gTech.featureSize;
			/* re-Calculate the logic effort */
			double capLastStage = CalculateGateCap((1 + gTech.pnSizeRatio) * gInputParameter.maxNmosSize * gTech.featureSize, gTech);
			F = logicEffort * capLastStage / inputCap;
			f =	pow(F, 1.0 / (optimalNumStage));
		}

		if (widthNMOS[optimalNumStage-1] < minNMOSDriverWidth) {
			/* the last level Inv can not provide minimum current so that the Inv chain can't only decided by Logic Effort */
			areaOptimizationLevel = latency_area_trade_off;
		} else {
			widthPMOS[optimalNumStage-1] = widthNMOS[optimalNumStage-1] * gTech.pnSizeRatio;

			for (int i = optimalNumStage-2; i >= 0; i--) {
				widthNMOS[i] = widthNMOS[i+1] / f;
				if (widthNMOS[i] < MIN_NMOS_SIZE * gTech.featureSize) {
					if (WARNING)
						std::cout << "[WARNING] Exceed minimum NMOS size!" << std::endl;
					widthNMOS[i] = MIN_NMOS_SIZE * gTech.featureSize;
				}
				widthPMOS[i] = widthNMOS[i] * gTech.pnSizeRatio;
			}
		}
	}

	if (areaOptimizationLevel == latency_area_trade_off){
		double newOutputCap = CalculateGateCap(minNMOSDriverWidth, gTech) * (1.0 + gTech.pnSizeRatio);
		double F = MAX(1, logicEffort * newOutputCap / inputCap);	/* Total logic effort */
		optimalNumStage = MAX(0, (int)(log(F) / log(OPT_F) + 0.5) - 1);

		if (!((optimalNumStage % 2) ^ inv))	/* If even, add 1 */
			optimalNumStage += 1;

		if (optimalNumStage > MAX_INV_CHAIN_LEN) {/* Exceed maximum stages */
			if (WARNING)
				std::cout << "[WARNING] Exceed maximum inverter chain length!" << std::endl;
			optimalNumStage = MAX_INV_CHAIN_LEN;
		}

		numStage = optimalNumStage + 1;

		widthNMOS[optimalNumStage] = minNMOSDriverWidth;
		widthPMOS[optimalNumStage] = widthNMOS[optimalNumStage] * gTech.pnSizeRatio;

		double f = pow(F, 1.0 / (optimalNumStage + 1));	/* Logic effort per stage */

		for (int i = optimalNumStage - 1; i >= 0; i--) {
			widthNMOS[i] = widthNMOS[i+1] / f;
			if (widthNMOS[i] < MIN_NMOS_SIZE * gTech.featureSize) {
				if (WARNING)
					std::cout << "[WARNING] Exceed minimum NMOS size!" << std::endl;
				widthNMOS[i] = MIN_NMOS_SIZE * gTech.featureSize;
			}
			widthPMOS[i] = widthNMOS[i] * gTech.pnSizeRatio;
		}
	} else if (areaOptimizationLevel == area_first) {
		optimalNumStage = 1;
		numStage = 1;
		widthNMOS[optimalNumStage - 1] = MAX(MIN_NMOS_SIZE * gTech.featureSize, minNMOSDriverWidth);
		if (widthNMOS[optimalNumStage - 1] > AREA_OPT_CONSTRAIN * gInputParameter.maxNmosSize * gTech.featureSize) {
			invalid = true;
			return;
		}
		widthPMOS[optimalNumStage - 1] = widthNMOS[optimalNumStage - 1] * gTech.pnSizeRatio;
	}

	/* Restore the original buffer design style */
	areaOptimizationLevel = _areaOptimizationLevel;

	initialized = true;
}

void OutputDriver::CalculateArea() {
	if (!initialized) {
		std::cout << "[Output Driver] Error: Require initialization first!" << std::endl;
	} else if (invalid) {
		height = width = area = invalid_value;
	} else {
		double totalHeight = 0;
		double totalWidth = 0;
		double h, w;
		for (int i = 0; i < numStage; i++) {
			CalculateGateArea(INV, 1, widthNMOS[i], widthPMOS[i], gTech.featureSize*40, gTech, &h, &w);
			totalHeight = MAX(totalHeight, h);
			totalWidth += w;
		}
		height = totalHeight;
		width = totalWidth;
		area = height * width;
	}
}

void OutputDriver::CalculateRC() {
	if (!initialized) {
		std::cout << "[Output Driver] Error: Require initialization first!" << std::endl;
	} else if (invalid) {
		;  // nothing to do if invalid
	} else if (numStage == 0) {
		capInput[0] = 0;
	} else {
		for (int i = 0; i < numStage; i++) {
			CalculateGateCapacitance(INV, 1, widthNMOS[i], widthPMOS[i], gTech.featureSize * MAX_TRANSISTOR_HEIGHT, gTech, &(capInput[i]), &(capOutput[i]));
		}
	}
}

void OutputDriver::CalculateLatency(double _rampInput) {
	if (!initialized) {
		std::cout << "[Output Driver] Error: Require initialization first!" << std::endl;
	} else if (invalid) {
		readLatency = writeLatency = invalid_value;
	} else {
		rampInput = _rampInput;
		double resPullDown;
		double capLoad;
		double tr;	/* time constant */
		double gm;	/* transconductance */
		double beta;	/* for horowitz calculation */
		double temp;
		readLatency = 0;
		for (int i = 0; i < numStage - 1; i++) {
			resPullDown = CalculateOnResistance(widthNMOS[i], NMOS, gInputParameter.temperature, gTech);
			capLoad = capOutput[i] + capInput[i+1];
			tr = resPullDown * capLoad;
			gm = CalculateTransconductance(widthNMOS[i], NMOS, gTech);
			beta = 1 / (resPullDown * gm);
			readLatency += horowitz(tr, beta, rampInput, &temp);
			rampInput = temp;	/* for next stage */
		}
		/* Last level inverter */
		resPullDown = CalculateOnResistance(widthNMOS[numStage-1], NMOS, gInputParameter.temperature, gTech);
		capLoad = capOutput[numStage-1] + outputCap;
		tr = resPullDown * capLoad + outputCap * outputRes / 2;
		gm = CalculateTransconductance(widthNMOS[numStage-1], NMOS, gTech);
		beta = 1 / (resPullDown * gm);
		readLatency += horowitz(tr, beta, rampInput, &rampOutput);
		rampInput = _rampInput;
		writeLatency = readLatency;
	}
}

void OutputDriver::CalculatePower() {
	if (!initialized) {
		std::cout << "[Output Driver] Error: Require initialization first!" << std::endl;
	} else if (invalid) {
		readDynamicEnergy = writeDynamicEnergy = leakage = invalid_value;
	} else {
		/* Leakage power */
		leakage = 0;
		for (int i = 0; i < numStage; i++) {
			leakage += CalculateGateLeakage(INV, 1, widthNMOS[i], widthPMOS[i], gInputParameter.temperature, gTech)
					* gTech.vdd;
		}
		/* Dynamic energy */
		readDynamicEnergy = 0;
		double capLoad;
		for (int i = 0; i < numStage - 1; i++) {
			capLoad = capOutput[i] + capInput[i+1];
			readDynamicEnergy += capLoad * gTech.vdd * gTech.vdd;
		}
		capLoad = capOutput[numStage-1] + outputCap;	/* outputCap here means the final load capacitance */
		readDynamicEnergy += capLoad * gTech.vdd * gTech.vdd;
		writeDynamicEnergy = readDynamicEnergy;
	}
}

void OutputDriver::PrintProperty() {
	std::cout << "Output Driver Properties:" << std::endl;
	FunctionUnit::PrintProperty();
	std::cout << "Number of inverter stage: " << numStage << std::endl;
}
