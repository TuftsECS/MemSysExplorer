#ifndef MSE_CELL_SLCNAND_HPP
#define MSE_CELL_SLCNAND_HPP

#include "cell/MemoryCell.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "constant.hpp"
#include "physics/Horowitz.hpp"

#include <cmath>

namespace mse::cell {

struct SlcNandParameters final : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;

    ScalarParameter<unit::Number> gateCouplingRatio{"Flash.GateCouplingRatio", "", 0.0}; // Ratio of control gate to total floating gate capacitance
    ScalarParameter<unit::Volt> eraseVoltage{"Flash.EraseVoltage", "", unit::Volt(0)};
    ScalarParameter<unit::Volt> passVoltage{"Flash.PassVoltage", "", unit::Volt(0)}; // Applied to unselected wordlines within the block during programming
    ScalarParameter<unit::Volt> programVoltage{"Flash.ProgramVoltage", "", unit::Volt(0)};
    ScalarParameter<unit::Second> eraseTime{"Flash.EraseTime", "", unit::Second(0)};
    ScalarParameter<unit::Second> programTime{"Flash.ProgramTime", "", unit::Second(0)};

    unit::Feature cellNmosGateWidth;
    unit::Feature transistorRegionHeight;

    int pageCount; // Pages in a block, which is the number of cells in a string
    int stringLength; // Cells in a string plus two select transistors

    SlcNandParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech) {
        using namespace mse::unit;

        fillParameter(gateCouplingRatio, eraseVoltage, passVoltage, programVoltage, eraseTime, programTime);

        cellNmosGateWidth = (gTech.featureSizeInNano <= 14) ? 2_feat : 1_feat; // Minimum size transistor
        transistorRegionHeight = cellWidth;

        pageCount = gInputParameter.flashBlockSize / gInputParameter.pageSize;
        stringLength = pageCount + 2;
    }
};

struct SlcNandProperties final : MemoryCellProperties {
    SlcNandProperties(const SlcNandParameters& parameters) : MemoryCellProperties(parameters) {
        using namespace mse::unit;
        operations = {Operation::Read, Operation::Write, Operation::Set, Operation::Reset};

        // Suppose the reference voltage is 0.5 vdd and the bitline is precharged to 0.6 vdd
        // If the bitline drops to 0.4 vdd, the sense amp can tell which data is stored
        sensingMethod = SenseMethod::Voltage;
        prechargeVoltage = gTech.vdd * 0.6;

        sharedContact = true;
        columnLeak = false;
        senseAmpNeedsBitlineMux = true;
        flash = true;
        // A subarray holds at least one block
        minimumRows = parameters.pageCount;
    }
};

class SlcNandTopology final : public MemoryCellTopology {
public:
    NmosTransistor cellTransistor;

    // The control gate is in series with the floating gate, so the wordline sees C_control | C_floating
    CellLineAdapter<unit::Farad> controlGateAdapter;
    // Each string only has one contact to the bitline, so a cell adds its share of that contact
    CellLineAdapter<unit::Farad> stringContactAdapter;

    WordlineList<1> wordlineList = {{
        {controlGateAdapter}
    }};

    BitlineList<1> bitlineList = {{
        {stringContactAdapter}
    }};

    WordlineListSpan wordlineControlledDevices() const noexcept override {
        return wordlineList;
    }

    BitlineListSpan bitlineAccessDevices() const noexcept override {
        return bitlineList;
    }

    // The whole string, including select transistors, pulls the bitline down in series
    unit::Ohm bitlinePullDownResistance() const noexcept {
        return cellTransistor.channelResistance() * static_cast<double>(_stringLength);
    }

    unit::Number beta() const noexcept {
        return 1.0 / (bitlinePullDownResistance() * cellTransistor.transconductance());
    }

    unit::Ampere leakageCurrent() const noexcept override {
        return unit::Ampere(0); // Assume NAND flash cells do not leak
    }

    SlcNandTopology(const SlcNandParameters& parameters)
            : cellTransistor(parameters.tech, parameters.cellNmosGateWidth, parameters.transistorRegionHeight),
              controlGateAdapter(cellTransistor.gateCapacitance() * parameters.gateCouplingRatio / (parameters.gateCouplingRatio + 1)),
              stringContactAdapter(cellTransistor.drainCapacitance() / static_cast<double>(parameters.pageCount)),
              _stringLength(parameters.stringLength) {}

private:

