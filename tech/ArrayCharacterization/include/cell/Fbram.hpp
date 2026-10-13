#ifndef MSE_CELL_FBRAM_HPP
#define MSE_CELL_FBRAM_HPP

#include "cell/MemoryCell.hpp"
#include "cell/Resistive.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "constant.hpp"
#include "formula.hpp"

namespace mse::cell {

struct FbramParameters final : ResistiveCellParameters {
    ScalarParameter<unit::Feature> soiDeviceWidth{"FloatingBody.Width", "", unit::Feature(0)};
    ScalarParameter<unit::Number> gateOxideThicknessFactor{"FloatingBody.GateOxideThicknessFactor", "", 2.0}; // Thicker than a usual SOI device

    FbramParameters(YamlInputFile& file) : ResistiveCellParameters(file) {
        using namespace mse::unit;

        fillParameter(soiDeviceWidth, gateOxideThicknessFactor);
        soiDeviceWidth.value() *= ((gTech.featureSizeInNano <= 14) ? 2.0 : 1.0);

        // The floating body device is driven with both the write voltage and current
        if (resetEnergy.value() == Joule(0)) {
            resetEnergy.value() = resetMode
                ? Joule(std::fabs(resetVoltage.value().value()) * std::fabs(resetCurrent.value().value()) * resetPulse.value().value())
                : currentPulseEnergy(resetVoltage.value() == Volt(0), resetVoltage, resetCurrent, resetPulse);
        }
        if (setEnergy.value() == Joule(0)) {
            setEnergy.value() = setMode
                ? Joule(std::fabs(setVoltage.value().value()) * std::fabs(setCurrent.value().value()) * setPulse.value().value())
                : currentPulseEnergy(resetVoltage.value() == Volt(0), setVoltage, setCurrent, setPulse);
        }
    }
};

// The floating body SOI device is both the storage element and access device
// Data is read through its on/off resistance, so no access resistance is added
class FbramTopology final : public ResistiveCellTopology {
public:
    CellLineAdapter<unit::Farad> gateAdapter;
    CellLineAdapter<unit::Farad> drainAdapter;

    WordlineList<1> wordlineList = {{
        {gateAdapter}
    }};

    BitlineList<1> bitlineList = {{
        {drainAdapter}
    }};

    WordlineListSpan wordlineControlledDevices() const noexcept override {
        return wordlineList;
    }

    BitlineListSpan bitlineAccessDevices() const noexcept override {
        return bitlineList;
    }

    unit::Ohm accessResistance() const noexcept override {
        return unit::Ohm(0);
    }

    FbramTopology(const FbramParameters& parameters)
            : ResistiveCellTopology(parameters),
              gateAdapter(unit::Farad(CalculateFBRAMGateCap(soiWidth(parameters), parameters.gateOxideThicknessFactor, parameters.tech))),
              drainAdapter(unit::Farad(CalculateFBRAMDrainCap(soiWidth(parameters), parameters.tech))) {}

private:

    static double soiWidth(const FbramParameters& parameters) {
        using namespace mse::unit;

        return (parameters.soiDeviceWidth.value() * parameters.tech.get().featureSize).value();
    }
};

struct FbramProperties final : ResistiveCellProperties {
    FbramProperties(const FbramParameters& parameters, const FbramTopology& topology) : ResistiveCellProperties(parameters, topology) {
        sharedContact = true;
    }
};

class FbramCell final : public MemoryCell {
public:
    struct Latency {
        unit::Second bitline; // TODO remove later, just here for legacy compat
        unit::Second bitlineOn; // TODO remove later, just here for legacy compat
        unit::Second bitlineOff; // TODO remove later, just here for legacy compat
        unit::Second read;
        unit::Second write;
        unit::Second set;
        unit::Second reset;
    };

    struct Energy {
        unit::Joule readDynamic;
        unit::Joule writeDynamic;
        unit::Joule cellRead;
        unit::Joule cellSet;
        unit::Joule cellReset;
    };

    using Parameters = FbramParameters;

private:

    Parameters _parameters;
    FbramTopology _topology;
    FbramProperties _properties;

public:

    FbramCell(const Parameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters, _topology) {}

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }
    const MemoryCellParameters& parameters() const noexcept override { return _parameters; }

    static constexpr std::string_view name = "FBRAM";

    Latency latency(MemoryCellLatencyContext context) const {
        ResistiveBitlineDelay bitlineDelay = resistiveBitlineDelay(_parameters, _topology, _properties, context);
        ResistiveWriteLatency writeLatency = pulsedWriteLatency(_parameters, context);

        Latency latency;

        latency.bitline = bitlineDelay.bitline;
        latency.bitlineOn = bitlineDelay.bitlineOn;
        latency.bitlineOff = bitlineDelay.bitlineOff;
        latency.read = bitlineDelay.bitline + context.bitlineMuxReadLatency;
        latency.write = writeLatency.write;
        latency.set = writeLatency.set;
        latency.reset = writeLatency.reset;

        return latency;
    }

    Energy energy(MemoryCellEnergyContext context) const {
        ResistiveWriteEnergy writeEnergy = resistiveCellWriteEnergy(_parameters, _topology, context);

        Energy energy;

        energy.readDynamic = resistiveReadDynamicEnergy(_parameters, _topology, _properties, context);
        energy.cellRead = resistiveCellReadEnergy(_parameters, context);
        // Due to the shaper inefficiency
        energy.cellReset = writeEnergy.reset / constant::shaperEfficiencyAggressive;
        energy.cellSet = writeEnergy.set / constant::shaperEfficiencyAggressive;
        energy.writeDynamic = std::max(energy.cellReset, energy.cellSet);

        return energy;
    }
};

} // namespace mse::cell

#endif
