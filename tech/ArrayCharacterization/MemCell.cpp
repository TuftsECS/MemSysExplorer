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


#include "MemCell.h"
#include "formula.h"
#include "global.h"
#include "macros.h"
#include "yaml-cpp/yaml.h"
#include <math.h>

MemCell::MemCell() {
	// TODO Auto-generated constructor stub
	memCellType         = PCRAM;
	area                = 0;
	aspectRatio         = 0;
	resistanceOn        = 0;
	resistanceOff       = 0;
	readMode            = true;
	readVoltage         = 0;
	readCurrent         = 0;
	readPower           = 0;
        wordlineBoostRatio  = 1.0;
	resetMode           = true;
	resetVoltage        = 0;
	resetCurrent        = 0;
	minSenseVoltage     = 0.08;
	resetPulse          = 0;
	resetEnergy         = 0;
	setMode             = true;
	setVoltage          = 0;
	setCurrent          = 0;
	setPulse            = 0;
	accessType          = CMOS_access;
	processNode         = 0;
	setEnergy           = 0;

	/* Optional */
	stitching         = 0;
	gateOxThicknessFactor = 2;
	widthSOIDevice = 0;
	widthAccessCMOS   = 0;
	widthAccessCMOSR   = 0;
	voltageDropAccessDevice = 0;
	leakageCurrentAccessDevice = 0;
	capDRAMCell		  = 0;
	widthSRAMCellNMOS = 2.08;	/* Default NMOS width in SRAM cells is 2.08 (from CACTI) */
	widthSRAMCellPMOS = 1.23;	/* Default PMOS width in SRAM cells is 1.23 (from CACTI) */

	/*For memristors */
	readFloating = false;
	resistanceOnAtSetVoltage = 0;
	resistanceOffAtSetVoltage = 0;
	resistanceOnAtResetVoltage = 0;
	resistanceOffAtResetVoltage = 0;
	resistanceOnAtReadVoltage = 0;
	resistanceOffAtReadVoltage = 0;
	resistanceOnAtHalfReadVoltage = 0;
	resistanceOffAtHalfReadVoltage = 0;

	retentionTime = invalid_value;
        /*For multi-level cells SA*/
        nFingers = 8;
        nLvl = 4;
}

MemCell::~MemCell() {
	// TODO Auto-generated destructor stub
}

