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
#include "enumInfo.hpp"
#include "unit.hpp"
#include "input.hpp"

#include <math.h>
#include <type_traits>

/* Reads the same keys as the cell types in include/cell, so the legacy cell stays in sync with them */
void MemCell::ReadCellFromFile(const std::string& inputFile)
{
    using namespace mse::unit;

    try {
        mse::YamlInputFile file(inputFile);

        // Fills a field from a key when the key is in the file, with unit values converted to base units
        const auto fill = [&file] <typename T, typename Field> (std::type_identity<T>, const std::string& key, Field& field) {
            mse::ScalarParameter<T> parameter{key, "", T(field)};
            if (!file.fillParameter(parameter)) {
                return false;
            }
            if constexpr (std::is_arithmetic_v<Field> && !std::is_arithmetic_v<T>) {
                field = parameter.value().value();
            } else {
                field = parameter.value();
            }

            return true;
        };
        const auto number = std::type_identity<double>{};

        // Basic Cell Properties
        fill(std::type_identity<MemCellType>{}, "MemCellType", memCellType);
        fill(std::type_identity<int>{}, "ProcessNode", processNode);
        fill(number, "Area", area);
        if (fill(number, "AspectRatio", aspectRatio)) {
            heightInFeatureSize = sqrt(area * aspectRatio);
            widthInFeatureSize = sqrt(area / aspectRatio);
        }

        // Resistance Values
        if (fill(std::type_identity<Ohm>{}, "Resistance.On", resistanceOn)) {
            resistanceOnAtReadVoltage = resistanceOn;
        }
        if (fill(std::type_identity<Ohm>{}, "Resistance.Off", resistanceOff)) {
            resistanceOffAtReadVoltage = resistanceOff;
        }
        fill(std::type_identity<Ohm>{}, "Resistance.OnAtSetVoltage", resistanceOnAtSetVoltage);
        fill(std::type_identity<Ohm>{}, "Resistance.OffAtSetVoltage", resistanceOffAtSetVoltage);
        fill(std::type_identity<Ohm>{}, "Resistance.OnAtResetVoltage", resistanceOnAtResetVoltage);
        fill(std::type_identity<Ohm>{}, "Resistance.OffAtResetVoltage", resistanceOffAtResetVoltage);
        fill(std::type_identity<Ohm>{}, "Resistance.OnAtHalfReadVoltage", resistanceOnAtHalfReadVoltage);
        fill(std::type_identity<Ohm>{}, "Resistance.OffAtHalfReadVoltage", resistanceOffAtHalfReadVoltage);
        fill(std::type_identity<Ohm>{}, "Resistance.OnAtHalfResetVoltage", resistanceOnAtHalfResetVoltage);

        // Capacitance
        fill(std::type_identity<Farad>{}, "Capacitance.On", capacitanceOn);
        fill(std::type_identity<Farad>{}, "Capacitance.Off", capacitanceOff);

        // Floating body
        fill(number, "FloatingBody.GateOxideThicknessFactor", gateOxThicknessFactor);
        fill(std::type_identity<Feature>{}, "FloatingBody.Width", widthSOIDevice);

        // Read Operation
        std::string mode;
        if (fill(std::type_identity<std::string>{}, "Read.Mode", mode)) {
            readMode = (mode == "voltage");
        }
        fill(std::type_identity<Volt>{}, "Read.Voltage", readVoltage);
        fill(std::type_identity<Ampere>{}, "Read.Current", readCurrent);
        fill(std::type_identity<Watt>{}, "Read.Power", readPower);

        fill(number, "WordlineBoostRatio", wordlineBoostRatio);
        fill(std::type_identity<Volt>{}, "MinSenseVoltage", minSenseVoltage);
        fill(std::type_identity<Volt>{}, "Storage.MaxNodeDrop", maxStorageNodeDrop);

        // Reset Operation
        if (fill(std::type_identity<std::string>{}, "Reset.Mode", mode)) {
            resetMode = (mode == "voltage");
        }
        fill(std::type_identity<Volt>{}, "Reset.Voltage", resetVoltage);
        fill(std::type_identity<Ampere>{}, "Reset.Current", resetCurrent);
        fill(std::type_identity<Second>{}, "Reset.Pulse", resetPulse);
        fill(std::type_identity<Joule>{}, "Reset.Energy", resetEnergy);

        // Set Operation
        if (fill(std::type_identity<std::string>{}, "Set.Mode", mode)) {
            setMode = (mode == "voltage");
        }
        fill(std::type_identity<Volt>{}, "Set.Voltage", setVoltage);
        fill(std::type_identity<Ampere>{}, "Set.Current", setCurrent);
        fill(std::type_identity<Second>{}, "Set.Pulse", setPulse);
        fill(std::type_identity<Joule>{}, "Set.Energy", setEnergy);

        // Access Device
        fill(std::type_identity<CellAccessType>{}, "Access.Type", accessType);
        double width = 0;
        if (fill(std::type_identity<Feature>{}, "Access.Width", width)) {
            if (accessType != CMOS_access) {
                std::cout << "Warning: CMOS width ignored (not CMOS-accessed)" << std::endl;
            } else {
                widthAccessCMOS = width;
            }
        }
        if (fill(std::type_identity<Feature>{}, "Access.ReadWidth", width)) {
            if (accessType != CMOS_access) {
                std::cout << "Warning: CMOS width R ignored (not CMOS-accessed)" << std::endl;
            } else {
                widthAccessCMOSR = width;
            }
        }
        fill(std::type_identity<Volt>{}, "Access.VoltageDrop", voltageDropAccessDevice);
        fill(std::type_identity<Ampere>{}, "Access.LeakageCurrent", leakageCurrentAccessDevice);

        // Additional Properties
        fill(std::type_identity<bool>{}, "ReadFloating", readFloating);

        // DRAM specific
        const bool dramCell = memCellType == DRAM || memCellType == eDRAM || memCellType == eDRAM3T || memCellType == eDRAM3T333;
        double capacitance = 0;
        if (fill(std::type_identity<Farad>{}, "Storage.Capacitance", capacitance)) {
            if (!dramCell) {
                std::cout << "Warning: DRAM capacitance ignored (not DRAM)" << std::endl;
            } else {
                capDRAMCell = capacitance;
            }
        }
        double retention = 0;
        if (fill(std::type_identity<Second>{}, "Storage.RetentionTime", retention)) {
            if (!dramCell) {
                std::cout << "Warning: Retention time ignored (not DRAM)" << std::endl;
            } else {
                retentionTime = retention;
            }
        }

        // SRAM specific
        if (fill(std::type_identity<Feature>{}, "Inverter.NmosWidth", width)) {
            if (memCellType != SRAM) {
                std::cout << "Warning: SRAM NMOS width ignored (not SRAM)" << std::endl;
            } else {
                widthSRAMCellNMOS = width;
            }
        }
        if (fill(std::type_identity<Feature>{}, "Inverter.PmosWidth", width)) {
            if (memCellType != SRAM) {
                std::cout << "Warning: SRAM PMOS width ignored (not SRAM)" << std::endl;
            } else {
                widthSRAMCellPMOS = width;
            }
        }

        // Flash specific
        const bool flashCell = memCellType == SLCNAND || memCellType == MLCNAND;
        const auto fillFlash = [&fill, flashCell] <typename T> (std::type_identity<T> type, const std::string& key, double& field) {
            double value = field;
            if (fill(type, key, value)) {
                if (!flashCell) {
                    std::cout << "Warning: Flash parameter " << key << " ignored (not Flash)" << std::endl;
                } else {
                    field = value;
                }
            }
        };
        fillFlash(std::type_identity<Volt>{}, "Flash.EraseVoltage", flashEraseVoltage);
        fillFlash(std::type_identity<Volt>{}, "Flash.ProgramVoltage", flashProgramVoltage);
        fillFlash(std::type_identity<Volt>{}, "Flash.PassVoltage", flashPassVoltage);
        fillFlash(std::type_identity<Second>{}, "Flash.EraseTime", flashEraseTime);
        fillFlash(std::type_identity<Second>{}, "Flash.ProgramTime", flashProgramTime);
        fillFlash(number, "Flash.GateCouplingRatio", gateCouplingRatio);

        // MLC specific
        const bool mlcCell = memCellType == MLCCTT || memCellType == MLCFeFET || memCellType == MLCRRAM;
        int fingers = nFingers;
        if (fill(std::type_identity<int>{}, "MultiLevel.SenseAmpFingers", fingers)) {
            if (!mlcCell) {
                std::cout << "Warning: MultiLevel.SenseAmpFingers used only for MLC SA" << std::endl;
            } else {
                nFingers = fingers;
            }
        }
        double levels = nLvl;
        if (fill(number, "MultiLevel.Levels", levels)) {
            if (!mlcCell) {
                std::cout << "Warning: MultiLevel.Levels used only for MLC" << std::endl;
            } else {
                nLvl = levels;
            }
        }

        // Warns about keys no cell reads, like those of the older cell file layout
        file.verifyStrictAccess();

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
    if (retentionTime == mse::constant::invalidValue) {
		// Calculate retention time if not given
		double leakageCurrent = 0;
		double effWidthAccessCMOS = 0;
		Technology* chosenTech = nullptr;
		const Technology::TemperatureTable* currentOffNmosArr = nullptr;

		if (memCellType == eDRAM3T || memCellType == eDRAM3T333) {
			chosenTech = &gTechW;
			currentOffNmosArr = &gTechW.currentOffNmos;
		} else if (memCellType == eDRAM || memCellType == DRAM) {
			chosenTech = &gTech;
			currentOffNmosArr = &gTech.currentOffNmos;
		}

		if (chosenTech != nullptr) {
			if (chosenTech->featureSizeInNano >= 22) {
				effWidthAccessCMOS = gCell.widthAccessCMOS * chosenTech->featureSizeInNano * 1e-9;
			} else if (chosenTech->featureSizeInNano >= 3) {
				effWidthAccessCMOS = (int)ceil(gCell.widthAccessCMOS) * chosenTech->effectiveWidth.value();
			} else {
				effWidthAccessCMOS = (int)ceil(gCell.widthAccessCMOS)
					* chosenTech->effectiveWidth.value()
					* chosenTech->maximumSheetCount
					/ chosenTech->maximumFinsPerGAA;
			}
			leakageCurrent = (*currentOffNmosArr)[gInputParameter.temperature - 300].value() * effWidthAccessCMOS;
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
				resetEnergy = gTech.vdd.value() * fabs(resetCurrent) * resetPulse; /*TO-DO consider charge pump*/
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
				setEnergy = gTech.vdd.value() * fabs(setCurrent) * setPulse; /*TO-DO consider charge pump*/
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
				return gTech.vdd.value() * readCurrent;
			}
			if (readCurrent == 0) { /*Voltage-divider sensing */
				double resInSerialForSenseAmp, maxBitlineCurrent;
				resInSerialForSenseAmp = sqrt(resistanceOn * resistanceOff);
				maxBitlineCurrent = (readVoltage - voltageDropAccessDevice) / (resistanceOn + resInSerialForSenseAmp);
				return gTech.vdd.value() * maxBitlineCurrent;
			}
		} else { /* current-sensing */
			double maxBitlineCurrent = (readVoltage - voltageDropAccessDevice) / resistanceOn;
			return gTech.vdd.value() * maxBitlineCurrent;
		}
	} else {
		return -1.0; /* should not call the function if read energy exists */
	}
	return -1.0;
}

