#ifndef MSE_CELL_SRAM6T_HPP
#define MSE_CELL_SRAM6T_HPP

#include "cell/MemoryCell.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "physics/Horowitz.hpp"

#include <cmath>

namespace mse::cell {

struct Sram6tParameters final : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;

    ScalarParameter<unit::Feature> inverterNmosGateWidth{"Inverter.NmosWidth", "", unit::Feature(2.08)}; // Default value from CACTI
    ScalarParameter<unit::Feature> inverterPmosGateWidth{"Inverter.PmosWidth", "", unit::Feature(1.23)}; // Default value from CACTI
    ScalarParameter<unit::Feature> accessNmosGateWidth{"Access.Width", "", unit::Feature(0)};
    ScalarParameter<unit::Volt> readVoltage{"Read.Voltage", "", unit::Volt(0)}; // Precharge voltage of the global bitlines with external sense amps

    unit::Feature transistorRegionHeight;

    Sram6tParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech) {
        fillParameter(inverterNmosGateWidth, inverterPmosGateWidth, accessNmosGateWidth, readVoltage);

        unit::Number featureScale = (tech.get().featureSizeInNano <= 14 ? 2 : 1);
        inverterNmosGateWidth.value() *= featureScale;
        inverterPmosGateWidth.value() *= featureScale;
        accessNmosGateWidth.value() *= featureScale;

        transistorRegionHeight = cellWidth;
    }
};

struct Sram6tProperties final : public MemoryCellProperties {
    Sram6tProperties(const Sram6tParameters& parameters) : MemoryCellProperties(parameters) {
        using namespace mse::unit;
        operations = {Operation::Read, Operation::Write};

        sensingMethod = SenseMethod::Voltage;
        prechargeVoltage = gTech.vdd * 0.5;

        sharedContact = true;
        columnLeak = true;

        globalBitline.model = GlobalBitlineModel::Sram;
        globalBitline.readVoltage = parameters.readVoltage;
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

    unit::Ampere leakageCurrent() const noexcept override {
        return normalOutputInverter.leakageCurrent() + invertedOutputInverter.leakageCurrent() + (normalAccessTransistor.leakageCurrent() + invertedAccessTransistor.leakageCurrent()) / 2.0;
        // the average of the normal access and inverted access leakage currents is used because their values are opposite
        // one will be on, and the other will be off and leaking
        // so taking the 50/50 average assumes the probability the cell is on (0.5) = the probability the cell is off (0.5)
    }

    unit::Number beta(int bitline) const noexcept {
        switch (bitline) {
            case 0: return 1.0 / (normalAccessTransistor.transconductance() * normalOutputInverter.nmosResistance());
            case 1: return 1.0 / (invertedAccessTransistor.transconductance() * invertedOutputInverter.nmosResistance());
            default: return 0.0;
        }
    }

    unit::Ohm bitlinePullDownResistance(int bitline) const noexcept {
        using namespace mse::unit;
        switch (bitline) {
            case 0: return normalAccessTransistor.channelResistance() + normalOutputInverter.nmosResistance();
            case 1: return invertedAccessTransistor.channelResistance() + invertedOutputInverter.nmosResistance();
            default: return 0.0_Ohm;
        }
    }

    Sram6tTopology(const Sram6tParameters& parameters)
            : normalAccessTransistor(parameters.tech, parameters.accessNmosGateWidth, parameters.transistorRegionHeight),
              invertedAccessTransistor(normalAccessTransistor),
              normalOutputInverter(parameters.tech, parameters.inverterNmosGateWidth, parameters.inverterPmosGateWidth, parameters.transistorRegionHeight),
              invertedOutputInverter(normalOutputInverter) {}
};

class Sram6tCell final : public MemoryCell {
public:
    using Parameters = Sram6tParameters;

private:

    Parameters _parameters;
    Sram6tTopology _topology;
    Sram6tProperties _properties;

public:

    Sram6tCell(const Parameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters) {}

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }
    const MemoryCellParameters& parameters() const noexcept override { return _parameters; }