void MemCell::ReadCellFromFile(const string & inputFile)
{
    try {
        YAML::Node config = YAML::LoadFile(inputFile);
        
        // Basic Cell Properties
        if (config["MemCellType"]) {
            string cellType = config["MemCellType"].as<string>();
            if (cellType == "SRAM")
                memCellType = SRAM;
            else if (cellType == "DRAM")
                memCellType = DRAM;
            else if (cellType == "eDRAM")
                memCellType = eDRAM;
            else if (cellType == "eDRAM3T")
                memCellType = eDRAM3T;
            else if (cellType == "eDRAM3T333")
                memCellType = eDRAM3T333;
            else if (cellType == "MRAM")
                memCellType = MRAM;
            else if (cellType == "PCRAM")
                memCellType = PCRAM;
            else if (cellType == "FBRAM")
                memCellType = FBRAM;
            else if (cellType == "memristor")
                memCellType = memristor;
            else if (cellType == "CTT")
                memCellType = CTT;
            else if (cellType == "MLCCTT")
                memCellType = MLCCTT;
            else if (cellType == "FeFET")
                memCellType = FeFET;
            else if (cellType == "MLCFeFET")
                memCellType = MLCFeFET;
            else if (cellType == "MLCRRAM")
                memCellType = MLCRRAM;
            else if (cellType == "SLCNAND")
                memCellType = SLCNAND;
            else
                memCellType = MLCNAND;
        }
        
        if (config["ProcessNode"])
            processNode = config["ProcessNode"].as<int>();
            
        if (config["CellArea_F2"])
            area = config["CellArea_F2"].as<double>();
            
        if (config["CellAspectRatio"]) {
            aspectRatio = config["CellAspectRatio"].as<double>();
            heightInFeatureSize = sqrt(area * aspectRatio);
            widthInFeatureSize = sqrt(area / aspectRatio);
        }
        
        // Resistance Values
        if (config["Resistance"]) {
            YAML::Node resist = config["Resistance"];
            if (resist["OnAtSetVoltage_ohm"])
                resistanceOnAtSetVoltage = resist["OnAtSetVoltage_ohm"].as<double>();
            if (resist["OffAtSetVoltage_ohm"])
                resistanceOffAtSetVoltage = resist["OffAtSetVoltage_ohm"].as<double>();
            if (resist["OnAtResetVoltage_ohm"])
                resistanceOnAtResetVoltage = resist["OnAtResetVoltage_ohm"].as<double>();
            if (resist["OffAtResetVoltage_ohm"])
                resistanceOffAtResetVoltage = resist["OffAtResetVoltage_ohm"].as<double>();
            if (resist["OnAtReadVoltage_ohm"]) {
                resistanceOnAtReadVoltage = resist["OnAtReadVoltage_ohm"].as<double>();
                resistanceOn = resistanceOnAtReadVoltage;
            }
            if (resist["OffAtReadVoltage_ohm"]) {
                resistanceOffAtReadVoltage = resist["OffAtReadVoltage_ohm"].as<double>();
                resistanceOff = resistanceOffAtReadVoltage;
            }
            if (resist["OnAtHalfReadVoltage_ohm"])
                resistanceOnAtHalfReadVoltage = resist["OnAtHalfReadVoltage_ohm"].as<double>();
            if (resist["OffAtHalfReadVoltage_ohm"])
                resistanceOffAtHalfReadVoltage = resist["OffAtHalfReadVoltage_ohm"].as<double>();
            if (resist["OnAtHalfResetVoltage_ohm"])
                resistanceOnAtHalfResetVoltage = resist["OnAtHalfResetVoltage_ohm"].as<double>();
        }
        
        // Also support flat resistance fields (backwards compatibility)
        if (config["ResistanceOn_ohm"])
            resistanceOn = config["ResistanceOn_ohm"].as<double>();
        if (config["ResistanceOff_ohm"])
            resistanceOff = config["ResistanceOff_ohm"].as<double>();
        
        // Capacitance
        if (config["Capacitance"]) {
            YAML::Node cap = config["Capacitance"];
            if (cap["On_F"])
                capacitanceOn = cap["On_F"].as<double>();
            if (cap["Off_F"])
                capacitanceOff = cap["Off_F"].as<double>();
        }
        
        // Also support flat capacitance fields
        if (config["CapacitanceOn_F"])
            capacitanceOn = config["CapacitanceOn_F"].as<double>();
        if (config["CapacitanceOff_F"])
            capacitanceOff = config["CapacitanceOff_F"].as<double>();
        
        if (config["GateOxThicknessFactor"])
            gateOxThicknessFactor = config["GateOxThicknessFactor"].as<double>();
            
        if (config["SOIDeviceWidth_F"])
            widthSOIDevice = config["SOIDeviceWidth_F"].as<double>();
        
        // Read Operation
        if (config["Read"]) {
            YAML::Node read = config["Read"];
            if (read["Mode"]) {
                string mode = read["Mode"].as<string>();
                readMode = (mode == "voltage");
            }
            if (read["Voltage_V"])
                readVoltage = read["Voltage_V"].as<double>();
            if (read["Current_uA"])
                readCurrent = read["Current_uA"].as<double>() / 1e6;
            if (read["Power_uW"])
                readPower = read["Power_uW"].as<double>() / 1e6;
        }
        
        if (config["WordlineBoostRatio"])
            wordlineBoostRatio = config["WordlineBoostRatio"].as<double>();
            
        if (config["MinSenseVoltage_mV"])
            minSenseVoltage = config["MinSenseVoltage_mV"].as<double>() / 1e3;
            
        if (config["MaxStorageNodeDrop_V"])
            maxStorageNodeDrop = config["MaxStorageNodeDrop_V"].as<double>();
        
        // Reset Operation
        if (config["Reset"]) {
            YAML::Node reset = config["Reset"];
            if (reset["Mode"]) {
                string mode = reset["Mode"].as<string>();
                resetMode = (mode == "voltage");
            }
            if (reset["Voltage_V"])
                resetVoltage = reset["Voltage_V"].as<double>();
            if (reset["Current_uA"])
                resetCurrent = reset["Current_uA"].as<double>() / 1e6;
            if (reset["Pulse_ns"])
                resetPulse = reset["Pulse_ns"].as<double>() / 1e9;
            if (reset["Energy_pJ"])
                resetEnergy = reset["Energy_pJ"].as<double>() / 1e12;
        }
        
        // Set Operation
        if (config["Set"]) {
            YAML::Node set = config["Set"];
            if (set["Mode"]) {
                string mode = set["Mode"].as<string>();
                setMode = (mode == "voltage");
            }
            if (set["Voltage_V"])
                setVoltage = set["Voltage_V"].as<double>();
            if (set["Current_uA"])
                setCurrent = set["Current_uA"].as<double>() / 1e6;
            if (set["Pulse_ns"])
                setPulse = set["Pulse_ns"].as<double>() / 1e9;
            if (set["Energy_pJ"])
                setEnergy = set["Energy_pJ"].as<double>() / 1e12;
        }
        
        // Access Device
        if (config["Access"]) {
            YAML::Node access = config["Access"];
            if (access["Type"]) {
                string type = access["Type"].as<string>();
                if (type == "CMOS")
                    accessType = CMOS_access;
                else if (type == "BJT")
                    accessType = BJT_access;
                else if (type == "diode")
                    accessType = diode_access;
                else
                    accessType = none_access;
            }
            if (access["CMOSWidth_F"]) {
                if (accessType != CMOS_access)
                    cout << "Warning: CMOS width ignored (not CMOS-accessed)" << endl;
                else
                    widthAccessCMOS = access["CMOSWidth_F"].as<double>();
            }
            if (access["CMOSWidthR_F"]) {
                if (accessType != CMOS_access)
                    cout << "Warning: CMOS width R ignored (not CMOS-accessed)" << endl;
                else
                    widthAccessCMOSR = access["CMOSWidthR_F"].as<double>();
            }
            if (access["VoltageDropAccessDevice_V"])
                voltageDropAccessDevice = access["VoltageDropAccessDevice_V"].as<double>();
            if (access["LeakageCurrentAccessDevice_uA"])
                leakageCurrentAccessDevice = access["LeakageCurrentAccessDevice_uA"].as<double>() / 1e6;
        }
        
        // Also support flat access fields
        if (config["AccessType"]) {
            string type = config["AccessType"].as<string>();
            if (type == "CMOS")
                accessType = CMOS_access;
            else if (type == "BJT")
                accessType = BJT_access;
            else if (type == "diode")
                accessType = diode_access;
            else
                accessType = none_access;
        }
        if (config["AccessCMOSWidth_F"]) {
            if (accessType != CMOS_access)
                cout << "Warning: CMOS width ignored (not CMOS-accessed)" << endl;
            else
                widthAccessCMOS = config["AccessCMOSWidth_F"].as<double>();
        }
        if (config["AccessCMOSWidthR_F"]) {
            if (accessType != CMOS_access)
                cout << "Warning: CMOS width R ignored (not CMOS-accessed)" << endl;
            else
                widthAccessCMOSR = config["AccessCMOSWidthR_F"].as<double>();
        }
        if (config["VoltageDropAccessDevice_V"])
            voltageDropAccessDevice = config["VoltageDropAccessDevice_V"].as<double>();
        if (config["LeakageCurrentAccessDevice_uA"])
            leakageCurrentAccessDevice = config["LeakageCurrentAccessDevice_uA"].as<double>() / 1e6;
        
        // Additional Properties
        if (config["ReadFloating"]) {
            readFloating = config["ReadFloating"].as<bool>();
        }
        
        // DRAM specific
        if (config["DRAMCellCapacitance_F"]) {
            if (memCellType != DRAM && memCellType != eDRAM && 
                memCellType != eDRAM3T && memCellType != eDRAM3T333)
                cout << "Warning: DRAM capacitance ignored (not DRAM)" << endl;
            else
                capDRAMCell = config["DRAMCellCapacitance_F"].as<double>();
        }
        
        // SRAM specific
        if (config["SRAMCellNMOSWidth_F"]) {
            if (memCellType != SRAM)
                cout << "Warning: SRAM NMOS width ignored (not SRAM)" << endl;
            else
                widthSRAMCellNMOS = config["SRAMCellNMOSWidth_F"].as<double>();
        }
        
        if (config["SRAMCellPMOSWidth_F"]) {
            if (memCellType != SRAM)
                cout << "Warning: SRAM PMOS width ignored (not SRAM)" << endl;
            else
                widthSRAMCellPMOS = config["SRAMCellPMOSWidth_F"].as<double>();
        }
        
        // Flash specific
        if (config["Flash"]) {
            YAML::Node flash = config["Flash"];
            if (memCellType != SLCNAND && memCellType != MLCNAND) {
                cout << "Warning: Flash parameters ignored (not Flash)" << endl;
            } else {
                if (flash["EraseVoltage_V"])
                    flashEraseVoltage = flash["EraseVoltage_V"].as<double>();
                if (flash["ProgramVoltage_V"])
                    flashProgramVoltage = flash["ProgramVoltage_V"].as<double>();
                if (flash["PassVoltage_V"])
                    flashPassVoltage = flash["PassVoltage_V"].as<double>();
                if (flash["EraseTime_ms"])
                    flashEraseTime = flash["EraseTime_ms"].as<double>() / 1e3;
                if (flash["ProgramTime_us"])
                    flashProgramTime = flash["ProgramTime_us"].as<double>() / 1e6;
                if (flash["GateCouplingRatio"])
                    gateCouplingRatio = flash["GateCouplingRatio"].as<double>();
            }
        }
        
        // Also support flat flash fields
        if (config["FlashEraseVoltage_V"]) {
            if (memCellType != SLCNAND && memCellType != MLCNAND)
                cout << "Warning: Flash erase voltage ignored (not Flash)" << endl;
            else
                flashEraseVoltage = config["FlashEraseVoltage_V"].as<double>();
        }
        if (config["FlashProgramVoltage_V"]) {
            if (memCellType != SLCNAND && memCellType != MLCNAND)
                cout << "Warning: Flash program voltage ignored (not Flash)" << endl;
            else
                flashProgramVoltage = config["FlashProgramVoltage_V"].as<double>();
        }
        if (config["FlashPassVoltage_V"]) {
            if (memCellType != SLCNAND && memCellType != MLCNAND)
                cout << "Warning: Flash pass voltage ignored (not Flash)" << endl;
            else
                flashPassVoltage = config["FlashPassVoltage_V"].as<double>();
        }
        if (config["FlashEraseTime_ms"]) {
            if (memCellType != SLCNAND && memCellType != MLCNAND)
                cout << "Warning: Flash erase time ignored (not Flash)" << endl;
            else
                flashEraseTime = config["FlashEraseTime_ms"].as<double>() / 1e3;
        }
        if (config["FlashProgramTime_us"]) {
            if (memCellType != SLCNAND && memCellType != MLCNAND)
                cout << "Warning: Flash program time ignored (not Flash)" << endl;
            else
                flashProgramTime = config["FlashProgramTime_us"].as<double>() / 1e6;
        }
        if (config["GateCouplingRatio"]) {
            if (memCellType != SLCNAND && memCellType != MLCNAND)
                cout << "Warning: Gate coupling ratio ignored (not Flash)" << endl;
            else
                gateCouplingRatio = config["GateCouplingRatio"].as<double>();
        }
        
        // Retention time
        if (config["RetentionTime_us"]) {
            if (memCellType != DRAM && memCellType != eDRAM && 
                memCellType != eDRAM3T && memCellType != eDRAM3T333)
                cout << "Warning: Retention time ignored (not DRAM)" << endl;
            else
                retentionTime = config["RetentionTime_us"].as<double>() / 1e6;
        }
        
        // MLC specific
        if (config["InputFingers"]) {
            if (memCellType != MLCCTT && memCellType != MLCFeFET && memCellType != MLCRRAM)
                cout << "Warning: InputFingers used only for MLC SA" << endl;
            else
                nFingers = config["InputFingers"].as<int>();
        }
        
        if (config["CellLevels"]) {
            if (memCellType != MLCCTT && memCellType != MLCFeFET && memCellType != MLCRRAM)
                cout << "Warning: CellLevels used only for MLC" << endl;
            else
                nLvl = config["CellLevels"].as<double>();
        }
        
    } catch (const YAML::Exception& e) {
        cout << "Error parsing YAML file: " << e.what() << endl;
        exit(-1);
    } catch (const std::exception& e) {
        cout << "Error reading file: " << e.what() << endl;
        exit(-1);
    }
}

