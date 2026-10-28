// 8T SRAM cell for the MemSysExplorer tutorial
// Each STEP comment says which lines to uncomment, and which 6T placeholder lines to delete.
// The file builds and runs after every step.

#ifndef MSE_CELL_SRAM8T_HPP
#define MSE_CELL_SRAM8T_HPP

#include "cell/MemoryCell.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "physics/Horowitz.hpp"

#include <algorithm>
#include <cmath>

namespace mse::cell {

//==================================================
// Parameters, the values read from the cell file

struct Sram8tParameters final : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;

    ScalarParameter<unit::Feature> inverterNmosGateWidth{"Inverter.NmosWidth", "", unit::Feature(2.31)};
    ScalarParameter<unit::Feature> inverterPmosGateWidth{"Inverter.PmosWidth", "", unit::Feature(3.08)};
    ScalarParameter<unit::Feature> accessNmosGateWidth{"Access.Width", "", unit::Feature(2.77)};
    // STEP 2a. Uncomment the width of the read access transistor, key "Access.ReadWidth", default 3.54 F
    //ScalarParameter<unit::Feature> readNmosGateWidth{"Access.ReadWidth", "", unit::Feature(3.54)};

    unit::Feature transistorRegionHeight;

    Sram8tParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech) {
        fillParameter(inverterNmosGateWidth, inverterPmosGateWidth, accessNmosGateWidth);
        // STEP 2b. Uncomment to read the read width from the cell file
        //fillParameter(readNmosGateWidth);

        // FinFET nodes need twice the width
        unit::Number featureScale = (tech.get().featureSizeInNano <= 14 ? 2 : 1);
        inverterNmosGateWidth.value() *= featureScale;
        inverterPmosGateWidth.value() *= featureScale;
        accessNmosGateWidth.value() *= featureScale;
        // STEP 2c. Uncomment to scale the read width like the other widths
        //readNmosGateWidth.value() *= featureScale;

        transistorRegionHeight = cellWidth;
    }
};

//==================================================
// Properties, what the rest of the array needs to know about the cell

struct Sram8tProperties final : public MemoryCellProperties {
    Sram8tProperties(const Sram8tParameters& parameters) : MemoryCellProperties(parameters) {
        operations = {Operation::Read, Operation::Write};

        sensingMethod = SenseMethod::Voltage;
        // STEP 5. The single-ended read bitline precharges to vdd instead of vdd / 2
        //   Uncomment the 8T line, the 6T placeholder above it can stay or be deleted
        prechargeVoltage = parameters.tech.get().vdd * 0.5; // 6T placeholder
        //prechargeVoltage = parameters.tech.get().vdd;

        sharedContact = true;
        columnLeak = true;

        globalBitline.model = GlobalBitlineModel::Sram;
    }
};

//==================================================
// Topology, the devices in the cell and which lines they sit on

class Sram8tTopology final : public MemoryCellTopology {
public:
    NmosTransistor writeAccess;
    NmosTransistor writeAccessBar;
    // STEP 3a. Uncomment the two read port transistors
    //NmosTransistor readStack;
    //NmosTransistor readAccess;
    CmosInverter inverter;
    CmosInverter inverterBar;

    CellLineAdapter<CmosTransistor> writeAccessAdapter = writeAccess;
    CellLineAdapter<CmosTransistor> writeAccessBarAdapter = writeAccessBar;
    // STEP 3b. Uncomment the adapter, readAccess sits on the read wordline and the read bitline
    //CellLineAdapter<CmosTransistor> readAccessAdapter = readAccess;

    // STEP 3c. Two wordlines and three bitlines, with the read lines first so they are line 0
    //   Delete the 6T placeholder lists and uncomment the 8T lists
    // 6T placeholder
    WordlineList<1> wordlineList = {{
        {writeAccessAdapter, writeAccessBarAdapter}
    }};

    BitlineList<2> bitlineList = {{
        {writeAccessAdapter},
        {writeAccessBarAdapter}
    }};

    //WordlineList<2> wordlineList = {{
    //    {readAccessAdapter}, // Read wordline
    //    {writeAccessAdapter, writeAccessBarAdapter} // Write wordline
    //}};

    //BitlineList<3> bitlineList = {{
    //    {readAccessAdapter}, // Read bitline
    //    {writeAccessAdapter}, // Write bitline
    //    {writeAccessBarAdapter} // Write bitline bar
    //}};

    WordlineListSpan wordlineControlledDevices() const noexcept override {
        return wordlineList;
    }

    BitlineListSpan bitlineAccessDevices() const noexcept override {
        return bitlineList;
    }