    static constexpr std::string_view name = "SRAM";

    CellTiming timing(const MemoryCellLatencyContext& context) const noexcept override {
        using namespace mse::unit;

        Ohm cellToBitlineResistance = _topology.bitlinePullDownResistance(0);
        Farad cellToBitlineCapacitance = _topology.bitlineCapacitance(0);
        Number beta = _topology.beta(0);
        Volt prechargeVoltage = _properties.prechargeVoltage;
        Volt sensingVoltage = _parameters.minSenseVoltage.value();

        Second tau
            = cellToBitlineResistance * cellToBitlineCapacitance
            + cellToBitlineResistance * (context.bitlineCapacitance / 2.0)
            + (cellToBitlineResistance + context.bitlineResistance) * (context.bitlineCapacitance / 2.0)
            + (cellToBitlineResistance + context.bitlineResistance) * context.bitlineMuxCapacitance;

        tau *= std::log(prechargeVoltage / (prechargeVoltage - sensingVoltage / 2.0));
        // one signal raises and the other drops, so sensing voltage / 2 is enough

        CellTiming timing;

        SecondExp<-1> inputRamp = context.rowDecoderRampOutput;
        Second bitlineDelay = physics::horowitz(tau, beta, inputRamp).delay;
        timing.bitline = bitlineDelay;
        timing.latency[Operation::Read] = bitlineDelay + context.bitlineMuxReadLatency;
		Second decoderLatency = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency);
        timing.latency[Operation::Write] = decoderLatency + bitlineDelay + context.bitlineMuxReadLatency + context.senseAmpReadLatency + context.senseAmpMuxLev1ReadLatency + context.senseAmpMuxLev2ReadLatency;

        return timing;
    }

    CellPower power(const MemoryCellEnergyContext& context) const noexcept override {
        using namespace mse::unit;

        CellPower power;

        power.dynamicEnergy[Operation::Read] = (_topology.bitlineCapacitance(0) + context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance) * _properties.prechargeVoltage * _properties.prechargeVoltage * context.numColumn;
        power.dynamicEnergy[Operation::Write] = (_topology.bitlineCapacitance(0) + context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance) * _properties.prechargeVoltage * _properties.prechargeVoltage * context.numColumn / context.muxSenseAmp / context.muxOutputLev1 / context.muxOutputLev2;

        return power;
    }

    void printSummary(std::ostream& stream) const override {
        stream << "SRAM Cell Access Transistor Width: " << _parameters.accessNmosGateWidth.value().value() << "F\n";
        stream << "SRAM Cell NMOS Width: " << _parameters.inverterNmosGateWidth.value().value() << "F\n";
        stream << "SRAM Cell PMOS Width: " << _parameters.inverterPmosGateWidth.value().value() << "F\n";
        stream << "SRAM Cell Peripheral Roadmap: " << gTech.deviceRoadmap << "\n";
        stream << "SRAM Cell Peripheral Node: " << gTech.featureSizeInNano << "nm\n";
        stream << "SRAM Cell VDD: " << gTech.vdd.value() << "V\n";
        stream << "Temperature: " << double(gInputParameter.temperature) << "K\n";
    }

    void writeSummary(YAML::Node node) const override {
        node["WidthAccessCMOS_F"] = _parameters.accessNmosGateWidth.value().value();
        node["WidthSRAMCellNMOS_F"] = _parameters.inverterNmosGateWidth.value().value();
        node["WidthSRAMCellPMOS_F"] = _parameters.inverterPmosGateWidth.value().value();
        node["PeripheralRoadmap"] = enumToString(gTech.deviceRoadmap);
        node["PeripheralNode_nm"] = gTech.featureSizeInNano;
        node["VDD_V"] = gTech.vdd.value();
        node["WWL_SWING"] = gTech.vdd.value();
        node["Temperature_K"] = double(gInputParameter.temperature);
    }
};

} // namespace mse::cell

#endif
