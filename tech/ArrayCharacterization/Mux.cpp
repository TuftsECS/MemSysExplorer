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


#include "Mux.h"
#include "global.h"
#include "formula.h"

Mux::~Mux() {
	// TODO Auto-generated destructor stub
}

void Mux::Initialize(int _numInput, long long _numMux, double _capLoad, double _capInputNextStage, double _minDriverCurrent){
	if (initialized)
		cout << "[Mux] Warning: Already initialized!" << endl;

	numInput = _numInput;
	numMux = _numMux;
	capLoad = _capLoad;
	capInputNextStage = _capInputNextStage;
	minDriverCurrent = _minDriverCurrent;

	if ((numInput > 1) && (numMux > 0 )) {
		double minNMOSWidth = minDriverCurrent / gTech->currentOnNmos[gInputParameter->temperature - 300];
		if (gCell->memCellType == MRAM || gCell->memCellType == PCRAM || gCell->memCellType == memristor || gCell->memCellType == FeFET || gCell->memCellType == MLCFeFET || gCell->memCellType == MLCRRAM) {
			/* Mux resistance should be small enough for voltage dividing */
			double maxResNMOSPassTransistor = gCell->resistanceOn * IR_DROP_TOLERANCE;
	    	        widthNMOSPassTransistor = CalculateOnResistance(((gTech->featureSize <= 14*1e-9)? 2:1)*gTech->featureSize, NMOS, gInputParameter->temperature, *gTech)
					* gTech->featureSize / maxResNMOSPassTransistor;
	    	        if (widthNMOSPassTransistor > gInputParameter->maxNmosSize * gTech->featureSize) {	// Change the transistor size to avoid severe IR drop
	    		    widthNMOSPassTransistor = gInputParameter->maxNmosSize * gTech->featureSize;
	    	        }
			widthNMOSPassTransistor = MAX(MAX(widthNMOSPassTransistor,minNMOSWidth), 6 * MIN_NMOS_SIZE * gTech->featureSize);
		} else {
			widthNMOSPassTransistor = MAX(6 * MIN_NMOS_SIZE * gTech->featureSize, minNMOSWidth);
		}
	}

	initialized = true;
}

void Mux::CalculateArea(){
	if (!initialized) {
		cout << "[Mux] Error: Require initialization first!" << endl;
	} else {
		if ((numInput > 1) && (numMux > 0 )) {
			double h,w;
			CalculateGateArea(INV, 1, widthNMOSPassTransistor, 0, ((gTech->featureSize <= 14*1e-9)? 2:1)*gTech->featureSize*40, *gTech, &h, &w);
			width = numMux * numInput * w;
			height = h;
			area = width * height;
		} else {
			height = width = area = 0;
		}
	}
}

void Mux::CalculateRC() {
	if (!initialized) {
		cout << "[Mux] Error: Require initialization first!" << endl;
	} else {
		if ((numInput > 1) && (numMux > 0 )) {
			capNMOSPassTransistor = CalculateDrainCap(widthNMOSPassTransistor, NMOS, ((gTech->featureSize <= 14*1e-9)? 2:1)*gTech->featureSize*40, *gTech);
			capForPreviousPowerCalculation = capNMOSPassTransistor;
			capOutput = numInput * capNMOSPassTransistor;
			capForPreviousDelayCalculation = capOutput + capNMOSPassTransistor + capLoad;
			resNMOSPassTransistor = CalculateOnResistance(widthNMOSPassTransistor, NMOS, gInputParameter->temperature, *gTech);
		} else {
			;	/* nothing to do */
		}
	}
}

void Mux::CalculateLatency(double _rampInput) {  //rampInput is actually useless in Mux module
	if (!initialized) {
		cout << "[Mux] Error: Require initialization first!" << endl;
	} else {
		if ((numInput > 1) && (numMux > 0 )) {
			rampInput = _rampInput;
			double tr;
			tr = resNMOSPassTransistor * (capOutput + capLoad);
			readLatency = 2.3 * tr;
			writeLatency = readLatency;
		} else {
			readLatency = writeLatency = 0;
		}
	}
}

void Mux::CalculatePower() {
	if (!initialized) {
		cout << "[Mux] Error: Require initialization first!" << endl;
	} else {
		if ((numInput > 1) && (numMux > 0 )) {
			leakage = 0; //TO-DO
			readDynamicEnergy = (capOutput + capInputNextStage) * gTech->vdd * (gTech->vdd - gTech->vth);
			readDynamicEnergy *= numMux;  //worst-case dynamic power analysis
			writeDynamicEnergy = readDynamicEnergy;
		} else {
			readDynamicEnergy = writeDynamicEnergy = leakage = 0;
		}
	}
}

void Mux::PrintProperty() {
	cout << "Mux Properties:" << endl;
	FunctionUnit::PrintProperty();
}

Mux& Mux::operator=(const Mux& rhs) {
	height = rhs.height;
	width = rhs.width;
	area = rhs.area;
	readLatency = rhs.readLatency;
	writeLatency = rhs.writeLatency;
	readDynamicEnergy = rhs.readDynamicEnergy;
	writeDynamicEnergy = rhs.writeDynamicEnergy;
	leakage = rhs.leakage;
	initialized = rhs.initialized;
	numInput = rhs.numInput;
	numMux = rhs.numMux;
	capLoad = rhs.capLoad;
	capInputNextStage = rhs.capInputNextStage;
	minDriverCurrent = rhs.minDriverCurrent;
    capOutput = rhs.capOutput;
	widthNMOSPassTransistor = rhs.widthNMOSPassTransistor;
	resNMOSPassTransistor = rhs.resNMOSPassTransistor;
	capNMOSPassTransistor = rhs.capNMOSPassTransistor;
	capForPreviousDelayCalculation = rhs.capForPreviousDelayCalculation;
	capForPreviousPowerCalculation = rhs.capForPreviousPowerCalculation;
	rampInput = rhs.rampInput;
	rampOutput = rhs.rampOutput;

	return *this;
}
