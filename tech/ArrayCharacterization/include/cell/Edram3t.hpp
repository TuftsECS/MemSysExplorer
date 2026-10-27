#ifndef MSE_CELL_EDRAM3T_HPP
#define MSE_CELL_EDRAM3T_HPP

#include "cell/MemoryCell.hpp"
#include "cell/Dram.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "physics/Horowitz.hpp"

namespace mse::cell {

struct Edram3tParameters final : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;
    std::reference_wrapper<const Technology> techR;
    std::reference_wrapper<const Technology> techW;

    ScalarParameter<unit::Feature> accessWidth{"Access.Width", "", unit::Feature(0)}; // Write access transistor
    ScalarParameter<unit::Feature> readAccessWidth{"Access.ReadWidth", "", unit::Feature(0)};
    ScalarParameter<unit::Farad> internalCapacitance{"Storage.Capacitance", "", unit::Farad(0)};
    ScalarParameter<unit::Volt> maxNodeDrop{"Storage.MaxNodeDrop", "", unit::Volt(0)};
    ScalarParameter<unit::Second> givenRetentionTime{"Storage.RetentionTime", "", unit::Second(0)};
    ScalarParameter<unit::Ampere> readCurrent{"Access.ReadCurrent", "", unit::Ampere(0)}; // Read device current at the sensing bias, zero uses its on current

    unit::Feature accessNmosGateWidthR;
    unit::Feature accessNmosGateWidthW;
    unit::Feature transistorRegionHeight;
    unit::Second retentionTime; // Found from the write access transistor leakage when not given

    Edram3tParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech), techR(gTechR), techW(gTechW) {
        fillParameter(accessWidth, readAccessWidth, internalCapacitance, maxNodeDrop, readCurrent);
        retentionTime = fillParameter(givenRetentionTime)
            ? givenRetentionTime.value()
            : storageRetentionTime(techW, accessWidth, internalCapacitance, maxNodeDrop);
        
        using namespace mse::unit;

        accessNmosGateWidthR = ((gTechR.featureSizeInNano <= 14) ? 2.0 : 1.0) * readAccessWidth.value();
        accessNmosGateWidthW = ((gTechW.featureSizeInNano <= 14) ? 2.0 : 1.0) * accessWidth.value();

        transistorRegionHeight = cellWidth;
    }
};

struct Edram3tProperties final : MemoryCellProperties {
    Edram3tProperties(const Edram3tParameters& parameters) : MemoryCellProperties(parameters) {
        using namespace mse::unit;
        operations = {Operation::Read, Operation::Write, Operation::Refresh};

        sensingMethod = SenseMethod::Voltage;
        prechargeVoltage = gTech.vdd;

        sharedContact = false;
        columnLeak = true;
        separateReadWordline = true;
        // Only the write device holds off the storage node
        leakageVoltage = parameters.techW.get().vdd;
        destructiveRead = true;
        retentionTime = parameters.retentionTime;
    }
};

struct Edram3tTopology final : MemoryCellTopology {
    NmosTransistor accessTransistorR;
    NmosTransistor accessTransistorW;
    unit::Farad internalCapacitance;
    unit::Ohm readResistance;

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

    unit::Ohm bitlinePullDownResistance(int bitline) const noexcept {
        using namespace mse::unit;
        switch (bitline) {
            case 0: return readResistance;
            case 1: return accessTransistorW.channelResistance();
            default: return 0.0_Ohm;
        }
    }

    unit::Ampere leakageCurrent() const noexcept override {
        return accessTransistorW.leakageCurrent();
    }

    Edram3tTopology(const Edram3tParameters& parameters)
            : accessTransistorR(parameters.techR, parameters.accessNmosGateWidthR, parameters.transistorRegionHeight),
              accessTransistorW(parameters.techW, parameters.accessNmosGateWidthW, parameters.transistorRegionHeight),
              internalCapacitance(parameters.internalCapacitance) {
        const Technology& techR = parameters.techR;
        if (parameters.readCurrent.value() > unit::Ampere(0)) {
            readResistance = techR.effectiveResistanceMultiplier * techR.vdd / parameters.readCurrent.value();
        } else {
            readResistance = accessTransistorR.channelResistance() / constant::readDeviceOnCurrentRatio;
        }
    }
};

class Edram3tCell : public MemoryCell {
private:

    Edram3tParameters _parameters;
    Edram3tTopology _topology;

protected:

    Edram3tProperties _properties;

public:
    using Parameters = Edram3tParameters;