void MemCell::ApplyPVT() {
	temperature = gInputParameter->temperature;
    if (retentionTime == invalid_value) {
		// Calculate retention time if not given
		double leakageCurrent = 0;
		double effWidthAccessCMOS = 0;
		Technology *chosenTech = NULL;
		double *currentOffNmosArr = NULL;

		if (memCellType == eDRAM3T || memCellType == eDRAM3T333) {
			chosenTech = gTechW;
			currentOffNmosArr = gTechW->currentOffNmos;
		} else if (memCellType == eDRAM || memCellType == DRAM) {
			chosenTech = gTech;
			currentOffNmosArr = gTech->currentOffNmos;
		}

		if (chosenTech != NULL) {
			if (chosenTech->featureSizeInNano >= 22) {
				effWidthAccessCMOS = gCell->widthAccessCMOS * chosenTech->featureSizeInNano * 1e-9;
			} else if (chosenTech->featureSizeInNano >= 3) {
				effWidthAccessCMOS = (int)ceil(gCell->widthAccessCMOS) * chosenTech->effective_width;
			} else {
				effWidthAccessCMOS = (int)ceil(gCell->widthAccessCMOS)
					* chosenTech->effective_width
					* chosenTech->max_sheet_num
					/ chosenTech->max_fin_per_GAA;
			}
			leakageCurrent = currentOffNmosArr[gInputParameter->temperature - 300] * effWidthAccessCMOS;
		} else {
			leakageCurrent = 0;
		}
		retentionTime = (capDRAMCell * maxStorageNodeDrop)/(leakageCurrent);
    }
	return;
}


