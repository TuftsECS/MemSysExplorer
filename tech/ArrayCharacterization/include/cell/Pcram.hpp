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

class PcramCell final : public ResistiveMemoryCell<PcramParameters, PcramTopology, PcramProperties> {
public:
    using Parameters = PcramParameters;

    using ResistiveMemoryCell::ResistiveMemoryCell;

    static constexpr std::string_view name = "PCRAM";

    CellTiming timing(const MemoryCellLatencyContext& context) const noexcept override {
        ResistiveBitlineDelay bitlineDelay = resistiveBitlineDelay(_parameters, _topology, _properties, context);

        CellTiming timing;

        timing.bitline = bitlineDelay.bitline;
        timing.bitlineOn = bitlineDelay.bitlineOn;
        timing.bitlineOff = bitlineDelay.bitlineOff;
        timing.latency[Operation::Read] = bitlineDelay.bitline + context.bitlineMuxReadLatency;

        if (gInputParameter.writeScheme != write_and_verify) { // TODO write and verify programming
            ResistiveWriteLatency writeLatency = pulsedWriteLatency(_parameters, context);
            timing.latency[Operation::Write] = writeLatency.write;
            timing.latency[Operation::Set] = writeLatency.set;
            timing.latency[Operation::Reset] = writeLatency.reset;
        }

        return timing;
    }

    CellPower power(const MemoryCellEnergyContext& context) const noexcept override {
        CellPower power;

        power.dynamicEnergy[Operation::Read] = resistiveReadDynamicEnergy(_parameters, _topology, _properties, context);
        power.cellEnergy[Operation::Read] = resistiveCellReadEnergy(_parameters, context);

        if (gInputParameter.writeScheme != write_and_verify) { // TODO write and verify programming
            ResistiveWriteEnergy writeEnergy = resistiveCellWriteEnergy(_parameters, _topology, context);
            // Due to the shaper inefficiency
            power.cellEnergy[Operation::Reset] = writeEnergy.reset / constant::shaperEfficiencyConservative;
            power.cellEnergy[Operation::Set] = writeEnergy.set / constant::shaperEfficiencyConservative;
            power.dynamicEnergy[Operation::Write] = std::max(power.cellEnergy[Operation::Reset], power.cellEnergy[Operation::Set]);
        }

        return power;
    }
};

} // namespace mse::cell

#endif
