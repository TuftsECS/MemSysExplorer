#ifndef MSE_CELL_DRAM_HPP
#define MSE_CELL_DRAM_HPP

#include "cell/MemoryCell.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "physics/Horowitz.hpp"

#include <cmath>

namespace mse::cell {

// How long a storage node keeps its data through the leakage of a minimum length access transistor of the given width
inline unit::Second storageRetentionTime(const Technology& tech, unit::Feature accessWidth, unit::Farad storageCapacitance, unit::Volt maxNodeDrop) {
    const double width = accessWidth.value();

    double effectiveWidth;
    if (tech.featureSizeInNano >= 22) {
        effectiveWidth = width * tech.featureSizeInNano * 1e-9;
    } else if (tech.featureSizeInNano >= 3) {
        effectiveWidth = static_cast<int>(std::ceil(width)) * tech.effectiveWidth.value();
    } else {
        effectiveWidth = static_cast<int>(std::ceil(width)) * tech.effectiveWidth.value() * tech.maximumSheetCount / tech.maximumFinsPerGAA;
    }
    const double leakageCurrent = tech.currentOffNmos[gInputParameter.temperature - 300].value() * effectiveWidth;

    return unit::Second((storageCapacitance.value() * maxNodeDrop.value()) / leakageCurrent);
}

struct DramParameters final : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;

    ScalarParameter<unit::Feature> accessNmosGateWidth{"Access.Width", "", unit::Feature(0)};
    ScalarParameter<unit::Farad> internalCapacitance{"Storage.Capacitance", "", unit::Farad(0)};
    ScalarParameter<unit::Volt> maxNodeDrop{"Storage.MaxNodeDrop", "", unit::Volt(0)};
    ScalarParameter<unit::Second> givenRetentionTime{"Storage.RetentionTime", "", unit::Second(0)};

    unit::Feature transistorRegionHeight;
    unit::Second retentionTime; // Found from the access transistor leakage when not given

    DramParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech) {
        fillParameter(accessNmosGateWidth, internalCapacitance, maxNodeDrop);
        retentionTime = fillParameter(givenRetentionTime)
            ? givenRetentionTime.value()
            : storageRetentionTime(tech, accessNmosGateWidth, internalCapacitance, maxNodeDrop);

        unit::Number featureScale = (tech.get().featureSizeInNano <= 14 ? 2 : 1);
        accessNmosGateWidth.value() *= featureScale;

        transistorRegionHeight = cellWidth;
    }
};

struct DramProperties final : MemoryCellProperties {
    DramProperties(const DramParameters& parameters) : MemoryCellProperties(parameters) {
        using namespace mse::unit;
        operations = {Operation::Read, Operation::Write, Operation::Refresh};

        sensingMethod = SenseMethod::Voltage;
        prechargeVoltage = gTech.vdd * 0.5;

        sharedContact = true;
        columnLeak = true;
        destructiveRead = true;
        boostedWordline = true;
        retentionTime = parameters.retentionTime;
    }
};

class DramTopology final : public MemoryCellTopology {
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

    DramTopology(const DramParameters& parameters)
            : accessTransistor(parameters.tech, parameters.accessNmosGateWidth, parameters.transistorRegionHeight),
              internalCapacitance(parameters.internalCapacitance) {}
};

class DramCell : public MemoryCell {
private:

    DramParameters _parameters;
    DramTopology _topology;
    DramProperties _properties;

public:
    using Parameters = DramParameters;

    DramCell(const DramParameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters) {}

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }
    const MemoryCellParameters& parameters() const noexcept override { return _parameters; }

    static constexpr std::string_view name = "DRAM";

    // The stored charge is shared with the bitline when read
    unit::Volt senseVoltage(unit::Farad bitlineCapacitance) const noexcept override {
        const double storageCapacitance = _parameters.internalCapacitance.value().value();

        return gTech.vdd / 2 * storageCapacitance / (storageCapacitance + bitlineCapacitance.value());
    }

    CellTiming timing(const MemoryCellLatencyContext& context) const noexcept override {
        using namespace mse::unit;

        Farad cellCap = _topology.bitlineCapacitance(0) + _topology.storageCapacitance();
        Farad lineCap = context.bitlineCapacitance + context.bitlineMuxCapacitance;
        Farad effectiveCapacitance = cellCap * lineCap / (cellCap + lineCap);

        Second tau = (context.bitlineResistance + _topology.bitlinePullDownResistance(0, 0)) * effectiveCapacitance * 2.3;

        CellTiming timing;

        Second bitlineDelay = physics::horowitz(tau, Number(0.0), context.rowDecoderRampOutput).delay;
        timing.bitline = bitlineDelay;
        timing.latency[Operation::Read] = bitlineDelay;
        Second decoderLatency = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency);
        timing.latency[Operation::Write] = decoderLatency + bitlineDelay + context.senseAmpReadLatency + context.senseAmpMuxLev1ReadLatency + context.senseAmpMuxLev2ReadLatency;

        // refresh doesnt pass sense amplifier
        timing.latency[Operation::Refresh] = (decoderLatency + bitlineDelay + context.senseAmpReadLatency) * context.numRow;

        return timing;
    }

    CellPower power(const MemoryCellEnergyContext& context) const noexcept override {
        using namespace mse::unit;

        CellPower power;

        power.dynamicEnergy[Operation::Read] = (_topology.bitlineCapacitance(0) + context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance) * context.senseVoltage * gTech.vdd * context.numColumn;
        Volt writeVoltage = gTech.vdd; // The bitline swings to vdd, vpp only boosts the wordline
        power.dynamicEnergy[Operation::Write] = (context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance) * writeVoltage * writeVoltage * context.numColumn;
        power.dynamicEnergy[Operation::Refresh] = power.dynamicEnergy[Operation::Read] + power.dynamicEnergy[Operation::Write];

        return power;
    }

    void printSummary(std::ostream& stream) const override {
        stream << "DRAM Cell Access Transistor Width: " << _parameters.accessNmosGateWidth.value().value() << "F\n";
        stream << "DRAM Cell Peripheral Roadmap: " << gTech.deviceRoadmap << "\n";
        stream << "DRAM Cell Peripheral Node: " << gTech.featureSizeInNano << "nm\n";
        stream << "DRAM Cell VDD: " << gTech.vdd.value() << "V\n";
        stream << "DRAM Cell WL_SWING: " << gTech.vpp.value() << "V\n";
        stream << "Temperature: " << double(gInputParameter.temperature) << "K\n";
    }

    void writeSummary(YAML::Node node) const override {
        node["WidthAccessCMOS_F"] = _parameters.accessNmosGateWidth.value().value();
        node["PeripheralRoadmap"] = enumToString(gTech.deviceRoadmap);
        node["PeripheralNode_nm"] = gTech.featureSizeInNano;
        node["VDD_V"] = gTech.vdd.value();
        node["WWL_SWING"] = gTech.vpp.value();
        node["Temperature_K"] = double(gInputParameter.temperature);
    }
};

} // namespace mse::cell

#endif
