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


#include "Precharger.hpp"
#include "formula.hpp"
#include "global.hpp"

void Precharger::Initialize(double _voltagePrecharge, int _numColumn, double _capBitline, double _resBitline){
	if (initialized)
		std::cout << "[Precharger] Warning: Already initialized!" << std::endl;

	voltagePrecharge = _voltagePrecharge;
	numColumn  = _numColumn;
	capBitline = _capBitline;
	resBitline = _resBitline;
	capWireLoadPerColumn = gCell.widthInFeatureSize * gTech.featureSize * gLocalWire.capWirePerUnit;
	resWireLoadPerColumn = gCell.widthInFeatureSize * gTech.featureSize * gLocalWire.resWirePerUnit;
	widthInvNmos = MIN_NMOS_SIZE * gTech.featureSize;
	widthInvPmos = widthInvNmos * gTech.pnSizeRatio;
	widthPMOSBitlineEqual      = MIN_NMOS_SIZE * gTech.featureSize;
	widthPMOSBitlinePrecharger = 6 * gTech.featureSize;
	capLoadInv  = CalculateGateCap(widthPMOSBitlineEqual, gTech) + 2 * CalculateGateCap(widthPMOSBitlinePrecharger, gTech)
			+ CalculateDrainCap(widthInvNmos, NMOS, gTech.featureSize*40, gTech)
			+ CalculateDrainCap(widthInvPmos, PMOS, gTech.featureSize*40, gTech);
	capOutputBitlinePrecharger = CalculateDrainCap(widthPMOSBitlinePrecharger, PMOS, gTech.featureSize*40, gTech) + CalculateDrainCap(widthPMOSBitlineEqual, PMOS, gTech.featureSize*40, gTech);
	double capInputInv         = CalculateGateCap(widthInvNmos, gTech) + CalculateGateCap(widthInvPmos, gTech);
	capLoadPerColumn           = capInputInv + capWireLoadPerColumn;
	double capLoadOutputDriver = numColumn * capLoadPerColumn;
	outputDriver.Initialize(1, capInputInv, capLoadOutputDriver, 0 /* TO-DO */, true, latency_first, 0);  /* Always Latency First */

	initialized = true;
}

void Precharger::CalculateArea() {
	if (!initialized) {
		std::cout << "[Precharger] Error: Require initialization first!" << std::endl;
	} else {
		outputDriver.CalculateArea();
		double hBitlinePrechareger, wBitlinePrechareger;
		double hBitlineEqual, wBitlineEqual;
		double hInverter, wInverter;
		CalculateGateArea(INV, 1, 0, widthPMOSBitlinePrecharger, gTech.featureSize*40, gTech, &hBitlinePrechareger, &wBitlinePrechareger);
		CalculateGateArea(INV, 1, 0, widthPMOSBitlineEqual, gTech.featureSize*40, gTech, &hBitlineEqual, &wBitlineEqual);
		CalculateGateArea(INV, 1, widthInvNmos, widthInvPmos, gTech.featureSize*40, gTech, &hInverter, &wInverter);
		width = 2 * wBitlinePrechareger + wBitlineEqual;
		width = MAX(width, wInverter);
		width *= numColumn;
		width = MAX(width, outputDriver.width);
		height = MAX(hBitlinePrechareger, hBitlineEqual);
		height += hInverter;
		height = MAX(height, outputDriver.height);
		area = height * width;
	}
}

void Precharger::CalculateRC() {
	if (!initialized) {
		std::cout << "[Precharger] Error: Require initialization first!" << std::endl;
	} else {
		outputDriver.CalculateRC();
		//more accurate RC model would include drain Capacitances of Precharger and Equalization PMOS transistors
	}
}

void Precharger::CalculateLatency(double _rampInput){
	if (!initialized) {
		std::cout << "[Precharger] Error: Require initialization first!" << std::endl;
	} else {
		rampInput= _rampInput;
		outputDriver.CalculateLatency(rampInput);
		enableLatency = outputDriver.readLatency;
		double resPullDown;
		double tr;	/* time constant */
		double gm;	/* transconductance */
		double beta;	/* for horowitz calculation */
		double temp;
		resPullDown = CalculateOnResistance(widthInvNmos, NMOS, gInputParameter.temperature, gTech);
		tr = resPullDown * capLoadInv;
		gm = CalculateTransconductance(widthInvNmos, NMOS, gTech);
		beta = 1 / (resPullDown * gm);
		enableLatency += horowitz(tr, beta, outputDriver.rampOutput, &temp);
		readLatency = 0;
		double resPullUp = CalculateOnResistance(widthPMOSBitlinePrecharger, PMOS,
				gInputParameter.temperature, gTech);
		double tau = resPullUp * (capBitline + capOutputBitlinePrecharger) + resBitline * capBitline / 2;
		gm = CalculateTransconductance(widthPMOSBitlinePrecharger, PMOS, gTech);
		beta = 1 / (resPullUp * gm);
		readLatency += horowitz(tau, beta, temp, &rampOutput);
		writeLatency = readLatency;
        refreshLatency = readLatency;
	}
}

void Precharger::CalculatePower() {
	if (!initialized) {
		std::cout << "[Precharger] Error: Require initialization first!" << std::endl;
	} else {
		outputDriver.CalculatePower();
		/* Leakage power */
		leakage = outputDriver.leakage;
		leakage += numColumn * gTech.vdd * CalculateGateLeakage(INV, 1, widthInvNmos, widthInvPmos, gInputParameter.temperature, gTech);
		leakage += numColumn * voltagePrecharge * CalculateGateLeakage(INV, 1, 0, widthPMOSBitlinePrecharger,
				gInputParameter.temperature, gTech);

		/* Dynamic energy */
		/* We don't count bitline precharge energy into account because it is a charging process */
		readDynamicEnergy = outputDriver.readDynamicEnergy;
		readDynamicEnergy += capLoadInv * gTech.vdd * gTech.vdd * numColumn;
		writeDynamicEnergy = 0;		/* No precharging is needed during the write operation */
        refreshDynamicEnergy = readDynamicEnergy;
	}
}

void Precharger::PrintProperty() {
	std::cout << "Precharger Properties:" << std::endl;
	FunctionUnit::PrintProperty();
}
