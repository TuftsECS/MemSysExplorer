#ifndef MSE_CELL_CTT_HPP
#define MSE_CELL_CTT_HPP

#include "cell/MemoryCell.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "physics/Horowitz.hpp"

#include <cmath>

namespace mse::cell {

struct CttParameters final : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;

    unit::Feature cellNmosGateWidth;
    unit::Feature transistorRegionHeight;

    CttParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech) {
        // TODO use specific cell fields in config file
        using namespace mse::unit;

        cellNmosGateWidth = (gTech.featureSizeInNano <= 14) ? 2_feat : 1_feat; // Minimum size transistor
        transistorRegionHeight = Feature(gCell.widthInFeatureSize);
    }
};

struct CttProperties final : MemoryCellProperties {
    CttProperties(const CttParameters&) {
        using namespace mse::unit;

        sensingMethod = gCell.readMode ? SenseMethod::Voltage : SenseMethod::Current;
        sensingVoltage = Volt(gCell.minSenseVoltage);
        prechargeVoltage = gTech.vdd * 0.5;

        sharedContact = false;
        isMLC = false;
        columnLeak = false;
    }
};

class CttTopology final : public MemoryCellTopology {
public:
    NmosTransistor cellTransistor;

    CellLineAdapter<CmosTransistor> cellTransistorAdapter = cellTransistor;

    WordlineList<1> wordlineList = {{
        {cellTransistorAdapter}
    }};

    BitlineList<1> bitlineList = {{
        {cellTransistorAdapter}
    }};

    WordlineListSpan wordlineControlledDevices() const noexcept override {
        return wordlineList;
    }

    BitlineListSpan bitlineAccessDevices() const noexcept override {
        return bitlineList;
    }

    // The cell transistor pulls the bitline down in series with its access transistor
    unit::Ohm bitlinePullDownResistance() const noexcept {
        return cellTransistor.channelResistance() * 2.0;
    }

    unit::Number beta() const noexcept {
        return 1.0 / (bitlinePullDownResistance() * cellTransistor.transconductance());
    }

    unit::Ampere leakageCurrent() const noexcept override {
        return unit::Ampere(0);
    }

    CttTopology(const CttParameters& parameters)
            : cellTransistor(parameters.tech, parameters.cellNmosGateWidth, parameters.transistorRegionHeight) {}
};

class CttCell final : public MemoryCell {
public:
    struct Latency {
        unit::Second bitline; // TODO remove later, just here for legacy compat
        unit::Second read;
        unit::Second write;
        unit::Second set;
        unit::Second reset;
    };

    struct Energy {
        unit::Joule readDynamic; // TODO no bitline energy model yet
        unit::Joule writeDynamic; // TODO no bitline energy model yet
    };

    using Parameters = CttParameters;

private:

    Parameters _parameters;
    CttTopology _topology;
    CttProperties _properties;

public:

    CttCell(const Parameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters) {}

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }
    const MemoryCellParameters& parameters() const noexcept override { return _parameters; }

    static constexpr std::string_view name = "CTT";

    Latency latency(MemoryCellLatencyContext context) const {
        using namespace mse::unit;

        Volt prechargeVoltage = _properties.prechargeVoltage;

        Second tau
            = _topology.bitlinePullDownResistance() * (_topology.bitlineCapacitance(0) + context.bitlineCapacitance + context.bitlineMuxCapacitance)
            + context.bitlineResistance * (context.bitlineMuxCapacitance + context.bitlineCapacitance / 2.0);

        tau *= std::log(prechargeVoltage / (prechargeVoltage - context.senseVoltage));
        // in one case the bitline is unchanged, and in the other case the bitline drops

        Latency latency;

        Second bitlineDelay = physics::horowitz(tau, _topology.beta(), context.rowDecoderRampOutput).delay;
        bitlineDelay = std::max(bitlineDelay, tau * 20.0); // TODO revisit, corrects unnecessary horowitz calculation
        latency.bitline = bitlineDelay;
        latency.read = bitlineDelay + context.bitlineMuxReadLatency;
        // Erase is reset and programming is set, programming is used as the write latency
        latency.reset = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency + context.chargeLatency);
        latency.set = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency + context.chargeLatency);
        latency.write = latency.set;

        return latency;
    }

    Energy energy(MemoryCellEnergyContext) const {
        Energy energy;
        return energy;
    }
};

} // namespace mse::cell

#endif
