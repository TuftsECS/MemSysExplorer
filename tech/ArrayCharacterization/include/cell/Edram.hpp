#ifndef MSE_CELL_EDRAM_HPP
#define MSE_CELL_EDRAM_HPP

#include "cell/MemoryCell.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "physics/Horowitz.hpp"

namespace mse::cell {

struct EdramParameters final : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;

    ScalarParameter<unit::Feature> accessNmosGateWidth{"Access CMOS Width", "", unit::Feature(1)};
    ScalarParameter<unit::Farad> internalCapacitance{"Storage Node Capacitance", "", unit::Farad(1)};

    unit::Feature transistorRegionHeight;

    EdramParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech) {
        //fillParameter(accessNmosGateWidth, internalCapacitance);
        // TODO
        accessNmosGateWidth.value() = unit::Feature(gCell.widthAccessCMOS);
        internalCapacitance.value() = unit::Farad(gCell.capDRAMCell);

        unit::Number featureScale = (tech.get().featureSizeInNano <= 14 ? 2 : 1);
        accessNmosGateWidth.value() *= featureScale;

        transistorRegionHeight = unit::Feature(gCell.widthInFeatureSize);
    }
};

class EdramProperties final : public MemoryCellProperties {
public:
    EdramProperties(const EdramParameters&) {
        using namespace mse::unit;

        sensingMethod = SenseMethod::Voltage;
		//sensingVoltage = Volt(gTech.vdd) / 2.0 * (parameters.internalCapacitance / (parameters.internalCapacitance + parameters.capBitline));
        sensingVoltage = 0_V; // TODO unused
        prechargeVoltage = gTech.vdd * 0.5;

        sharedContact = true;
    }

    bool isValid() {
        if (sensingVoltage.value() < gCell.minSenseVoltage) {
            // bitline too long
            return false;
        }
        return true;
    }
};

class EdramTopology final : public MemoryCellTopology {
public:
    NmosTransistor accessTransistor;
    unit::Farad internalCapacitance;

    CellLineAdapter<CmosTransistor> accessTransistorAdapter = accessTransistor;

    WordlineList<1> wordlineList = {{
        {accessTransistorAdapter}
    }};

    BitlineList<1> bitlineList = {{
        {accessTransistorAdapter}
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
            case 0: return accessTransistor.channelResistance();
            default: return 0.0_Ohm;
        }
    }

    unit::Ampere leakageCurrent() const noexcept override {
        return accessTransistor.leakageCurrent();
    }

    EdramTopology(const EdramParameters& parameters)
            : accessTransistor(parameters.tech, parameters.accessNmosGateWidth, parameters.transistorRegionHeight),
              internalCapacitance(parameters.internalCapacitance) {}
};

class EdramCell final : public MemoryCell {
public:
    struct Latency {
        mse::unit::Second bitline; // TODO remove later, just here for legacy compat
        mse::unit::Second read;
        mse::unit::Second write;
        mse::unit::Second refresh;
    };

    struct Energy {
        mse::unit::Joule readDynamic;
        mse::unit::Joule writeDynamic;
        mse::unit::Joule refreshDynamic;
    };

    using Parameters = EdramParameters;

    EdramCell(const EdramParameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters) {}

    EdramParameters _parameters;
    EdramTopology _topology;
    EdramProperties _properties;

    const MemoryCellTopology& topology() const noexcept { return _topology; }
    const MemoryCellProperties& properties() const noexcept { return _properties; }
    const MemoryCellParameters& parameters() const noexcept { return _parameters; }

    static constexpr std::string_view name = "eDRAM";

    Latency latency(MemoryCellLatencyContext context) const {
        using namespace mse::unit;

        Farad cellCap = _topology.bitlineCapacitance(0) + _topology.storageCapacitance();
        Farad lineCap = context.bitlineCapacitance + context.bitlineMuxCapacitance;
        Farad effectiveCapacitance = cellCap * lineCap / (cellCap + lineCap);

        Second tau = (context.bitlineResistance + _topology.bitlinePullDownResistance(0, 0)) * effectiveCapacitance * 2.3;

        Latency latency;

        Second bitlineDelay = physics::horowitz(tau, Number(0.0), context.rowDecoderRampOutput).delay;
        latency.bitline = bitlineDelay;
        latency.read = bitlineDelay;
        Second decoderLatency = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency);
        latency.write = decoderLatency + bitlineDelay + context.senseAmpReadLatency + context.senseAmpMuxLev1ReadLatency + context.senseAmpMuxLev2ReadLatency;

        // refresh doesnt pass sense amplifier
        latency.refresh = (decoderLatency + bitlineDelay + context.senseAmpReadLatency) * context.numRow;

        return latency;
    }

    Energy energy(MemoryCellEnergyContext context) const {
        using namespace mse::unit;

        Energy energy;

        energy.readDynamic = (_topology.bitlineCapacitance(0) + context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance) * context.senseVoltage * gTech.vdd * context.numColumn;
        Volt writeVoltage = gTech.vpp; // should also equal to setVoltage, for DRAM it is Vdd
        energy.writeDynamic = (context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance) * writeVoltage * writeVoltage * context.numColumn;
        energy.refreshDynamic = energy.readDynamic + energy.writeDynamic;

        return energy;
    }
};

} // namspace mse::cell

#endif
