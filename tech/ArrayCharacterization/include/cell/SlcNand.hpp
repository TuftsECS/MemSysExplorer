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
    SlcNandProperties(const SlcNandParameters& parameters) {
        using namespace mse::unit;

        // Suppose the reference voltage is 0.5 vdd and the bitline is precharged to 0.6 vdd
        // If the bitline drops to 0.4 vdd, the sense amp can tell which data is stored
        sensingMethod = SenseMethod::Voltage;
        sensingVoltage = std::max(parameters.minSenseVoltage.value(), 0.2 * gTech.vdd);
        prechargeVoltage = gTech.vdd * 0.6;

        sharedContact = true;
        isMLC = false;
        columnLeak = false;
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
    struct Latency {
        unit::Second bitline; // TODO remove later, just here for legacy compat
        unit::Second read;
        unit::Second write;
        unit::Second set;
        unit::Second reset;
    };

    // Wordlines in a NAND block are driven to pass, program, and erase voltages instead of vdd
    // So the row decoder energies are recalculated here and replace the ones found by the row decoder
    struct Energy {
        unit::Joule readDynamic;
        unit::Joule writeDynamic;
        unit::Joule setDynamic;
        unit::Joule resetDynamic;
        unit::Joule rowDecoderReadDynamic;
        unit::Joule rowDecoderWriteDynamic;
        unit::Joule rowDecoderSetDynamic;
        unit::Joule rowDecoderResetDynamic;
    };

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

    Latency latency(MemoryCellLatencyContext context) const {
        using namespace mse::unit;

        Volt prechargeVoltage = _properties.prechargeVoltage;

        Second tau
            = _topology.bitlinePullDownResistance() * (_topology.cellTransistor.drainCapacitance() + context.bitlineCapacitance + context.bitlineMuxCapacitance)
            + context.bitlineResistance * (context.bitlineMuxCapacitance + context.bitlineCapacitance / 2.0);

        tau *= std::log(prechargeVoltage / (prechargeVoltage - context.senseVoltage));
        // in one case the bitline is unchanged, and in the other case the bitline drops from 0.6 vdd to 0.4 vdd

        Latency latency;

        Second bitlineDelay = physics::horowitz(tau, _topology.beta(), context.rowDecoderRampOutput).delay;
        bitlineDelay = std::max(bitlineDelay, tau * 20.0); // TODO revisit, corrects unnecessary horowitz calculation
        latency.bitline = bitlineDelay;
        latency.read = bitlineDelay + context.bitlineMuxReadLatency;
        // Erase is reset and programming is set, programming is used as the write latency
        latency.reset = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency + context.chargeLatency) + _parameters.eraseTime.value();
        latency.set = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency + context.chargeLatency) + _parameters.programTime.value();
        latency.write = latency.set;

        return latency;
    }

    Energy energy(MemoryCellEnergyContext context) const {
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

        Energy energy;

        // Read, only the selected bitline is charged to the precharge voltage
        double voltagePrecharge = _properties.prechargeVoltage.value();
        energy.readDynamic = unit::Joule(capLine * voltagePrecharge * voltagePrecharge * context.numColumn);
        // Unselected wordlines in the block, and both select lines, are charged to the pass voltage
        // The selected wordline itself is not charged
        double wordlineReadEnergy = rowDecoderEnergy / vdd / vdd * passVoltage * passVoltage;
        wordlineReadEnergy = wordlineReadEnergy * (context.numRow / pageCount * stringLength - 1);
        energy.rowDecoderReadDynamic = unit::Joule(wordlineReadEnergy);

        // Programming (set), the source line is charged to the program voltage as part of the bitline
        double setEnergy = capLine * programVoltage * programVoltage * context.numColumn;
        // Tunneling current, all unselected bitlines are precharged too so every column is counted
        setEnergy += constant::floatingGateThresholdDelta.value() * constant::tunnelCurrentFlow.value() * _parameters.area.value() * featureSize * featureSize * _parameters.programTime.value().value() * context.numColumn;
        energy.setDynamic = unit::Joule(setEnergy);
        // The string select line is precharged to vdd, which the row decoder already found
        double wordlineSetEnergy = rowDecoderEnergy;
        // Unselected wordlines in the block are charged to the pass voltage
        wordlineSetEnergy += rowDecoderEnergy / vdd / vdd * passVoltage * passVoltage * (context.numRow / pageCount * stringLength - 1);
        // And the selected wordline is charged to the program voltage
        wordlineSetEnergy += rowDecoderEnergy / vdd / vdd * programVoltage * programVoltage;
        energy.rowDecoderSetDynamic = unit::Joule(wordlineSetEnergy);

        // Erase (reset), all bitlines and the source line are charged to the erase voltage minus the built-in potential
        double bitlineEraseVoltage = eraseVoltage - tech.buildInPotential.value();
        double resetEnergy = capLine * bitlineEraseVoltage * bitlineEraseVoltage;
        resetEnergy *= (context.numColumn + 1); // Plus 1 for the source line
        // The p-well shared by the selected block is charged to the erase voltage
        double wellJunctionCapacitance = tech.capJunction.value() * _parameters.area.value() * featureSize * featureSize;
        wellJunctionCapacitance *= gInputParameter.flashBlockSize; // One block shares the same well
        resetEnergy += wellJunctionCapacitance * eraseVoltage * eraseVoltage;
        energy.resetDynamic = unit::Joule(resetEnergy);
        // All wordlines and select lines in unselected blocks, and select lines in the selected block, are charged to beta * erase voltage
        double beta = 0.8;
        double wordlineResetEnergy = rowDecoderEnergy / vdd / vdd * (eraseVoltage * beta) * (eraseVoltage * beta);
        wordlineResetEnergy *= (context.numRow / pageCount * stringLength - pageCount);
        energy.rowDecoderResetDynamic = unit::Joule(wordlineResetEnergy);

        // Write energy is the average energy per page
        energy.rowDecoderWriteDynamic = (energy.rowDecoderSetDynamic + energy.rowDecoderResetDynamic / static_cast<double>(pageCount)) / 2.0;
        energy.writeDynamic = (energy.setDynamic + energy.resetDynamic / static_cast<double>(pageCount)) / 2.0;

        return energy;
    }
};

} // namespace mse::cell

#endif
