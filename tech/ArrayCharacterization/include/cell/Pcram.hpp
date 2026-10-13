#ifndef MSE_CELL_PCRAM_HPP
#define MSE_CELL_PCRAM_HPP

#include "cell/MemoryCell.hpp"
#include "cell/Resistive.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "constant.hpp"

namespace mse::cell {

struct PcramParameters final : ResistiveCellParameters {
    PcramParameters(YamlInputFile& file) : ResistiveCellParameters(file) {
        // PCM cells show low resistance during most of the switching time
        fillWriteEnergy(resistanceOn, resistanceOn);
    }
};

class PcramTopology final : public ResistiveAccessTopology {
public:
    PcramTopology(const PcramParameters& parameters)
            : ResistiveAccessTopology(parameters) {}
};

struct PcramProperties final : ResistiveCellProperties {
    PcramProperties(const PcramParameters& parameters, const PcramTopology& topology) : ResistiveCellProperties(parameters, topology) {}
};

class PcramCell final : public MemoryCell {
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

    using Parameters = PcramParameters;

private:

    Parameters _parameters;
    PcramTopology _topology;
    PcramProperties _properties;

public:

    PcramCell(const Parameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters, _topology) {}

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }
    const MemoryCellParameters& parameters() const noexcept override { return _parameters; }

    static constexpr std::string_view name = "PCRAM";

    Latency latency(MemoryCellLatencyContext context) const {
        ResistiveBitlineDelay bitlineDelay = resistiveBitlineDelay(_parameters, _topology, _properties, context);

        Latency latency;

        latency.bitline = bitlineDelay.bitline;
        latency.bitlineOn = bitlineDelay.bitlineOn;
        latency.bitlineOff = bitlineDelay.bitlineOff;
        latency.read = bitlineDelay.bitline + context.bitlineMuxReadLatency;

        if (gInputParameter.writeScheme != write_and_verify) { // TODO write and verify programming
            ResistiveWriteLatency writeLatency = pulsedWriteLatency(_parameters, context);
            latency.write = writeLatency.write;
            latency.set = writeLatency.set;
            latency.reset = writeLatency.reset;
        }

        return latency;
    }

    Energy energy(MemoryCellEnergyContext context) const {
        Energy energy;

        energy.readDynamic = resistiveReadDynamicEnergy(_parameters, _topology, _properties, context);
        energy.cellRead = resistiveCellReadEnergy(_parameters, context);

        if (gInputParameter.writeScheme != write_and_verify) { // TODO write and verify programming
            ResistiveWriteEnergy writeEnergy = resistiveCellWriteEnergy(_parameters, _topology, context);
            // Due to the shaper inefficiency
            energy.cellReset = writeEnergy.reset / constant::shaperEfficiencyConservative;
            energy.cellSet = writeEnergy.set / constant::shaperEfficiencyConservative;
            energy.writeDynamic = std::max(energy.cellReset, energy.cellSet);
        }

        return energy;
    }
};

} // namespace mse::cell

#endif
