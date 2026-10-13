#ifndef MSE_CELL_MEMRISTOR_HPP
#define MSE_CELL_MEMRISTOR_HPP

#include "cell/MemoryCell.hpp"
#include "cell/Resistive.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"

namespace mse::cell {

struct MemristorParameters final : ResistiveCellParameters {
    MemristorParameters(YamlInputFile& file) : ResistiveCellParameters(file) {
        // Without an access device, the cell resistance while switching is taken at the write voltage
        if (accessType == none_access) {
            fillWriteEnergy(resistanceOnAtResetVoltage, resistanceOnAtSetVoltage);
        } else {
            fillWriteEnergy(resistanceOn, resistanceOn);
        }
    }
};

class MemristorTopology final : public ResistiveAccessTopology {
public:
    MemristorTopology(const MemristorParameters& parameters)
            : ResistiveAccessTopology(parameters) {}
};

struct MemristorProperties final : ResistiveCellProperties {
    MemristorProperties(const MemristorParameters& parameters, const MemristorTopology& topology) : ResistiveCellProperties(parameters, topology) {}
};

class MemristorCell final : public MemoryCell {
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

    using Parameters = MemristorParameters;

private:

    Parameters _parameters;
    MemristorTopology _topology;
    MemristorProperties _properties;

public:

    MemristorCell(const Parameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters, _topology) {}

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }
    const MemoryCellParameters& parameters() const noexcept override { return _parameters; }

    static constexpr std::string_view name = "memristor";

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
