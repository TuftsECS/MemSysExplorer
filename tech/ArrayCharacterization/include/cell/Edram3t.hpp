#ifndef mse_edram3t_hpp
#define mse_edram3t_hpp

#include "cell/MemoryCell.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"

namespace mse::cell {

struct Edram3tParameters final : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;
    std::reference_wrapper<const Technology> techR;
    std::reference_wrapper<const Technology> techW;

    //ScalarParameter<unit::Feature> accessNmosGateWidth;
    //ScalarParameter<unit::Feature> accessNmosGateWidthR;
    //ScalarParameter<unit::Feature> accessNmosGateWidthW;
    ScalarParameter<unit::Farad> internalCapacitance{"Storage Node Capacitance"};

    unit::Feature accessNmosGateWidth;
    unit::Feature accessNmosGateWidthR;
    unit::Feature accessNmosGateWidthW;
    unit::Feature transistorRegionHeight;

    Edram3tParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech), techR(gTechR), techW(gTechW) {
        // TODO use specific cell fields in config file
        // for now just steal from gCell, figure out new input style later
        // fillParameter(accessNmosGateWidth, accessNmosGateWidthR, accessNmosGateWidthW);
        fillParameter(internalCapacitance);
        
        using namespace mse::unit;

        accessNmosGateWidth = ((gTech.featureSize <= 14 * 1e-9) ? 2_feat : 1_feat) * gCell.widthAccessCMOS;
        accessNmosGateWidthR = ((gTechR.featureSize <= 14 * 1e-9) ? 2_feat : 1_feat) * gCell.widthAccessCMOSR;
        accessNmosGateWidthW = ((gTechW.featureSize <= 14 * 1e-9) ? 2_feat : 1_feat) * gCell.widthAccessCMOS;

        transistorRegionHeight = unit::Feature(gCell.widthInFeatureSize);
    }
};

struct Edram3tProperties final : MemoryCellProperties {
    Edram3tProperties(const Edram3tParameters&) {
        using namespace mse::unit;

        sensingMethod = SenseMethod::Voltage;
		//sensingVoltage = Volt(gTech.vdd) / 2.0 * (parameters.internalCapacitance / (parameters.internalCapacitance + parameters.capBitline));
        prechargeVoltage = unit::Volt(gTech.vdd);

        sharedContact = false;
    }
};

struct Edram3tTopology final : MemoryCellTopology {
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

    unit::Ohm bitlinePullDownResistance(int bitline, [[maybe_unused]] int path = 0) const noexcept override {
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

    Edram3tTopology(const Edram3tParameters& parameters)
            : accessTransistor(parameters.tech, parameters.accessNmosGateWidth, parameters.transistorRegionHeight),
              accessTransistorR(parameters.techR, parameters.accessNmosGateWidthR, parameters.transistorRegionHeight),
              accessTransistorW(parameters.techW, parameters.accessNmosGateWidthW, parameters.transistorRegionHeight),
              internalCapacitance(parameters.internalCapacitance) {}
};

struct Edram3tCell : MemoryCell {
    struct Latency {
        unit::Second bitlineR; // TODO remove later, just here for legacy compat
        unit::Second bitlineW; // TODO remove later, just here for legacy compat
        unit::Second read;
        unit::Second write;
        unit::Second refresh;
    };

    struct Energy {
        unit::Joule read;
        unit::Joule write;
        unit::Joule refresh;
    };

    using Parameters = Edram3tParameters;

    Edram3tCell(const Edram3tParameters& parameters) : _topology(parameters), _properties(parameters) {}

    Edram3tTopology _topology;
    Edram3tProperties _properties;

    const MemoryCellTopology& topology() const noexcept { return _topology; }
    const MemoryCellProperties& properties() const noexcept { return _properties; }

    static inline constexpr std::string name = "eDRAM3T";
};

} // namespace mse::cell

#endif
