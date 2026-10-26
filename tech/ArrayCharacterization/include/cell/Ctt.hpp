#ifndef MSE_CELL_CTT_HPP
#define MSE_CELL_CTT_HPP

#include "cell/MemoryCell.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "physics/Horowitz.hpp"

#include <cmath>

namespace mse::cell {

struct CttParameters final : MemoryCellParameters, MultiLevelParameters {
    std::reference_wrapper<const Technology> tech;

    ScalarParameter<std::string> readModeName{"Read.Mode", "", std::string("voltage")};
    bool readMode; // true = voltage sensing, false = current sensing

    unit::Feature cellNmosGateWidth;
    unit::Feature transistorRegionHeight;

    CttParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech) {
        using namespace mse::unit;

        fillParameter(readModeName, levels, senseAmpFingers);
        readMode = readModeName.value() == "voltage";

        cellNmosGateWidth = (gTech.featureSizeInNano <= 14) ? 2_feat : 1_feat; // Minimum size transistor
        transistorRegionHeight = cellWidth;
    }
};

struct CttProperties final : MemoryCellProperties {
    CttProperties(const CttParameters& parameters) : MemoryCellProperties(parameters) {
        using namespace mse::unit;
        operations = {Operation::Read, Operation::Write, Operation::Set, Operation::Reset};

        sensingMethod = parameters.readMode ? SenseMethod::Voltage : SenseMethod::Current;
        prechargeVoltage = gTech.vdd * 0.5;

        sharedContact = false;
        columnLeak = false;

        multiLevel = parameters.multiLevel();
        senseAmp.fittedMultiLevel = multiLevel.has_value();
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
        return unit::Ampere(0); // TODO no leakage model for CTT cells
    }

    CttTopology(const CttParameters& parameters)
            : cellTransistor(parameters.tech, parameters.cellNmosGateWidth, parameters.transistorRegionHeight) {}
};

class CttCell final : public MemoryCell {
public:
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

    // The access transistor
    unit::Meter bitlineOverhead(long long) const noexcept override {
        using namespace mse::unit;

        return 5_feat * gTech.featureSize;
    }

    CellTiming timing(const MemoryCellLatencyContext& context) const noexcept override {
        using namespace mse::unit;

        Volt prechargeVoltage = _properties.prechargeVoltage;

        Second tau
            = _topology.bitlinePullDownResistance() * (_topology.bitlineCapacitance(0) + context.bitlineCapacitance + context.bitlineMuxCapacitance)
            + context.bitlineResistance * (context.bitlineMuxCapacitance + context.bitlineCapacitance / 2.0);

        tau *= std::log(prechargeVoltage / (prechargeVoltage - context.senseVoltage));
        // in one case the bitline is unchanged, and in the other case the bitline drops

        CellTiming timing;

        Second bitlineDelay = physics::horowitz(tau, _topology.beta(), context.rowDecoderRampOutput).delay;
        bitlineDelay = std::max(bitlineDelay, tau * 20.0); // TODO revisit, corrects unnecessary horowitz calculation
        timing.bitline = bitlineDelay;
        timing.latency[Operation::Read] = bitlineDelay + context.bitlineMuxReadLatency;
        // Erase is reset and programming is set, programming is used as the write latency
        // TODO no erase or programming time, so reset and set only wait for the decoders and the charge
        timing.latency[Operation::Reset] = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency + context.chargeLatency);
        timing.latency[Operation::Set] = std::max(context.rowDecoderReadLatency, context.columnDecoderLatency + context.chargeLatency);
        timing.latency[Operation::Write] = timing.latency[Operation::Set];

        return timing;
    }

    // TODO no energy model for CTT cells, kept from the original model where they fell through to an empty MLC NAND case
    // Their read and write energy is only the peripheral energy the subarray adds
    CellPower power(const MemoryCellEnergyContext&) const noexcept override {
        CellPower power;
        return power;
    }
};

} // namespace mse::cell

#endif
