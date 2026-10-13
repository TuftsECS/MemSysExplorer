#ifndef MSE_CELL_FEFET_HPP
#define MSE_CELL_FEFET_HPP

#include "cell/MemoryCell.hpp"
#include "cell/Resistive.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "formula.hpp"

namespace mse::cell {

struct FefetParameters final : ResistiveCellParameters {
    FefetParameters(YamlInputFile& file) : ResistiveCellParameters(file) {
        // Without an access device, the cell resistance while switching is taken at the write voltage
        if (accessType == none_access) {
            fillWriteEnergy(resistanceOnAtResetVoltage, resistanceOnAtSetVoltage);
        } else {
            fillWriteEnergy(resistanceOn, resistanceOn);
        }
    }
};

class FefetTopology final : public ResistiveAccessTopology {
public:
    FefetTopology(const FefetParameters& parameters)
            : ResistiveAccessTopology(parameters, ferroelectricGateCapacitance(parameters)) {}

private:

    // The ferroelectric gate of the access transistor is the wordline load
    static unit::Farad ferroelectricGateCapacitance(const FefetParameters& parameters) {
        using namespace mse::unit;

        const Technology& tech = parameters.tech;
        const Meter gateWidth = parameters.accessNmosGateWidth.value() * tech.featureSize;

        return Farad(CalculateFeFETGateCap(gateWidth.value(), tech));
    }
};

struct FefetProperties final : ResistiveCellProperties {
    FefetProperties(const FefetParameters& parameters, const FefetTopology& topology) : ResistiveCellProperties(parameters, topology) {}
};

class FefetCell final : public MemoryCell {
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

    using Parameters = FefetParameters;

private:

    Parameters _parameters;
    FefetTopology _topology;
    FefetProperties _properties;

public:

    FefetCell(const Parameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters, _topology) {}

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }
    const MemoryCellParameters& parameters() const noexcept override { return _parameters; }

    static constexpr std::string_view name = "FeFET";

    Latency latency(MemoryCellLatencyContext context) const {
        ResistiveBitlineDelay bitlineDelay = resistiveBitlineDelay(_parameters, _topology, _properties, context);
        ResistiveWriteLatency writeLatency = accessDeviceWriteLatency(_parameters, context);

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
        ResistiveWriteEnergy writeEnergy = accessDeviceWriteEnergy(_parameters, _topology, context);

        Energy energy;

        energy.readDynamic = resistiveReadDynamicEnergy(_parameters, _topology, _properties, context);
        energy.cellRead = resistiveCellReadEnergy(_parameters, context);
        energy.writeDynamic = writeEnergy.write;
        energy.cellSet = writeEnergy.set;
        energy.cellReset = writeEnergy.reset;

        return energy;
    }
};

} // namespace mse::cell

#endif