void MemCell::CellScaling(int _targetProcessNode) {
	if ((processNode > 0) && (processNode != _targetProcessNode)) {
		double scalingFactor = (double)processNode / _targetProcessNode;
		if (memCellType == PCRAM) {
			resistanceOn *= scalingFactor;
			resistanceOff *= scalingFactor;
			if (!setMode) {
				setCurrent /= scalingFactor;
			} else {
				setVoltage *= 1;
			}
			if (!resetMode) {
				resetCurrent /= scalingFactor;
			} else {
				resetVoltage *= 1;
			}
			if (accessType == diode_access) {
				capacitanceOn /= scalingFactor; //TO-DO
				capacitanceOff /= scalingFactor; //TO-DO
			}
		} else if (memCellType == MRAM){ //TO-DO: MRAM
			resistanceOn *= scalingFactor * scalingFactor;
			resistanceOff *= scalingFactor * scalingFactor;
			if (!setMode) {
				setCurrent /= scalingFactor;
			} else {
				setVoltage *= scalingFactor;
			}
			if (!resetMode) {
				resetCurrent /= scalingFactor;
			} else {
				resetVoltage *= scalingFactor;
			}
			if (accessType == diode_access) {
				capacitanceOn /= scalingFactor; //TO-DO
				capacitanceOff /= scalingFactor; //TO-DO
			}
		} else if (memCellType == memristor) { //TO-DO: memristor

		} else { //TO-DO: other RAMs

		}
		processNode = _targetProcessNode;
	}
}

