#ifndef MSE_CELL_EDRAM3T333_HPP
#define MSE_CELL_EDRAM3T333_HPP

#include "cell/MemoryCell.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "physics/Horowitz.hpp"

namespace mse::cell {

struct Edram3t333Parameters final : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;
    std::reference_wrapper<const Technology> techR;
    std::reference_wrapper<const Technology> techW;

    //input::SimpleParameter<unit::Feature> accessNmosGateWidth;
    //input::SimpleParameter<unit::Feature> accessNmosGateWidthR;
    //input::SimpleParameter<unit::Feature> accessNmosGateWidthW;
    ScalarParameter<unit::Farad> internalCapacitance{"Storage Node Capacitance", "", unit::Farad(0)};

    unit::Feature accessNmosGateWidth;
    unit::Feature accessNmosGateWidthR;
    unit::Feature accessNmosGateWidthW;
    unit::Feature transistorRegionHeight;

    Edram3t333Parameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech), techR(gTechR), techW(gTechW) {
        // TODO use specific cell fields in config file
        // for now just steal from gCell, figure out new input style later
        // fillParameter(accessNmosGateWidth, accessNmosGateWidthR, accessNmosGateWidthW);
        //fillParameter(internalCapacitance);
        internalCapacitance.value() = unit::Farad(gCell.capDRAMCell);

        using namespace mse::unit;

        accessNmosGateWidth = ((gTech.featureSizeInNano <= 14) ? 2_feat : 1_feat) * gCell.widthAccessCMOS;
        accessNmosGateWidthR = ((gTechR.featureSizeInNano <= 14) ? 2_feat : 1_feat) * gCell.widthAccessCMOSR;
        accessNmosGateWidthW = ((gTechW.featureSizeInNano <= 14) ? 2_feat : 1_feat) * gCell.widthAccessCMOS;

        transistorRegionHeight = unit::Feature(gCell.widthInFeatureSize);
    }
};

struct Edram3t333Properties final : MemoryCellProperties {
    Edram3t333Properties(const Edram3t333Parameters&) {
        using namespace mse::unit;

        sensingMethod = SenseMethod::Voltage;
        // TODO ok lets move this one out of here it isnt really a user set thing right?
        // super hacky for now tho It Just Works TM
		//sensingVoltage = Volt(gTech.vdd) / 2.0 * (parameters.internalCapacitance / (parameters.internalCapacitance + parameters.capBitline));
        prechargeVoltage = gTech.vdd;

        sharedContact = false;
    }
};

struct Edram3t333Topology final : MemoryCellTopology {
    NmosTransistor accessTransistor;
    NmosTransistor accessTransistorR;
    NmosTransistor accessTransistorW;
    unit::Farad internalCapacitance;

    CellLineAdapter<CmosTransistor> accessTransistorAdapterR{accessTransistorR};
    CellLineAdapter<CmosTransistor> accessTransistorAdapterW{accessTransistorW};

    WordlineList<2> wordlineList = {{
        {accessTransistorAdapterR}, // TODO these are maxd?
        {accessTransistorAdapterW}
    }};

    BitlineList<2> bitlineList = {{
        {accessTransistorAdapterR}, // TODO these are maxd?
        {accessTransistorAdapterW}
    }};
    
    WordlineListSpan wordlineControlledDevices() const noexcept override {
        return wordlineList;
    }

    BitlineListSpan bitlineAccessDevices() const noexcept override {
        return bitlineList;
    }

    unit::Farad storageCapacitance() const noexcept {
        return internalCapacitance;
    };

    unit::Ohm bitlinePullDownResistance(int bitline, [[maybe_unused]] int path = 0) const noexcept {
        using namespace mse::unit;
        switch (bitline) {
            case 0: return accessTransistorR.channelResistance();
            case 1: return accessTransistorW.channelResistance();
            default: return 0.0_Ohm;
        }
    }

    unit::Ampere leakageCurrent() const noexcept override {
        return accessTransistor.leakageCurrent();
    }

    Edram3t333Topology(const Edram3t333Parameters& parameters)
            : accessTransistor(parameters.tech, parameters.accessNmosGateWidth, parameters.transistorRegionHeight),
              accessTransistorR(parameters.techR, parameters.accessNmosGateWidthR, parameters.transistorRegionHeight),
              accessTransistorW(parameters.techW, parameters.accessNmosGateWidthW, parameters.transistorRegionHeight),
              internalCapacitance(parameters.internalCapacitance) {}
};

struct Edram3t333Cell : MemoryCell {
    struct Latency {
        unit::Second bitlineR; // TODO remove later, just here for legacy compat
        unit::Second bitlineW; // TODO remove later, just here for legacy compat
        unit::Second read;
        unit::Second write;
        unit::Second refresh;
    };

    struct Energy {
        unit::Joule readDynamic;
        unit::Joule writeDynamic;
        unit::Joule refreshDynamic;
    };

    using Parameters = Edram3t333Parameters;

    Edram3t333Cell(const Edram3t333Parameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters) {}

    Edram3t333Parameters _parameters;
    Edram3t333Topology _topology;
    Edram3t333Properties _properties;

    const MemoryCellTopology& topology() const noexcept { return _topology; }
    const MemoryCellProperties& properties() const noexcept { return _properties; }
    const MemoryCellParameters& parameters() const noexcept { return _parameters; }

    static constexpr std::string_view name = "eDRAM3T333";

    Latency latency(MemoryCellLatencyContext context) const {
        using namespace mse::unit;

        Farad readCapacitance = _topology.bitlineCapacitance(0) + _topology.storageCapacitance() + context.bitlineCapacitance + context.bitlineMuxCapacitance;
        Farad writeCapacitance = _topology.bitlineCapacitance(1) + _topology.storageCapacitance() + context.bitlineCapacitance + context.bitlineMuxCapacitance;;
        Second tauRead = 2.3 * (_topology.bitlinePullDownResistance(0) * readCapacitance + context.bitlineResistance * (context.bitlineMuxCapacitance + context.bitlineCapacitance / 2.0));
        Second tauWrite = 2.3 * (_topology.bitlinePullDownResistance(1) * writeCapacitance + context.bitlineResistance * (context.bitlineMuxCapacitance + context.bitlineCapacitance / 2.0));
        Second bitlineDelayR = physics::horowitz(tauRead, 0.0, context.rowDecoderRampOutput).delay;
        Second bitlineDelayW = physics::horowitz(tauWrite, 0.0, context.rowDecoderRampOutput).delay;

        Latency latency;

        latency.bitlineR = bitlineDelayR;
        latency.bitlineW = bitlineDelayW;
        latency.read = bitlineDelayR;
        Second decoderLatency = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency);
        latency.write = bitlineDelayW + decoderLatency;
        latency.refresh = (decoderLatency + bitlineDelayW + context.senseAmpReadLatency) * context.numRow;

        return latency;
    }

    Energy energy(MemoryCellEnergyContext context) const {
        using namespace mse::unit;

        Energy energy;

        energy.readDynamic = (_topology.bitlineCapacitance(0) + context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance) * context.senseVoltage * _parameters.techR.get().vdd * context.numColumn;
        Volt writeVoltage = _parameters.techW.get().vpp; // should also equal to setVoltage, for DRAM, it is Vdd
        energy.writeDynamic = (context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance) * writeVoltage * writeVoltage * context.numColumn;
        energy.refreshDynamic = energy.readDynamic + energy.writeDynamic;

        return energy;
    }
};

} // namespace mse::cell

#endif