    unit::Number beta(int bitline) const noexcept {
        switch (bitline) {
            // STEP 4a. Bitline 0 is now the read bitline, driven through readAccess and readStack
            //   Delete the 6T placeholder case 0 and uncomment the 8T one
            case 0: return 1.0 / (writeAccess.transconductance() * inverter.nmosResistance()); // 6T placeholder
            //case 0: return 1.0 / (readAccess.transconductance() * readStack.channelResistance());
            case 1: return 1.0 / (writeAccess.transconductance() * inverter.nmosResistance());
            case 2: return 1.0 / (writeAccessBar.transconductance() * inverterBar.nmosResistance());
            default: return 0.0;
        }
    }

    // The resistance of the path that pulls each bitline down
    unit::Ohm bitlinePullDownResistance(int bitline) const noexcept {
        using namespace mse::unit;

        switch (bitline) {
            // STEP 4b. Same change as beta, the read bitline goes through readAccess and readStack
            //   Delete the 6T placeholder case 0 and uncomment the 8T one
            case 0: return writeAccess.channelResistance() + inverter.nmosResistance(); // 6T placeholder
            //case 0: return readAccess.channelResistance() + readStack.channelResistance();
            case 1: return writeAccess.channelResistance() + inverter.nmosResistance();
            case 2: return writeAccessBar.channelResistance() + inverterBar.nmosResistance();
            default: return 0.0_Ohm;
        }
    }

    // Each pair has one device on and one off, so half of each pair leaks on average
    unit::Ampere leakageCurrent() const noexcept override {
        unit::Ampere leakage = inverter.leakageCurrent() + inverterBar.leakageCurrent()
            + (writeAccess.leakageCurrent() + writeAccessBar.leakageCurrent()) / 2.0;
        // STEP 4c. Uncomment, the read port leaks too, half of readStack and readAccess like the write pair
        //leakage += (readStack.leakageCurrent() + readAccess.leakageCurrent()) / 2.0;

        return leakage;
    }

    Sram8tTopology(const Sram8tParameters& parameters)
            : writeAccess(parameters.tech, parameters.accessNmosGateWidth, parameters.transistorRegionHeight),
              writeAccessBar(writeAccess),
              // STEP 3d. Uncomment to build the read stack like a write access transistor, and the read access transistor with the read width
              //readStack(writeAccess),
              //readAccess(parameters.tech, parameters.readNmosGateWidth, parameters.transistorRegionHeight),
              inverter(parameters.tech, parameters.inverterNmosGateWidth, parameters.inverterPmosGateWidth, parameters.transistorRegionHeight),
              inverterBar(inverter) {}
};

//==================================================
// The cell, with its name, its three parts, and its latency and energy models

class Sram8tCell final : public MemoryCell {
public:
    using Parameters = Sram8tParameters;

private:

    Parameters _parameters;
    Sram8tTopology _topology;
    Sram8tProperties _properties;

public:

