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
#include "cell/types.hpp"

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
        // The run's cell is created from the type, which is read here only so strict access doesn't flag it
        std::string type;
        fill(std::type_identity<std::string>{}, "MemCellType", type);
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
        fill(std::type_identity<Farad>{}, "Storage.Capacitance", capDRAMCell);
        // The DRAM cells read their retention time themselves
        double retention = 0;
        fill(std::type_identity<Second>{}, "Storage.RetentionTime", retention);

        // SRAM specific
        fill(std::type_identity<Feature>{}, "Inverter.NmosWidth", widthSRAMCellNMOS);
        fill(std::type_identity<Feature>{}, "Inverter.PmosWidth", widthSRAMCellPMOS);

        // Flash specific
        fill(std::type_identity<Volt>{}, "Flash.EraseVoltage", flashEraseVoltage);
        fill(std::type_identity<Volt>{}, "Flash.ProgramVoltage", flashProgramVoltage);
        fill(std::type_identity<Volt>{}, "Flash.PassVoltage", flashPassVoltage);
        fill(std::type_identity<Second>{}, "Flash.EraseTime", flashEraseTime);
        fill(std::type_identity<Second>{}, "Flash.ProgramTime", flashProgramTime);
        fill(number, "Flash.GateCouplingRatio", gateCouplingRatio);

        // The multi-level cells read their levels and sense amp fingers themselves
        int fingers = 0;
        fill(std::type_identity<int>{}, "MultiLevel.SenseAmpFingers", fingers);
        double levels = 0;
        fill(number, "MultiLevel.Levels", levels);

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
}


void MemCell::PrintCell() {
    const mse::cell::MemoryCell& cell = mse::cell::memoryCell();

    std::cout << "Memory Cell: " << mse::cell::memoryCellName() << "\n";

	std::cout << "Cell Area (F^2)    : " << area << " (" << heightInFeatureSize << "Fx" << widthInFeatureSize << "F)\n";
	std::cout << "Cell Area (um^2)    : " << area / 1000000.0 * gTech.featureSizeInNano * gTech.featureSizeInNano << " (" << heightInFeatureSize * gTech.featureSizeInNano << "nm x" << widthInFeatureSize * gTech.featureSizeInNano << "nm y)\n";
	std::cout << "Cell Aspect Ratio  : " << aspectRatio << "\n";

    cell.printSummary(std::cout);
    if (const std::optional<mse::cell::MultiLevel>& multiLevel = cell.properties().multiLevel) {
        std::cout << "Number of Input Fingers: " << multiLevel->senseAmpFingers << "\n";
        std::cout << "Number of Levels per Cell: " << multiLevel->levels << "\n";
    }
}