void MemCell::PrintCell() {
    using namespace mse::unit;

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
		std::cout << "  - Reset Pulse: " << Second(resetPulse) << "\n";

		if (setMode) {
			std::cout << "Set Mode: Voltage\n";
			std::cout << "  - Set Voltage: " << setVoltage << "V\n";
		} else {
			std::cout << "Set Mode: Current\n";
			std::cout << "  - Set Current: " << setCurrent * 1e6 << "uA\n";
		}
		std::cout << "  - Set Pulse: " << Second(setPulse) << "\n";

        std::cout << "Access Type: " << accessType << "\n";
	} else if (memCellType == SRAM) {
		std::cout << "SRAM Cell Access Transistor Width: " << widthAccessCMOS << "F\n";
		std::cout << "SRAM Cell NMOS Width: " << widthSRAMCellNMOS << "F\n";
		std::cout << "SRAM Cell PMOS Width: " << widthSRAMCellPMOS << "F\n";
		std::cout << "SRAM Cell Peripheral Roadmap: " << gTech.deviceRoadmap << "\n";
		std::cout << "SRAM Cell Peripheral Node: " << gTech.featureSizeInNano << "nm\n";
		std::cout << "SRAM Cell VDD: " << gTech.vdd.value() << "V\n";
		std::cout << "Temperature: " << gCell.temperature << "K\n";
	} else if (memCellType == DRAM || memCellType == eDRAM) {
		std::cout << "DRAM Cell Access Transistor Width: " << widthAccessCMOS << "F\n";
		std::cout << "DRAM Cell Peripheral Roadmap: " << gTech.deviceRoadmap << "\n";
		std::cout << "DRAM Cell Peripheral Node: " << gTech.featureSizeInNano << "nm\n";
		std::cout << "DRAM Cell VDD: " << gTech.vdd.value() << "V\n";
		std::cout << "DRAM Cell WL_SWING: " << gTech.vpp.value() << "V\n";
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
		std::cout << "3T DRAM Cell VDD: " << gTech.vdd.value() << "V\n";
		std::cout << "3T DRAM Cell WWL_SWING: " << gTechW.vpp.value() << "V\n";
		std::cout << "Temperature: " << gCell.temperature << "K\n";
	} else if (memCellType == SLCNAND) {
		std::cout << "Pass Voltage       : " << flashPassVoltage << "V\n";
		std::cout << "Programming Voltage: " << flashProgramVoltage << "V\n";
		std::cout << "Erase Voltage      : " << flashEraseVoltage << "V\n";
		std::cout << "Programming Time   : " << Second(flashProgramTime) << "\n";
		std::cout << "Erase Time         : " << Second(flashEraseTime) << "\n";
		std::cout << "Gate Coupling Ratio: " << gateCouplingRatio << "\n";
	} 
	if (memCellType == MLCCTT || memCellType == MLCFeFET || memCellType == MLCRRAM) {
			std::cout << "Number of Input Fingers: " << nFingers << "\n";
			std::cout << "Number of Levels per Cell: " << nLvl << "\n";
	}
}
