#ifndef MSE_CELL_EDRAM_HPP
#define MSE_CELL_EDRAM_HPP

#include "cell/MemoryCell.hpp"
#include "global.hpp"

namespace mse::cell {

struct EdramParameters final : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;

    ScalarParameter<unit::Feature> accessNmosGateWidth{"Access CMOS Width"};
    ScalarParameter<unit::Farad> internalCapacitance{"Storage Node Capacitance"};

    unit::Feature transistorRegionHeight;

    EdramParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech) {
        fillParameter(accessNmosGateWidth, internalCapacitance);

        unit::Number featureScale = (tech.get().featureSize <= 14 * 1e-9 ? 2 : 1);
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
        prechargeVoltage = gTech.vdd * 0.5_V;

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

    unit::Ohm bitlinePullDownResistance(int bitline, [[maybe_unused]] int path = 0) const noexcept override {
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
        mse::unit::Joule read;
        mse::unit::Joule write;
        mse::unit::Joule refresh;
    };

    using Parameters = EdramParameters;

    EdramCell(const EdramParameters& parameters) : _topology(parameters), _properties(parameters) {}

    EdramTopology _topology;
    EdramProperties _properties;

    const MemoryCellTopology& topology() const noexcept { return _topology; }
    const MemoryCellProperties& properties() const noexcept { return _properties; }

    static inline constexpr std::string name = "eDRAM";
};

} // namspace mse::cell

#endif