double MemCell::GetMemristance(double _relativeReadVoltage) { /* Get the LRS resistance of memristor at log-linera region of I-V curve */
	if (memCellType == memristor || memCellType == FeFET || memCellType == MLCFeFET || memCellType == MLCRRAM) {
		double x1, x2, x3;  // x1: read voltage, x2: half voltage, x3: applied voltage
		if (readVoltage == 0) {
			x1 = readCurrent * resistanceOnAtReadVoltage;
		} else {
			x1 = readVoltage;
		}
		x2 = readVoltage / 2;
		x3 = _relativeReadVoltage * readVoltage;
		double y1, y2 ,y3; // y1:log(read current), y2: log(leakage current at half read voltage
		y1 = log2(x1/resistanceOnAtReadVoltage);
		y2 = log2(x2/resistanceOnAtHalfReadVoltage);
		y3 = (y2 - y1) / (x2 -x1) * x3 + (x2 * y1 - x1 * y2) / (x2 - x1);  //insertion
		return x3 / pow(2, y3);
	} else {  // not memristor, can't call the function
		cout <<"Warning[MemCell] : Try to get memristance from a non-memristor memory cell" << endl;
		return -1;
	}
}

void MemCell::CalculateWriteEnergy() {
	if (resetEnergy == 0) {
                cout << " Warning: over-writing reset energy" << endl;
		if (resetMode) {
			if (memCellType == memristor || memCellType == FeFET || memCellType == MLCFeFET || memCellType == MLCRRAM)
				if (accessType == none_access)
					resetEnergy = fabs(resetVoltage) * (fabs(resetVoltage) - voltageDropAccessDevice) / resistanceOnAtResetVoltage * resetPulse;
				else
					resetEnergy = fabs(resetVoltage) * (fabs(resetVoltage) - voltageDropAccessDevice) / resistanceOn * resetPulse;
			else if (memCellType == PCRAM)
				resetEnergy = fabs(resetVoltage) * (fabs(resetVoltage) - voltageDropAccessDevice) / resistanceOn * resetPulse;	// PCM cells shows low resistance during most time of the switching
			else if (memCellType == FBRAM)
				resetEnergy = fabs(resetVoltage) * fabs(resetCurrent) * resetPulse;
			else
				resetEnergy = fabs(resetVoltage) * (fabs(resetVoltage) - voltageDropAccessDevice) / resistanceOn * resetPulse;
		} else {
			if (resetVoltage == 0){
				resetEnergy = gTech->vdd * fabs(resetCurrent) * resetPulse; /*TO-DO consider charge pump*/
			} else {
				resetEnergy = fabs(resetVoltage) * fabs(resetCurrent) * resetPulse;
			}
			/* previous model seems to be problematic
			if (memCellType == memristor)
				if (accessType == none_access)
					resetEnergy = resetCurrent * (resetCurrent * resistanceOffAtResetVoltage + voltageDropAccessDevice) * resetPulse;
				else
					resetEnergy = resetCurrent * (resetCurrent * resistanceOff + voltageDropAccessDevice) * resetPulse;
			else if (memCellType == PCRAM)
				resetEnergy = resetCurrent * (resetCurrent * resistanceOn + voltageDropAccessDevice) * resetPulse;		// PCM cells shows low resistance during most time of the switching
			else if (memCellType == FBRAM)
				resetEnergy = fabs(resetVoltage) * fabs(resetCurrent) * resetPulse;
			else
				resetEnergy = resetCurrent * (resetCurrent * resistanceOff + voltageDropAccessDevice) * resetPulse;
		    */
		}
	}
	if (setEnergy == 0) {
                cout << " Warning: over-writing set energy" << endl;
		if (setMode) {
			if (memCellType == memristor || memCellType == FeFET || memCellType == MLCFeFET || memCellType == MLCRRAM)
				if (accessType == none_access)
					setEnergy = fabs(setVoltage) * (fabs(setVoltage) - voltageDropAccessDevice) / resistanceOnAtSetVoltage * setPulse;
				else
					setEnergy = fabs(setVoltage) * (fabs(setVoltage) - voltageDropAccessDevice) / resistanceOn * setPulse;
			else if (memCellType == PCRAM)
				setEnergy = fabs(setVoltage) * (fabs(setVoltage) - voltageDropAccessDevice) / resistanceOn * setPulse;			// PCM cells shows low resistance during most time of the switching
			else if (memCellType == FBRAM)
				setEnergy = fabs(setVoltage) * fabs(setCurrent) * setPulse;
			else
				setEnergy = fabs(setVoltage) * (fabs(setVoltage) - voltageDropAccessDevice) / resistanceOn * setPulse;
		} else {
			if (resetVoltage == 0){
				setEnergy = gTech->vdd * fabs(setCurrent) * setPulse; /*TO-DO consider charge pump*/
			} else {
				setEnergy = fabs(setVoltage) * fabs(setCurrent) * setPulse;
			}
			/* previous model seems to be problematic
			if (memCellType == memristor)
				if (accessType == none_access)
					setEnergy = setCurrent * (setCurrent * resistanceOffAtSetVoltage + voltageDropAccessDevice) * setPulse;
				else
					setEnergy = setCurrent * (setCurrent * resistanceOff + voltageDropAccessDevice) * setPulse;
			else if (memCellType == PCRAM)
				setEnergy = setCurrent * (setCurrent * resistanceOn + voltageDropAccessDevice) * setPulse;		// PCM cells shows low resistance during most time of the switching
			else if (memCellType == FBRAM)
				setEnergy = fabs(setVoltage) * fabs(setCurrent) * setPulse;
			else
				setEnergy = setCurrent * (setCurrent * resistanceOff + voltageDropAccessDevice) * setPulse;
			*/
		}
	}
}