    int _stringLength;
};

class SlcNandCell final : public MemoryCell {
public:
    // Wordlines in a NAND block are driven to pass, program, and erase voltages instead of vdd
    // So the row decoder energies are recalculated here and replace the ones found by the row decoder
    using Parameters = SlcNandParameters;

private:

    Parameters _parameters;
    SlcNandTopology _topology;
    SlcNandProperties _properties;

public:

    SlcNandCell(const Parameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters) {}

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }
    const MemoryCellParameters& parameters() const noexcept override { return _parameters; }

    static constexpr std::string_view name = "SLCNAND";

    // With a 0.5 vdd reference and a 0.6 vdd initial bitline, the sense amp can tell the data apart once the bitline drops to 0.4 vdd
    unit::Volt senseVoltage(unit::Farad) const noexcept override {
        return std::max(_parameters.minSenseVoltage.value(), 0.2 * gTech.vdd);
    }

    // Two select transistors including contacts are 5F long in total for each string
    unit::Meter bitlineOverhead(long long rows) const noexcept override {
        using namespace mse::unit;

        return (rows / _parameters.pageCount) * 5_feat * gTech.featureSize;
    }

    CellTiming timing(const MemoryCellLatencyContext& context) const noexcept override {
        using namespace mse::unit;

        Volt prechargeVoltage = _properties.prechargeVoltage;

        Second tau
            = _topology.bitlinePullDownResistance() * (_topology.cellTransistor.drainCapacitance() + context.bitlineCapacitance + context.bitlineMuxCapacitance)
            + context.bitlineResistance * (context.bitlineMuxCapacitance + context.bitlineCapacitance / 2.0);

        tau *= std::log(prechargeVoltage / (prechargeVoltage - context.senseVoltage));
        // in one case the bitline is unchanged, and in the other case the bitline drops from 0.6 vdd to 0.4 vdd

        CellTiming timing;

        Second bitlineDelay = physics::horowitz(tau, _topology.beta(), context.rowDecoderRampOutput).delay;
        bitlineDelay = std::max(bitlineDelay, tau * 20.0); // TODO revisit, corrects unnecessary horowitz calculation
        timing.bitline = bitlineDelay;
        timing.latency[Operation::Read] = bitlineDelay + context.bitlineMuxReadLatency;
        // Erase is reset and programming is set, programming is used as the write latency
        timing.latency[Operation::Reset] = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency + context.chargeLatency) + _parameters.eraseTime.value();
        timing.latency[Operation::Set] = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency + context.chargeLatency) + _parameters.programTime.value();
        timing.latency[Operation::Write] = timing.latency[Operation::Set];

        return timing;
    }

    CellPower power(const MemoryCellEnergyContext& context) const noexcept override {
        const Technology& tech = _parameters.tech;
        double vdd = tech.vdd.value();
        double featureSize = tech.featureSize.value();
        int pageCount = _parameters.pageCount;
        int stringLength = _parameters.stringLength;
        double passVoltage = _parameters.passVoltage.value().value();
        double programVoltage = _parameters.programVoltage.value().value();
        double eraseVoltage = _parameters.eraseVoltage.value().value();

        double capLine = _topology.cellTransistor.drainCapacitance().value() + context.bitlineCapacitance.value() + context.bitlineMuxPreviousPowerCapacitance.value();
        double rowDecoderEnergy = context.rowDecoderReadDynamic.value();

        CellPower power;
        OperationValues<unit::Joule> wordlineEnergy;

        // Read, only the selected bitline is charged to the precharge voltage
        double voltagePrecharge = _properties.prechargeVoltage.value();
        power.dynamicEnergy[Operation::Read] = unit::Joule(capLine * voltagePrecharge * voltagePrecharge * context.numColumn);
        // Unselected wordlines in the block, and both select lines, are charged to the pass voltage
        // The selected wordline itself is not charged
        double wordlineReadEnergy = rowDecoderEnergy / vdd / vdd * passVoltage * passVoltage;
        wordlineReadEnergy = wordlineReadEnergy * (context.numRow / pageCount * stringLength - 1);
        wordlineEnergy[Operation::Read] = unit::Joule(wordlineReadEnergy);

        // Programming (set), the source line is charged to the program voltage as part of the bitline
        double setEnergy = capLine * programVoltage * programVoltage * context.numColumn;
        // Tunneling current, all unselected bitlines are precharged too so every column is counted
        setEnergy += constant::floatingGateThresholdDelta.value() * constant::tunnelCurrentFlow.value() * _parameters.area.value() * featureSize * featureSize * _parameters.programTime.value().value() * context.numColumn;
        power.dynamicEnergy[Operation::Set] = unit::Joule(setEnergy);
        // The string select line is precharged to vdd, which the row decoder already found
        double wordlineSetEnergy = rowDecoderEnergy;
        // Unselected wordlines in the block are charged to the pass voltage
        wordlineSetEnergy += rowDecoderEnergy / vdd / vdd * passVoltage * passVoltage * (context.numRow / pageCount * stringLength - 1);
        // And the selected wordline is charged to the program voltage
        wordlineSetEnergy += rowDecoderEnergy / vdd / vdd * programVoltage * programVoltage;
        wordlineEnergy[Operation::Set] = unit::Joule(wordlineSetEnergy);

        // Erase (reset), all bitlines and the source line are charged to the erase voltage minus the built-in potential
        double bitlineEraseVoltage = eraseVoltage - tech.buildInPotential.value();
        double resetEnergy = capLine * bitlineEraseVoltage * bitlineEraseVoltage;
        resetEnergy *= (context.numColumn + 1); // Plus 1 for the source line
        // The p-well shared by the selected block is charged to the erase voltage
        double wellJunctionCapacitance = tech.capJunction.value() * _parameters.area.value() * featureSize * featureSize;
        wellJunctionCapacitance *= gInputParameter.flashBlockSize; // One block shares the same well
        resetEnergy += wellJunctionCapacitance * eraseVoltage * eraseVoltage;
        power.dynamicEnergy[Operation::Reset] = unit::Joule(resetEnergy);
        // All wordlines and select lines in unselected blocks, and select lines in the selected block, are charged to beta * erase voltage
        double beta = 0.8;
        double wordlineResetEnergy = rowDecoderEnergy / vdd / vdd * (eraseVoltage * beta) * (eraseVoltage * beta);
        wordlineResetEnergy *= (context.numRow / pageCount * stringLength - pageCount);
        wordlineEnergy[Operation::Reset] = unit::Joule(wordlineResetEnergy);

        // Write energy is the average energy per page
        wordlineEnergy[Operation::Write] = (wordlineEnergy[Operation::Set] + wordlineEnergy[Operation::Reset] / static_cast<double>(pageCount)) / 2.0;
        power.dynamicEnergy[Operation::Write] = (power.dynamicEnergy[Operation::Set] + power.dynamicEnergy[Operation::Reset] / static_cast<double>(pageCount)) / 2.0;

        power.rowDecoderEnergy = wordlineEnergy;


        return power;
    }

    void printSummary(std::ostream& stream) const override {
        stream << "Pass Voltage       : " << _parameters.passVoltage.value().value() << "V\n";
        stream << "Programming Voltage: " << _parameters.programVoltage.value().value() << "V\n";
        stream << "Erase Voltage      : " << _parameters.eraseVoltage.value().value() << "V\n";
        stream << "Programming Time   : " << _parameters.programTime.value() << "\n";
        stream << "Erase Time         : " << _parameters.eraseTime.value() << "\n";
        stream << "Gate Coupling Ratio: " << _parameters.gateCouplingRatio.value() << "\n";
    }

    void writeSummary(YAML::Node node, [[maybe_unused]] bool cacheResult) const override {
        node["PassVoltage_V"] = _parameters.passVoltage.value().value();
        node["ProgramVoltage_V"] = _parameters.programVoltage.value().value();
        node["EraseVoltage_V"] = _parameters.eraseVoltage.value().value();
        node["ProgramTime_s"] = _parameters.programTime.value().value() / 1e9;
        node["EraseTime_s"] = _parameters.eraseTime.value().value() / 1e9;
        node["GateCouplingRatio"] = _parameters.gateCouplingRatio.value();
    }
};

} // namespace mse::cell

#endif
