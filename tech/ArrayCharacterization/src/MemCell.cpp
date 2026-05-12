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


#include "MemCell.hpp"
#include "formula.hpp"
#include "global.hpp"
#include "macros.hpp"
#include "enuminfo.hpp"

#include <math.h>

void MemCell::ReadCellFromFile(const std::string& inputFile)
{
    try {
        YAML::Node config = YAML::LoadFile(inputFile);
        
        // Basic Cell Properties
        yamlValueFromNode(memCellType, config, "MemCellType");
        
        yamlValueFromNode(processNode, config, "ProcessNode");
        yamlValueFromNode(area, config, "CellArea_F2");
        if (yamlValueFromNode(aspectRatio, config, "CellAspectRatio")) {
            heightInFeatureSize = sqrt(area * aspectRatio);
            widthInFeatureSize = sqrt(area / aspectRatio);
        }
        
        // Resistance Values
        if (config["Resistance"]) {
            YAML::Node resist = config["Resistance"];
            yamlValueFromNode(resistanceOnAtSetVoltage, resist, "OnAtSetVoltage_ohm");
            yamlValueFromNode(resistanceOffAtSetVoltage, resist, "OffAtSetVoltage_ohm");
            yamlValueFromNode(resistanceOnAtResetVoltage, resist, "OnAtResetVoltage_ohm");
            yamlValueFromNode(resistanceOffAtResetVoltage, resist, "OffAtResetVoltage_ohm");
            yamlValueFromNode(resistanceOnAtReadVoltage, resist, "OnAtReadVoltage_ohm");
            yamlValueFromNode(resistanceOn, resist, "OnAtReadVoltage_ohm");
            yamlValueFromNode(resistanceOffAtReadVoltage, resist, "OffAtReadVoltage_ohm");
            yamlValueFromNode(resistanceOff, resist, "OffAtReadVoltage_ohm");
            yamlValueFromNode(resistanceOnAtHalfReadVoltage, resist, "OnAtHalfReadVoltage_ohm");
            yamlValueFromNode(resistanceOffAtHalfReadVoltage, resist, "OffAtHalfReadVoltage_ohm");
            yamlValueFromNode(resistanceOnAtHalfResetVoltage, resist, "OnAtHalfResetVoltage_ohm");
        }
        
        // Also support flat resistance fields (backwards compatibility)
        yamlValueFromNode(resistanceOn, config, "ResistanceOn_ohm");
        yamlValueFromNode(resistanceOff, config, "ResistanceOff_ohm");
        
        // Capacitance
        if (config["Capacitance"]) {
            YAML::Node cap = config["Capacitance"];
            yamlValueFromNode(capacitanceOn, cap, "On_F");
            yamlValueFromNode(capacitanceOff, cap, "Off_F");
        }
        
        // Also support flat capacitance fields
        yamlValueFromNode(capacitanceOn, config, "CapacitanceOn_F");
        yamlValueFromNode(capacitanceOff, config, "CapacitanceOff_F");
        
        yamlValueFromNode(gateOxThicknessFactor, config, "GateOxThicknessFactor");
            
        yamlValueFromNode(widthSOIDevice, config, "SOIDeviceWidth_F");
        
        // Read Operation
        if (config["Read"]) {
            YAML::Node read = config["Read"];
            std::string mode;
            if (yamlValueFromNode(mode, read, "Mode")) {
                readMode = (mode == "voltage");
            }
            yamlValueFromNode(readVoltage, read, "Voltage_V");
            if (yamlValueFromNode(readCurrent, read, "Current_uA")) {
                readCurrent /= 1e6;
            }
            if (yamlValueFromNode(readPower, read, "Power_uW")) {
                readPower /= 1e6;
            }
        }
        
        yamlValueFromNode(wordlineBoostRatio, config, "WordlineBoostRatio");
        yamlValueFromNode(minSenseVoltage, config, "MinSenseVoltage_mV");
        yamlValueFromNode(maxStorageNodeDrop, config, "MaxStorageNodeDrop_V");
        
        // Reset Operation
        if (config["Reset"]) {
            YAML::Node reset = config["Reset"];
            std::string mode;
            if (yamlValueFromNode(mode, reset, "Mode")) {
                resetMode = (mode == "voltage");
            }
            yamlValueFromNode(resetVoltage, reset, "Voltage_V");
            if (yamlValueFromNode(resetCurrent, reset, "Current_uA")) {
                resetCurrent /= 1e6;
            }
            if (yamlValueFromNode(resetPulse, reset, "Pulse_ns")) {
                resetPulse /= 1e9;
            }
            if (yamlValueFromNode(resetEnergy, reset, "Energy_pJ")) {
                resetEnergy /= 1e12;
            }
        }
        
        // Set Operation
        if (config["Set"]) {
            YAML::Node set = config["Set"];
            std::string mode;
            if (yamlValueFromNode(mode, set, "Mode")) {
                setMode = (mode == "voltage");
            }
            yamlValueFromNode(setVoltage, set, "Voltage_V");
            if (yamlValueFromNode(setCurrent, set, "Current_uA")) {
                setCurrent /= 1e6;
            }
            if (yamlValueFromNode(setPulse, set, "Pulse_ns")) {
                setPulse /= 1e9;
            }
            if (yamlValueFromNode(setEnergy, set, "Energy_pJ")) {
                setEnergy /= 1e12;
            }
        }
        
        // Access Device
        if (config["Access"]) {
            YAML::Node access = config["Access"];
            yamlValueFromNode(accessType, access, "Type");
            if (access["CMOSWidth_F"]) {
                if (accessType != CMOS_access) {
                    std::cout << "Warning: CMOS width ignored (not CMOS-accessed)" << std::endl;
                } else {
                    yamlValueFromNode(widthAccessCMOS, access, "CMOSWidth_F");
                }
            }
            if (access["CMOSWidthR_F"]) {
                if (accessType != CMOS_access) {
                    std::cout << "Warning: CMOS width R ignored (not CMOS-accessed)" << std::endl;
                } else {
                    yamlValueFromNode(widthAccessCMOSR, access, "CMOSWidthR_F");
                }
            }
            yamlValueFromNode(voltageDropAccessDevice, access, "VoltageDropAccessDevice_V");
            if (yamlValueFromNode(leakageCurrentAccessDevice, access, "LeakageCurrentAccessDevice_uA")) {
                leakageCurrentAccessDevice /= 1e6;
            }
        }
        
        // Also support flat access fields
        yamlValueFromNode(accessType, config, "AccessType");
        if (config["AccessCMOSWidth_F"]) {
            if (accessType != CMOS_access) {
                std::cout << "Warning: CMOS width ignored (not CMOS-accessed)" << std::endl;
            } else {
                yamlValueFromNode(widthAccessCMOS, config, "AccessCMOSWidth_F");
            }
        }
        if (config["AccessCMOSWidthR_F"]) {
            if (accessType != CMOS_access) {
                std::cout << "Warning: CMOS width R ignored (not CMOS-accessed)" << std::endl;
            } else {
                yamlValueFromNode(widthAccessCMOSR, config, "AccessCMOSWidthR_F");
            }
        }
        yamlValueFromNode(voltageDropAccessDevice, config, "VoltageDropAccessDevice_V");
        if (yamlValueFromNode(leakageCurrentAccessDevice, config, "LeakageCurrentAccessDevice_uA")) {
            leakageCurrentAccessDevice /= 1e6;
        }
        
        // Additional Properties
        yamlValueFromNode(readFloating, config, "ReadFloating");
        
        // DRAM specific
        if (config["DRAMCellCapacitance_F"]) {
            if (memCellType != DRAM && memCellType != eDRAM && 
                memCellType != eDRAM3T && memCellType != eDRAM3T333) {
                std::cout << "Warning: DRAM capacitance ignored (not DRAM)" << std::endl;
            } else {
                yamlValueFromNode(capDRAMCell, config, "DRAMCellCapacitance_F");
            }
        }
        
        // SRAM specific
        if (config["SRAMCellNMOSWidth_F"]) {
            if (memCellType != SRAM) {
                std::cout << "Warning: SRAM NMOS width ignored (not SRAM)" << std::endl;
            } else {
                yamlValueFromNode(widthSRAMCellNMOS, config, "SRAMCellNMOSWidth_F");
            }
        }
        
        if (config["SRAMCellPMOSWidth_F"]) {
            if (memCellType != SRAM) {
                std::cout << "Warning: SRAM PMOS width ignored (not SRAM)" << std::endl;
            } else {
                yamlValueFromNode(widthSRAMCellPMOS, config, "SRAMCellPMOSWidth_F");
            }
        }
        
        // Flash specific
        if (config["Flash"]) {
            YAML::Node flash = config["Flash"];
            if (memCellType != SLCNAND && memCellType != MLCNAND) {
                std::cout << "Warning: Flash parameters ignored (not Flash)" << std::endl;
            } else {
                yamlValueFromNode(flashEraseVoltage, flash, "EraseVoltage_V");
                yamlValueFromNode(flashProgramVoltage, flash, "ProgramVoltage_V");
                yamlValueFromNode(flashPassVoltage, flash, "PassVoltage_V");
                if (yamlValueFromNode(flashEraseTime, flash, "EraseTime_ms")) {
                    flashEraseTime /= 1e3;
                }
                if (yamlValueFromNode(flashProgramTime, flash, "ProgramTime_us")) {
                    flashProgramTime /= 1e6;
                }
                yamlValueFromNode(gateCouplingRatio, flash, "GateCouplingRatio");
            }
        }
        
        // Also support flat flash fields
        if (config["FlashEraseVoltage_V"]) {
            if (memCellType != SLCNAND && memCellType != MLCNAND) {
                std::cout << "Warning: Flash erase voltage ignored (not Flash)" << std::endl;
            } else {
                yamlValueFromNode(flashEraseVoltage, config, "FlashEraseVoltage_V");
            }
        }
        if (config["FlashProgramVoltage_V"]) {
            if (memCellType != SLCNAND && memCellType != MLCNAND) {
                std::cout << "Warning: Flash program voltage ignored (not Flash)" << std::endl;
            } else {
                yamlValueFromNode(flashProgramVoltage, config, "FlashProgramVoltage_V");
            }
        }
        if (config["FlashPassVoltage_V"]) {
            if (memCellType != SLCNAND && memCellType != MLCNAND) {
                std::cout << "Warning: Flash pass voltage ignored (not Flash)" << std::endl;
            } else {
                yamlValueFromNode(flashPassVoltage, config, "FlashPassVoltage_V");
            }
        }
        if (config["FlashEraseTime_ms"]) {
            if (memCellType != SLCNAND && memCellType != MLCNAND) {
                std::cout << "Warning: Flash erase time ignored (not Flash)" << std::endl;
            } else {
                if (yamlValueFromNode(flashEraseTime, config, "FlashEraseTime_ms")) {
                    flashEraseTime /= 1e3;
                }
            }
        }
        if (config["FlashProgramTime_us"]) {
            if (memCellType != SLCNAND && memCellType != MLCNAND) {
                std::cout << "Warning: Flash program time ignored (not Flash)" << std::endl;
            } else {
                if (yamlValueFromNode(flashProgramTime, config, "FlashProgramTime_us")) {
                    flashProgramTime /= 1e6;
                }
            }
        }
        if (config["GateCouplingRatio"]) {
            if (memCellType != SLCNAND && memCellType != MLCNAND) {
                std::cout << "Warning: Gate coupling ratio ignored (not Flash)" << std::endl;
            } else {
                yamlValueFromNode(gateCouplingRatio, config, "GateCouplingRatio");
            }
        }
        
        // Retention time
        if (config["RetentionTime_us"]) {
            if (memCellType != DRAM && memCellType != eDRAM && 
                memCellType != eDRAM3T && memCellType != eDRAM3T333) {
                std::cout << "Warning: Retention time ignored (not DRAM)" << std::endl;
            } else {
                if (yamlValueFromNode(retentionTime, config, "RetentionTime_us")) {
                    retentionTime /= 1e6;
                }
            }
        }
        
        // MLC specific
        if (config["InputFingers"]) {
            if (memCellType != MLCCTT && memCellType != MLCFeFET && memCellType != MLCRRAM) {
                std::cout << "Warning: InputFingers used only for MLC SA" << std::endl;
            } else {
                yamlValueFromNode(nFingers, config, "InputFingers");
            }
        }
        
        if (config["CellLevels"]) {
            if (memCellType != MLCCTT && memCellType != MLCFeFET && memCellType != MLCRRAM) {
                std::cout << "Warning: CellLevels used only for MLC" << std::endl;
            } else {
                yamlValueFromNode(nLvl, config, "CellLevels");
            }
        }
        
    } catch (const YAML::Exception& e) {
        std::cout << "Error parsing YAML file: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    } catch (const std::exception& e) {
        std::cout << "Error reading file: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void MemCell::ApplyPVT() {
	temperature = gInputParameter.temperature;
    if (retentionTime == invalid_value) {
		// Calculate retention time if not given
		double leakageCurrent = 0;
		double effWidthAccessCMOS = 0;
		Technology* chosenTech = nullptr;
		double* currentOffNmosArr = nullptr;

		if (memCellType == eDRAM3T || memCellType == eDRAM3T333) {
			chosenTech = &gTechW;
			currentOffNmosArr = gTechW.currentOffNmos;
		} else if (memCellType == eDRAM || memCellType == DRAM) {
			chosenTech = &gTech;
			currentOffNmosArr = gTech.currentOffNmos;
		}

		if (chosenTech != nullptr) {
			if (chosenTech->featureSizeInNano >= 22) {
				effWidthAccessCMOS = gCell.widthAccessCMOS * chosenTech->featureSizeInNano * 1e-9;
			} else if (chosenTech->featureSizeInNano >= 3) {
				effWidthAccessCMOS = (int)ceil(gCell.widthAccessCMOS) * chosenTech->effective_width;
			} else {
				effWidthAccessCMOS = (int)ceil(gCell.widthAccessCMOS)
					* chosenTech->effective_width
					* chosenTech->max_sheet_num
					/ chosenTech->max_fin_per_GAA;
			}
			leakageCurrent = currentOffNmosArr[gInputParameter.temperature - 300] * effWidthAccessCMOS;
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
		std::cout <<"Warning[MemCell] : Try to get memristance from a non-memristor memory cell" << std::endl;
		return -1;
	}
}

void MemCell::CalculateWriteEnergy() {
	if (resetEnergy == 0) {
                std::cout << " Warning: over-writing reset energy" << std::endl;
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
				resetEnergy = gTech.vdd * fabs(resetCurrent) * resetPulse; /*TO-DO consider charge pump*/
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
                std::cout << " Warning: over-writing set energy" << std::endl;
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
				setEnergy = gTech.vdd * fabs(setCurrent) * setPulse; /*TO-DO consider charge pump*/
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
		if (gCell.readMode) {	/* voltage-sensing */
			if (readVoltage == 0) { /* Current-in voltage sensing */
				return gTech.vdd * readCurrent;
			}
			if (readCurrent == 0) { /*Voltage-divider sensing */
				double resInSerialForSenseAmp, maxBitlineCurrent;
				resInSerialForSenseAmp = sqrt(resistanceOn * resistanceOff);
				maxBitlineCurrent = (readVoltage - voltageDropAccessDevice) / (resistanceOn + resInSerialForSenseAmp);
				return gTech.vdd * maxBitlineCurrent;
			}
		} else { /* current-sensing */
			double maxBitlineCurrent = (readVoltage - voltageDropAccessDevice) / resistanceOn;
			return gTech.vdd * maxBitlineCurrent;
		}
	} else {
		return -1.0; /* should not call the function if read energy exists */
	}
	return -1.0;
}

void MemCell::PrintCell() {
    std::cout << "Memory Cell: " << memCellType << "\n";

	std::cout << "Cell Area (F^2)    : " << area << " (" << heightInFeatureSize << "Fx" << widthInFeatureSize << "F)\n";
	std::cout << "Cell Area (um^2)    : " << area / 1000000.0 * gTech.featureSizeInNano * gTech.featureSizeInNano << " (" << heightInFeatureSize * gTech.featureSizeInNano << "nm x" << widthInFeatureSize * gTech.featureSizeInNano << "nm y)\n";
	std::cout << "Cell Aspect Ratio  : " << aspectRatio << "\n";

	if (memCellType == PCRAM || memCellType == MRAM || memCellType == memristor || memCellType == FBRAM || memCellType == FeFET || memCellType == MLCFeFET || memCellType == MLCRRAM) {
		if (resistanceOn < 1e3) {
			std::cout << "Cell Turned-On Resistance : " << resistanceOn << "ohm\n";
		} else if (resistanceOn < 1e6) {
			std::cout << "Cell Turned-On Resistance : " << resistanceOn / 1e3 << "Kohm\n";
		} else {
			std::cout << "Cell Turned-On Resistance : " << resistanceOn / 1e6 << "Mohm\n";
        }
		if (resistanceOff < 1e3) {
			std::cout << "Cell Turned-Off Resistance: "<< resistanceOff << "ohm\n";
		} else if (resistanceOff < 1e6) {
			std::cout << "Cell Turned-Off Resistance: "<< resistanceOff / 1e3 << "Kohm\n";
		} else {
			std::cout << "Cell Turned-Off Resistance: "<< resistanceOff / 1e6 << "Mohm\n";
        }

		if (readMode) {
			std::cout << "Read Mode: Voltage-Sensing\n";
			if (readCurrent > 0) {
				std::cout << "  - Read Current: " << readCurrent * 1e6 << "uA\n";
            }
			if (readVoltage > 0) {
				std::cout << "  - Read Voltage: " << readVoltage << "V\n";
            }
		} else {
			std::cout << "Read Mode: Current-Sensing\n";
			if (readCurrent > 0) {
				std::cout << "  - Read Current: " << readCurrent * 1e6 << "uA\n";
            }
			if (readVoltage > 0) {
				std::cout << "  - Read Voltage: " << readVoltage << "V\n";
            }
		}

		if (resetMode) {
			std::cout << "Reset Mode: Voltage\n";
			std::cout << "  - Reset Voltage: " << resetVoltage << "V\n";
		} else {
			std::cout << "Reset Mode: Current\n";
			std::cout << "  - Reset Current: " << resetCurrent * 1e6 << "uA\n";
		}
		std::cout << "  - Reset Pulse: " << TO_SECOND(resetPulse) << "\n";

		if (setMode) {
			std::cout << "Set Mode: Voltage\n";
			std::cout << "  - Set Voltage: " << setVoltage << "V\n";
		} else {
			std::cout << "Set Mode: Current\n";
			std::cout << "  - Set Current: " << setCurrent * 1e6 << "uA\n";
		}
		std::cout << "  - Set Pulse: " << TO_SECOND(setPulse) << "\n";

        std::cout << "Access Type: " << accessType << "\n";
	} else if (memCellType == SRAM) {
		std::cout << "SRAM Cell Access Transistor Width: " << widthAccessCMOS << "F\n";
		std::cout << "SRAM Cell NMOS Width: " << widthSRAMCellNMOS << "F\n";
		std::cout << "SRAM Cell PMOS Width: " << widthSRAMCellPMOS << "F\n";
		std::cout << "SRAM Cell Peripheral Roadmap: " << gTech.deviceRoadmap << "\n";
		std::cout << "SRAM Cell Peripheral Node: " << gTech.featureSizeInNano << "nm\n";
		std::cout << "SRAM Cell VDD: " << gTech.vdd << "V\n";
		std::cout << "Temperature: " << gCell.temperature << "K\n";
	} else if (memCellType == DRAM || memCellType == eDRAM) {
		std::cout << "DRAM Cell Access Transistor Width: " << widthAccessCMOS << "F\n";
		std::cout << "DRAM Cell Peripheral Roadmap: " << gTech.deviceRoadmap << "\n";
		std::cout << "DRAM Cell Peripheral Node: " << gTech.featureSizeInNano << "nm\n";
		std::cout << "DRAM Cell VDD: " << gTech.vdd << "V\n";
		std::cout << "DRAM Cell WL_SWING: " << gTech.vpp << "V\n";
		std::cout << "Temperature: " << gCell.temperature << "K\n";
	} else if (memCellType == eDRAM3T || memCellType == eDRAM3T333) {
		std::cout << "3T DRAM Cell Write Access Transistor Width: " << widthAccessCMOS << "F\n";
		std::cout << "3T DRAM Cell Read Access Transistor Width: " << widthAccessCMOSR << "F\n";
		std::cout << "3T DRAM Cell Peripheral Roadmap: " << gTech.deviceRoadmap << "\n";
		std::cout << "3T DRAM Cell Write Access Roadmap: " << gTechW.deviceRoadmap << "\n";
		std::cout << "3T DRAM Cell Read Access Roadmap: " << gTechR.deviceRoadmap << "\n";
		std::cout << "3T DRAM Cell Peripheral Node: " << gTech.featureSizeInNano << "nm\n";
		std::cout << "3T DRAM Cell Write Access Node: " << gTechW.featureSizeInNano << "nm\n";
		std::cout << "3T DRAM Cell Read Access Node: " << gTechR.featureSizeInNano << "nm\n";
		std::cout << "3T DRAM Cell VDD: " << gTech.vdd << "V\n";
		std::cout << "3T DRAM Cell WWL_SWING: " << gTechW.vpp << "V\n";
		std::cout << "Temperature: " << gCell.temperature << "K\n";
	} else if (memCellType == SLCNAND) {
		std::cout << "Pass Voltage       : " << flashPassVoltage << "V\n";
		std::cout << "Programming Voltage: " << flashProgramVoltage << "V\n";
		std::cout << "Erase Voltage      : " << flashEraseVoltage << "V\n";
		std::cout << "Programming Time   : " << TO_SECOND(flashProgramTime) << "\n";
		std::cout << "Erase Time         : " << TO_SECOND(flashEraseTime) << "\n";
		std::cout << "Gate Coupling Ratio: " << gateCouplingRatio << "\n";
	} 
	if (memCellType == MLCCTT || memCellType == MLCFeFET || memCellType == MLCRRAM) {
			std::cout << "Number of Input Fingers: " << nFingers << "\n";
			std::cout << "Number of Levels per Cell: " << nLvl << "\n";
	}
}