    Sram8tCell(const Parameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters) {}

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }
    const MemoryCellParameters& parameters() const noexcept override { return _parameters; }

    // Has to match MemCellType in the cell file
    static constexpr std::string_view name = "SRAM8T";

    CellTiming timing(const MemoryCellLatencyContext& context) const noexcept override {
        using namespace mse::unit;

        const Volt prechargeVoltage = _properties.prechargeVoltage;
        const Volt sensingVoltage = _parameters.minSenseVoltage;
        const SecondExp<-1> inputRamp = context.rowDecoderRampOutput;
        const Second decoderLatency = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency);

        CellTiming timing;

        // STEP 6a. Model the read and the write on their own bitlines
        //   The read stack discharges the single-ended read bitline (bitline 0) by the full sensing voltage
        //   The write bitlines (bitline 1) are differential like 6T, so half the sensing voltage on each side
        //   Uncomment the 8T model, the 6T placeholder above it can stay or be deleted
        // 6T placeholder, the 6T model on bitline 0 for both the read and the write
        const Ohm resistance = _topology.bitlinePullDownResistance(0);
        Second tau
            = resistance * _topology.bitlineCapacitance(0)
            + resistance * (context.bitlineCapacitance / 2.0)
            + (resistance + context.bitlineResistance) * (context.bitlineCapacitance / 2.0)
            + (resistance + context.bitlineResistance) * context.bitlineMuxCapacitance;
        tau *= std::log(prechargeVoltage / (prechargeVoltage - sensingVoltage / 2.0));

        const Second bitlineDelay = physics::horowitz(tau, _topology.beta(0), inputRamp).delay;

        timing.bitline = bitlineDelay;
        timing.latency[Operation::Read] = bitlineDelay + context.bitlineMuxReadLatency;
        timing.latency[Operation::Write] = decoderLatency + bitlineDelay + context.bitlineMuxReadLatency;

        //const Ohm readResistance = _topology.bitlinePullDownResistance(0);
        //Second readTau
        //    = readResistance * _topology.bitlineCapacitance(0)
        //    + readResistance * (context.bitlineCapacitance / 2.0)
        //    + (readResistance + context.bitlineResistance) * (context.bitlineCapacitance / 2.0)
        //    + (readResistance + context.bitlineResistance) * context.bitlineMuxCapacitance;
        //readTau *= std::log(prechargeVoltage / (prechargeVoltage - sensingVoltage));

        //const Ohm writeResistance = _topology.bitlinePullDownResistance(1);
        //Second writeTau
        //    = writeResistance * _topology.bitlineCapacitance(1)
        //    + writeResistance * (context.bitlineCapacitance / 2.0)
        //    + (writeResistance + context.bitlineResistance) * (context.bitlineCapacitance / 2.0)
        //    + (writeResistance + context.bitlineResistance) * context.bitlineMuxCapacitance;
        //writeTau *= std::log(prechargeVoltage / (prechargeVoltage - sensingVoltage / 2.0));

        //const Second readBitlineDelay = physics::horowitz(readTau, _topology.beta(0), inputRamp).delay;
        //const Second writeBitlineDelay = physics::horowitz(writeTau, _topology.beta(1), inputRamp).delay;

        //timing.bitline = readBitlineDelay;
        //timing.bitlineRead = readBitlineDelay;
        //timing.bitlineWrite = writeBitlineDelay;
        //timing.latency[Operation::Read] = readBitlineDelay + context.bitlineMuxReadLatency;
        //timing.latency[Operation::Write] = decoderLatency + writeBitlineDelay + context.bitlineMuxReadLatency;

        return timing;
    }

    CellPower power(const MemoryCellEnergyContext& context) const noexcept override {
        using namespace mse::unit;

        const Volt prechargeVoltage = _properties.prechargeVoltage;
        const int selectedColumns = context.numColumn / context.muxSenseAmp / context.muxOutputLev1 / context.muxOutputLev2;

        CellPower power;

        // STEP 6b. Give the read and the write their own energy
        //   Every read bitline (bitline 0) drops by the sensing voltage, so C x vdd x sensing voltage
        //   One line of each selected write pair (bitline 1) swings rail to rail, so C x vdd x vdd
        //   Uncomment the 8T model, the 6T placeholder above it can stay or be deleted
        // 6T placeholder, the 6T model on bitline 0 for both the read and the write
        const Farad load = _topology.bitlineCapacitance(0) + context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance;

        power.dynamicEnergy[Operation::Read] = load * prechargeVoltage * prechargeVoltage * context.numColumn;
        power.dynamicEnergy[Operation::Write] = load * prechargeVoltage * prechargeVoltage * selectedColumns;

        //const Volt sensingVoltage = _parameters.minSenseVoltage;
        //const Farad readLoad = _topology.bitlineCapacitance(0) + context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance;
        //const Farad writeLoad = _topology.bitlineCapacitance(1) + context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance;

        //power.dynamicEnergy[Operation::Read] = readLoad * prechargeVoltage * sensingVoltage * context.numColumn;
        //power.dynamicEnergy[Operation::Write] = writeLoad * prechargeVoltage * prechargeVoltage * selectedColumns;

        return power;
    }

    void printSummary(std::ostream& stream) const override {
        stream << "SRAM Cell Access Transistor Width: " << _parameters.accessNmosGateWidth.value().value() << "F\n";
        stream << "SRAM Cell NMOS Width: " << _parameters.inverterNmosGateWidth.value().value() << "F\n";
        stream << "SRAM Cell PMOS Width: " << _parameters.inverterPmosGateWidth.value().value() << "F\n";
        stream << "SRAM Cell Peripheral Roadmap: " << gTech.deviceRoadmap << "\n";
        stream << "SRAM Cell Peripheral Node: " << gTech.featureSizeInNano << "nm\n";
        stream << "SRAM Cell VDD: " << gTech.vdd.value() << "V\n";
        stream << "Temperature: " << double(gInputParameter.temperature) << "K\n";
    }

    void writeSummary(YAML::Node node) const override {
        node["WidthAccessCMOS_F"] = _parameters.accessNmosGateWidth.value().value();
        node["WidthSRAMCellNMOS_F"] = _parameters.inverterNmosGateWidth.value().value();
        node["WidthSRAMCellPMOS_F"] = _parameters.inverterPmosGateWidth.value().value();
        node["PeripheralRoadmap"] = enumToString(gTech.deviceRoadmap);
        node["PeripheralNode_nm"] = gTech.featureSizeInNano;
        node["VDD_V"] = gTech.vdd.value();
        node["WWL_SWING"] = gTech.vdd.value();
        node["Temperature_K"] = double(gInputParameter.temperature);
    }
};

} // namespace mse::cell

#endif