double MemCell::CalculateReadPower() { /* TO-DO consider charge pumped read voltage */
	if (readPower == 0) {
		if (gCell->readMode) {	/* voltage-sensing */
			if (readVoltage == 0) { /* Current-in voltage sensing */
				return gTech->vdd * readCurrent;
			}
			if (readCurrent == 0) { /*Voltage-divider sensing */
				double resInSerialForSenseAmp, maxBitlineCurrent;
				resInSerialForSenseAmp = sqrt(resistanceOn * resistanceOff);
				maxBitlineCurrent = (readVoltage - voltageDropAccessDevice) / (resistanceOn + resInSerialForSenseAmp);
				return gTech->vdd * maxBitlineCurrent;
			}
		} else { /* current-sensing */
			double maxBitlineCurrent = (readVoltage - voltageDropAccessDevice) / resistanceOn;
			return gTech->vdd * maxBitlineCurrent;
		}
	} else {
		return -1.0; /* should not call the function if read energy exists */
	}
	return -1.0;
}

void MemCell::PrintCell()
{
	switch (memCellType) {
	case SRAM:
		cout << "Memory Cell: SRAM" << endl;
		break;
	case DRAM:
		cout << "Memory Cell: DRAM" << endl;
		break;
	case eDRAM:
		cout << "Memory Cell: Embedded DRAM" << endl;
		break;
	case eDRAM3T:
		cout << "Memory Cell: 3T Embedded DRAM" << endl;
		break;
	case eDRAM3T333:
		cout << "Memory Cell: 333 Embedded DRAM" << endl;
		break;
	case MRAM:
		cout << "Memory Cell: MRAM (Magnetoresistive)" << endl;
		break;
	case PCRAM:
		cout << "Memory Cell: PCRAM (Phase-Change)" << endl;
		break;
	case memristor:
		cout << "Memory Cell: RRAM (Memristor)" << endl;
		break;
	case FBRAM:
		cout << "Memory Cell: FBRAM (Floating Body)" <<endl;
		break;
	case SLCNAND:
		cout << "Memory Cell: Single-Level Cell NAND Flash" << endl;
		break;
	case MLCNAND:
		cout << "Memory Cell: Multi-Level Cell NAND Flash" << endl;
		break;
	case CTT:
		cout << "Memory Cell: Single-Level Cell CTT" << endl;
		break;
	case MLCCTT:
		cout << "Memory Cell: Multi-Level Cell CTT" << endl;
		break;
	case FeFET:
		cout << "Memory Cell: Single-Level Cell FeFET" << endl;
		break;
	case MLCFeFET:
		cout << "Memory Cell: Multi-Level Cell FeFET" << endl;
		break;
	case MLCRRAM:
		cout << "Memory Cell: Multi-Level Cell RRAM (Memristor)" << endl;
		break;
	default:
		cout << "Memory Cell: Unknown" << endl;
	}
	cout << "Cell Area (F^2)    : " << area << " (" << heightInFeatureSize << "Fx" << widthInFeatureSize << "F)" << endl;
	cout << "Cell Area (um^2)    : " << area/1000000.0*gTech->featureSizeInNano*gTech->featureSizeInNano << " (" << heightInFeatureSize*gTech->featureSizeInNano << "nm x" << widthInFeatureSize*gTech->featureSizeInNano << "nm y)" << endl;
	cout << "Cell Aspect Ratio  : " << aspectRatio << endl;

	if (memCellType == PCRAM || memCellType == MRAM || memCellType == memristor || memCellType == FBRAM || memCellType == FeFET || memCellType == MLCFeFET || memCellType == MLCRRAM) {
		if (resistanceOn < 1e3 )
			cout << "Cell Turned-On Resistance : " << resistanceOn << "ohm" << endl;
		else if (resistanceOn < 1e6)
			cout << "Cell Turned-On Resistance : " << resistanceOn / 1e3 << "Kohm" << endl;
		else
			cout << "Cell Turned-On Resistance : " << resistanceOn / 1e6 << "Mohm" << endl;
		if (resistanceOff < 1e3 )
			cout << "Cell Turned-Off Resistance: "<< resistanceOff << "ohm" << endl;
		else if (resistanceOff < 1e6)
			cout << "Cell Turned-Off Resistance: "<< resistanceOff / 1e3 << "Kohm" << endl;
		else
			cout << "Cell Turned-Off Resistance: "<< resistanceOff / 1e6 << "Mohm" << endl;

		if (readMode) {
			cout << "Read Mode: Voltage-Sensing" << endl;
			if (readCurrent > 0)
				cout << "  - Read Current: " << readCurrent * 1e6 << "uA" << endl;
			if (readVoltage > 0)
				cout << "  - Read Voltage: " << readVoltage << "V" << endl;
		} else {
			cout << "Read Mode: Current-Sensing" << endl;
			if (readCurrent > 0)
				cout << "  - Read Current: " << readCurrent * 1e6 << "uA" << endl;
			if (readVoltage > 0)
				cout << "  - Read Voltage: " << readVoltage << "V" << endl;
		}

		if (resetMode) {
			cout << "Reset Mode: Voltage" << endl;
			cout << "  - Reset Voltage: " << resetVoltage << "V" << endl;
		} else {
			cout << "Reset Mode: Current" << endl;
			cout << "  - Reset Current: " << resetCurrent * 1e6 << "uA" << endl;
		}
		cout << "  - Reset Pulse: " << TO_SECOND(resetPulse) << endl;

		if (setMode) {
			cout << "Set Mode: Voltage" << endl;
			cout << "  - Set Voltage: " << setVoltage << "V" << endl;
		} else {
			cout << "Set Mode: Current" << endl;
			cout << "  - Set Current: " << setCurrent * 1e6 << "uA" << endl;
		}
		cout << "  - Set Pulse: " << TO_SECOND(setPulse) << endl;

		switch (accessType) {
		case CMOS_access:
			cout << "Access Type: CMOS" << endl;
			break;
		case BJT_access:
			cout << "Access Type: BJT" << endl;
			break;
		case diode_access:
			cout << "Access Type: Diode" << endl;
			break;
		default:
			cout << "Access Type: None Access Device" << endl;
		}
	} else if (memCellType == SRAM) {
		cout << "SRAM Cell Access Transistor Width: " << widthAccessCMOS << "F" << endl;
		cout << "SRAM Cell NMOS Width: " << widthSRAMCellNMOS << "F" << endl;
		cout << "SRAM Cell PMOS Width: " << widthSRAMCellPMOS << "F" << endl;
		cout << "SRAM Cell Peripheral Roadmap: " << gTech->deviceRoadmap << endl;
		cout << "SRAM Cell Peripheral Node: " << gTech->featureSizeInNano << "nm" << endl;
		cout << "SRAM Cell VDD: " << gTech->vdd << "V" << endl;
		cout << "Temperature: " << gCell->temperature << "K" << endl;
	} else if (memCellType == DRAM || memCellType == eDRAM) {
		cout << "DRAM Cell Access Transistor Width: " << widthAccessCMOS << "F" << endl;
		cout << "DRAM Cell Peripheral Roadmap: " << gTech->deviceRoadmap << endl;
		cout << "DRAM Cell Peripheral Node: " << gTech->featureSizeInNano << "nm" << endl;
		cout << "DRAM Cell VDD: " << gTech->vdd << "V" << endl;
		cout << "DRAM Cell WL_SWING: " << gTech->vpp << "V" << endl;
		cout << "Temperature: " << gCell->temperature << "K" << endl;
	} else if (memCellType == eDRAM3T || memCellType == eDRAM3T333) {
		cout << "3T DRAM Cell Write Access Transistor Width: " << widthAccessCMOS << "F" << endl;
		cout << "3T DRAM Cell Read Access Transistor Width: " << widthAccessCMOSR << "F" << endl;
		cout << "3T DRAM Cell Peripheral Roadmap: " << gTech->deviceRoadmap << endl;
		cout << "3T DRAM Cell Write Access Roadmap: " << gTechW->deviceRoadmap << endl;
		cout << "3T DRAM Cell Read Access Roadmap: " << gTechR->deviceRoadmap << endl;
		cout << "3T DRAM Cell Peripheral Node: " << gTech->featureSizeInNano << "nm" << endl;
		cout << "3T DRAM Cell Write Access Node: " << gTechW->featureSizeInNano << "nm" << endl;
		cout << "3T DRAM Cell Read Access Node: " << gTechR->featureSizeInNano << "nm" << endl;
		cout << "3T DRAM Cell VDD: " << gTech->vdd << "V" << endl;
		cout << "3T DRAM Cell WWL_SWING: " << gTechW->vpp << "V" << endl;
		cout << "Temperature: " << gCell->temperature << "K" << endl;
	} else if (memCellType == SLCNAND) {
		cout << "Pass Voltage       : " << flashPassVoltage << "V" << endl;
		cout << "Programming Voltage: " << flashProgramVoltage << "V" << endl;
		cout << "Erase Voltage      : " << flashEraseVoltage << "V" << endl;
		cout << "Programming Time   : " << TO_SECOND(flashProgramTime) << endl;
		cout << "Erase Time         : " << TO_SECOND(flashEraseTime) << endl;
		cout << "Gate Coupling Ratio: " << gateCouplingRatio << endl;
	} 
	if (memCellType == MLCCTT || memCellType == MLCFeFET || memCellType == MLCRRAM) {
			cout << "Number of Input Fingers: " << nFingers << endl;
			cout << "Number of Levels per Cell: " << nLvl << endl;
	}
}