    Edram3tCell(const Edram3tParameters& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters) {}

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }
    const MemoryCellParameters& parameters() const noexcept override { return _parameters; }

    static constexpr std::string_view name = "eDRAM3T";

    CellTiming timing(const MemoryCellLatencyContext& context) const noexcept override {
        using namespace mse::unit;

        Farad readCapacitance = _topology.bitlineCapacitance(0) + _topology.storageCapacitance() + context.bitlineCapacitance + context.bitlineMuxCapacitance;
        Second tauRead = 2.3 * (_topology.bitlinePullDownResistance(0) * readCapacitance + context.bitlineResistance * (context.bitlineMuxCapacitance + context.bitlineCapacitance / 2.0));
        // The write driver sets the write bitline while the write wordline is decoded,
        // then the write device only charges the storage node, the read device's gate and its own drain
        Farad storageNodeCapacitance = _topology.storageCapacitance() + _topology.bitlineCapacitance(1) + _topology.accessTransistorR.gateCapacitance();
        Second tauWrite = 2.3 * _topology.bitlinePullDownResistance(1) * storageNodeCapacitance;
        // The sense amp fires once the read bitline moves by the sense voltage, not after a full swing
        Volt precharge = _properties.prechargeVoltage;
        Second bitlineDelayR = tauRead / 2.3 * std::log((precharge / (precharge - context.senseVoltage)).value());
        Second bitlineDelayW = physics::horowitz(tauWrite, 0.0, context.rowDecoderRampOutput).delay;

        CellTiming timing;

        timing.bitlineRead = bitlineDelayR;
        timing.bitlineWrite = bitlineDelayW;
        timing.latency[Operation::Read] = bitlineDelayR;
        Second decoderLatency = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency);
        timing.latency[Operation::Write] = std::max(decoderLatency, context.writeDriverLatency) + bitlineDelayW;
        timing.latency[Operation::Refresh] = (context.readDecoderLatency + bitlineDelayR + context.senseAmpReadLatency + context.writeDriverLatency + bitlineDelayW)
                * context.numRow;

        return timing;
    }

    CellPower power(const MemoryCellEnergyContext& context) const noexcept override {
        using namespace mse::unit;

        CellPower power;

        power.dynamicEnergy[Operation::Read] = (_topology.bitlineCapacitance(0) + context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance) * context.senseVoltage * _parameters.techR.get().vdd * context.numColumn;
        Volt writeVoltage = _parameters.tech.get().vdd; // The bitline swings to vdd, vpp only boosts the wordline
        power.dynamicEnergy[Operation::Write] = (context.bitlineCapacitance + context.bitlineMuxPreviousPowerCapacitance) * writeVoltage * writeVoltage * context.numColumn;
        power.dynamicEnergy[Operation::Refresh] = power.dynamicEnergy[Operation::Read] + power.dynamicEnergy[Operation::Write];

        return power;
    }

    void printSummary(std::ostream& stream) const override {
        stream << "3T DRAM Cell Write Access Transistor Width: " << _parameters.accessWidth.value().value() << "F\n";
        stream << "3T DRAM Cell Read Access Transistor Width: " << _parameters.readAccessWidth.value().value() << "F\n";
        stream << "3T DRAM Cell Peripheral Roadmap: " << gTech.deviceRoadmap << "\n";
        stream << "3T DRAM Cell Write Access Roadmap: " << gTechW.deviceRoadmap << "\n";
        stream << "3T DRAM Cell Read Access Roadmap: " << gTechR.deviceRoadmap << "\n";
        stream << "3T DRAM Cell Peripheral Node: " << gTech.featureSizeInNano << "nm\n";
        stream << "3T DRAM Cell Write Access Node: " << gTechW.featureSizeInNano << "nm\n";
        stream << "3T DRAM Cell Read Access Node: " << gTechR.featureSizeInNano << "nm\n";
        stream << "3T DRAM Cell VDD: " << gTech.vdd.value() << "V\n";
        stream << "3T DRAM Cell WWL_SWING: " << gTechW.vpp.value() << "V\n";
        stream << "Temperature: " << double(gInputParameter.temperature) << "K\n";
    }

    void writeSummary(YAML::Node node) const override {
        node["WidthWriteAccessCMOS_F"] = _parameters.accessWidth.value().value();
        node["WidthReadAccessCMOS_F"] = _parameters.readAccessWidth.value().value();
        node["PeripheralRoadmap"] = enumToString(gTech.deviceRoadmap);
        node["WriteAccessRoadmap"] = enumToString(gTechW.deviceRoadmap);
        node["ReadAccessRoadmap"] = enumToString(gTechR.deviceRoadmap);
        node["PeripheralNode_nm"] = gTech.featureSizeInNano;
        node["WriteAccessNode_nm"] = gTechW.featureSizeInNano;
        node["ReadAccessNode_nm"] = gTechR.featureSizeInNano;
        node["VDD_V"] = gTech.vdd.value();
        node["WWL_SWING"] = gTechW.vpp.value();
        node["Temperature_K"] = double(gInputParameter.temperature);
    }
};

} // namespace mse::cell

#endif
