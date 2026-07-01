#ifndef MSE_CELL_SRAM6T_HPP
#define MSE_CELL_SRAM6T_HPP

#include "cell/MemoryCell.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"

namespace mse::cell {

struct Sram6tParameters final : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;

    ScalarParameter<unit::Feature> inverterNmosGateWidth{"Nmos Width", "", unit::Feature(2.08)}; // Default value from CACTI
    ScalarParameter<unit::Feature> inverterPmosGateWidth{"Pmos Width", "", unit::Feature(1.23)}; // Default value from CACTI
    ScalarParameter<unit::Feature> accessNmosGateWidth{"Access CMOS Width"};

    unit::Feature transistorRegionHeight;

    Sram6tParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech) {
        fillParameter(inverterNmosGateWidth, inverterPmosGateWidth, accessNmosGateWidth);

        unit::Number featureScale = (tech.get().featureSize <= 14 * 1e-9 ? 2 : 1);
        inverterNmosGateWidth.value() *= featureScale;
        inverterPmosGateWidth.value() *= featureScale;
        accessNmosGateWidth.value() *= featureScale;

        transistorRegionHeight = unit::Feature(gCell.widthInFeatureSize);
    }
};

struct Sram6tProperties final : public MemoryCellProperties {
    Sram6tProperties(const Sram6tParameters&) {
        using namespace mse::unit;

        sensingMethod = SenseMethod::Voltage;
        sensingVoltage = Volt(gCell.minSenseVoltage);
        prechargeVoltage = gTech.vdd * 0.5_V;

        sharedContact = true;
        isMLC = false;
        columnLeak = true;
    }
};

class Sram6tTopology final : public MemoryCellTopology {
public:
    NmosTransistor normalAccessTransistor;
    NmosTransistor invertedAccessTransistor;
    CmosInverter normalOutputInverter;
    CmosInverter invertedOutputInverter;

    CellLineAdapter<CmosTransistor> normalAccessTransistorAdapter = normalAccessTransistor;
    CellLineAdapter<CmosTransistor> invertedAccessTransistorAdapter = invertedAccessTransistor;

    WordlineList<1> wordlineList = {{
        {normalAccessTransistorAdapter, invertedAccessTransistorAdapter}
    }};

    BitlineList<2> bitlineList = {{
        {normalAccessTransistorAdapter},
        {invertedAccessTransistorAdapter}
    }};

    WordlineListSpan wordlineControlledDevices() const noexcept override {
        return wordlineList;
    }

    BitlineListSpan bitlineAccessDevices() const noexcept override {
        return bitlineList;
    }

    unit::Number beta(int bitline) const noexcept {
        switch (bitline) {
            case 0: return 1.0 / (normalAccessTransistor.transconductance() * normalOutputInverter.nmosResistance());
            case 1: return 1.0 / (invertedAccessTransistor.transconductance() * invertedOutputInverter.nmosResistance());
            default: return unit::Number(0.0);
        }
    }

    unit::Ohm bitlinePullDownResistance(int bitline, [[maybe_unused]] int path = 0) const noexcept override {
        using namespace mse::unit;
        switch (bitline) {
            case 0: return normalAccessTransistor.channelResistance() + normalOutputInverter.nmosResistance();
            case 1: return invertedAccessTransistor.channelResistance() + invertedOutputInverter.nmosResistance();
            default: return 0.0_Ohm;
        }
    }

    unit::Ampere leakageCurrent() const noexcept override {
        return normalOutputInverter.leakageCurrent() + invertedOutputInverter.leakageCurrent() + (normalAccessTransistor.leakageCurrent() + invertedAccessTransistor.leakageCurrent()) / 2.0;
        // the average of the normal access and inverted access leakage currents is used because their values are opposite
        // one will be on, and the other will be off and leaking
        // so taking the 50/50 average assumes the probability the cell is on (0.5) = the probability the cell is off (0.5)
    }

    Sram6tTopology(const Sram6tParameters& parameters)
            : normalAccessTransistor(parameters.tech, parameters.accessNmosGateWidth, parameters.transistorRegionHeight),
              invertedAccessTransistor(normalAccessTransistor),
              normalOutputInverter(parameters.tech, parameters.inverterNmosGateWidth, parameters.inverterPmosGateWidth, parameters.transistorRegionHeight),
              invertedOutputInverter(normalOutputInverter) {}
};

class Sram6tCell final : public MemoryCell {
public:
    struct Latency {
        mse::unit::Second bitline; // TODO remove later, just here for legacy compat
        mse::unit::Second read;
        mse::unit::Second write;
    };

    struct Energy {
        mse::unit::Joule read;
        mse::unit::Joule write;
    };

    using Parameters = Sram6tParameters;

    Sram6tCell(const Parameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters) {}

    Parameters _parameters;
    Sram6tTopology _topology;
    Sram6tProperties _properties;

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }

    static inline constexpr std::string name = "SRAM";
};

} // namespace mse::cell

#endif
