#ifndef MSE_CELL_SRAM8T_HPP
#define MSE_CELL_SRAM8T_HPP

#include "cell/MemoryCell.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"

namespace mse::cell {

struct Sram8tParameters final : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;

    input::SimpleParameter<unit::Feature> inverterNmosGateWidth{"Nmos Width", "", unit::Feature(2.08)}; // Default value from CACTI
    input::SimpleParameter<unit::Feature> inverterPmosGateWidth{"Pmos Width", "", unit::Feature(1.23)}; // Default value from CACTI
    input::SimpleParameter<unit::Feature> accessNmosGateWidth{"Access CMOS Width"};

    unit::Feature transistorRegionHeight;

    Sram8tParameters(input::YamlFile& file) : MemoryCellParameters(file), tech(gTech) {
        fillParameter(inverterNmosGateWidth, inverterPmosGateWidth, accessNmosGateWidth);

        unit::Number featureScale = (tech.get().featureSizeInNano <= 14 ? 2 : 1);
        inverterNmosGateWidth.value() *= featureScale;
        inverterPmosGateWidth.value() *= featureScale;
        accessNmosGateWidth.value() *= featureScale;

        transistorRegionHeight = unit::Feature(gCell.widthInFeatureSize);
    }
};

struct Sram8tProperties final : public MemoryCellProperties {
    Sram8tProperties(const Sram8tParameters&) {
        using namespace mse::unit;

        sensingMethod = SenseMethod::Voltage;
        sensingVoltage = Volt(gCell.minSenseVoltage);
        prechargeVoltage = gTech.vdd * 0.5;

        sharedContact = true;
        isMLC = false;
        columnLeak = true;
    }
};

class Sram8tTopology final : public MemoryCellTopology {
public:
    NmosTransistor acl;
    NmosTransistor acr;
    NmosTransistor r1;
    NmosTransistor r2;
    CmosInverter invL;
    CmosInverter invR;

    CellLineAdapter<CmosTransistor> aclAdapter = acl;
    CellLineAdapter<CmosTransistor> acrAdapter = acr;
    CellLineAdapter<CmosTransistor> r2Adapter = r2;

    WordlineList<2> wordlineList = {{
        {r2Adapter}, // read
        {aclAdapter, acrAdapter} // write
    }};

    BitlineList<3> bitlineList = {{
        {r2Adapter}, // read
        {aclAdapter}, // write
        {acrAdapter}, // write bar
    }};

    WordlineListSpan wordlineControlledDevices() const noexcept override {
        return wordlineList;
    }

    BitlineListSpan bitlineAccessDevices() const noexcept override {
        return bitlineList;
    }

    unit::Number beta(int bitline) const noexcept {
        using namespace mse::unit;
        switch (bitline) {
            case 0: return 1.0 / (r2.transconductance() * r1.channelResistance()); // read
            case 1: return 1.0 / (acl.transconductance() * invL.nmosResistance()); // write
            case 2: return 1.0 / (acr.transconductance() * invR.nmosResistance()); // write bar
            default: return Number(0.0);
        }
    }

    unit::Ohm bitlinePullDownResistance(int bitline, [[maybe_unused]] int path = 0) const noexcept {
        using namespace mse::unit;
        switch (bitline) {
            case 0: return r1.channelResistance() + r2.channelResistance(); // read
            case 1: return acl.channelResistance() + invL.nmosResistance(); // write
            case 2: return acr.channelResistance() + invR.nmosResistance(); // write bar
            default: return 0.0_Ohm;
        }
    }

    unit::Ampere leakageCurrent() const noexcept override {
        return invL.leakageCurrent() + invR.leakageCurrent() + (acl.leakageCurrent() + acr.leakageCurrent()) / 2.0 + (r1.leakageCurrent() + r2.leakageCurrent()) / 2.0;
    }

    Sram8tTopology(const Sram8tParameters& parameters)
            : acl(parameters.tech, mse::unit::Feature(2.77), parameters.transistorRegionHeight),
              acr(acl),
              r1(acl),
              r2(parameters.tech, mse::unit::Feature(3.54), parameters.transistorRegionHeight),
              invL(parameters.tech, mse::unit::Feature(2.31), mse::unit::Feature(3.08), parameters.transistorRegionHeight),
              invR(invL) {}
};

class Sram8tCell final : public MemoryCell {
public:
    struct Latency {
        mse::unit::Second bitlineR; // TODO remove later, just here for legacy compat
        mse::unit::Second bitlineW; // TODO remove later, just here for legacy compat
        mse::unit::Second read;
        mse::unit::Second write;
    };

    struct Energy {
        mse::unit::Joule read;
        mse::unit::Joule write;
    };

    using Parameters = Sram8tParameters;

    Sram8tCell(const Parameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters) {}

    Parameters _parameters;
    Sram8tTopology _topology;
    Sram8tProperties _properties;

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }

    static constexpr std::string_view name = "SRAM 8T";
};

} // namespace mse::cell

#endif
