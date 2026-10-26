#ifndef MSE_CELL_FBRAM_HPP
#define MSE_CELL_FBRAM_HPP

#include "cell/MemoryCell.hpp"
#include "cell/Resistive.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "constant.hpp"
#include "formula.hpp"

namespace mse::cell {

struct FbramParameters final : ResistiveCellParameters {
    ScalarParameter<unit::Feature> soiDeviceWidth{"FloatingBody.Width", "", unit::Feature(0)};
    ScalarParameter<unit::Number> gateOxideThicknessFactor{"FloatingBody.GateOxideThicknessFactor", "", 2.0}; // Thicker than a usual SOI device

    FbramParameters(YamlInputFile& file) : ResistiveCellParameters(file) {
        using namespace mse::unit;

        fillParameter(soiDeviceWidth, gateOxideThicknessFactor);
        soiDeviceWidth.value() *= ((gTech.featureSizeInNano <= 14) ? 2.0 : 1.0);

        // The floating body device is driven with both the write voltage and current
        if (resetEnergy.value() == Joule(0)) {
            resetEnergy.value() = resetMode
                ? Joule(std::fabs(resetVoltage.value().value()) * std::fabs(resetCurrent.value().value()) * resetPulse.value().value())
                : currentPulseEnergy(resetVoltage.value() == Volt(0), resetVoltage, resetCurrent, resetPulse);
        }
        if (setEnergy.value() == Joule(0)) {
            setEnergy.value() = setMode
                ? Joule(std::fabs(setVoltage.value().value()) * std::fabs(setCurrent.value().value()) * setPulse.value().value())
                : currentPulseEnergy(resetVoltage.value() == Volt(0), setVoltage, setCurrent, setPulse);
        }
    }
};

// The floating body SOI device is both the storage element and access device
// Data is read through its on/off resistance, so no access resistance is added
class FbramTopology final : public ResistiveCellTopology {
public:
    CellLineAdapter<unit::Farad> gateAdapter;
    CellLineAdapter<unit::Farad> drainAdapter;

    WordlineList<1> wordlineList = {{
        {gateAdapter}
    }};

    BitlineList<1> bitlineList = {{
        {drainAdapter}
    }};

    WordlineListSpan wordlineControlledDevices() const noexcept override {
        return wordlineList;
    }

    BitlineListSpan bitlineAccessDevices() const noexcept override {
        return bitlineList;
    }

    unit::Ohm accessResistance() const noexcept override {
        return unit::Ohm(0);
    }

    FbramTopology(const FbramParameters& parameters)
            : ResistiveCellTopology(parameters),
              gateAdapter(unit::Farad(CalculateFBRAMGateCap(soiWidth(parameters), parameters.gateOxideThicknessFactor, parameters.tech))),
              drainAdapter(unit::Farad(CalculateFBRAMDrainCap(soiWidth(parameters), parameters.tech))) {}

private:

    static double soiWidth(const FbramParameters& parameters) {
        using namespace mse::unit;

        return (parameters.soiDeviceWidth.value() * parameters.tech.get().featureSize).value();
    }
};

struct FbramProperties final : ResistiveCellProperties {
    FbramProperties(const FbramParameters& parameters, const FbramTopology& topology) : ResistiveCellProperties(parameters, topology) {
        sharedContact = true;
        senseAmpNeedsBitlineMux = true;
        // TODO the floating body cell is read through its own resistance but the mux isn't sized for voltage dividing
        maxMuxResistance = std::nullopt;
    }
};

class FbramCell final : public ResistiveMemoryCell<FbramParameters, FbramTopology, FbramProperties> {
public:
    using Parameters = FbramParameters;

    using ResistiveMemoryCell::ResistiveMemoryCell;

    static constexpr std::string_view name = "FBRAM";

    std::optional<ArrayCurrents> arrayCurrents(long long rows, long long, int) const noexcept override {
        using namespace mse::unit;

        // Bitline too long
        if (_parameters.resistanceOff.value().value() / _parameters.resistanceOn.value().value() < rows / constant::bitlineLeakageTolerance) {
            return std::nullopt;
        }

        ArrayCurrents currents;
        currents.maxBitlineCurrent = Ampere(std::max(_parameters.resetCurrent.value().value(), _parameters.setCurrent.value().value())
                + _parameters.leakageCurrentAccessDevice.value().value() * (rows - 1));

        return currents;
    }

    CellTiming timing(const MemoryCellLatencyContext& context) const noexcept override {
        ResistiveBitlineDelay bitlineDelay = resistiveBitlineDelay(_parameters, _topology, _properties, context);
        ResistiveWriteLatency writeLatency = pulsedWriteLatency(_parameters, context);

        CellTiming timing;

        timing.bitline = bitlineDelay.bitline;
        timing.bitlineOn = bitlineDelay.bitlineOn;
        timing.bitlineOff = bitlineDelay.bitlineOff;
        timing.latency[Operation::Read] = bitlineDelay.bitline + context.bitlineMuxReadLatency;
        timing.latency[Operation::Write] = writeLatency.write;
        timing.latency[Operation::Set] = writeLatency.set;
        timing.latency[Operation::Reset] = writeLatency.reset;

        return timing;
    }

    CellPower power(const MemoryCellEnergyContext& context) const noexcept override {
        ResistiveWriteEnergy writeEnergy = resistiveCellWriteEnergy(_parameters, _topology, context);

        CellPower power;

        power.dynamicEnergy[Operation::Read] = resistiveReadDynamicEnergy(_parameters, _topology, _properties, context);
        power.cellEnergy[Operation::Read] = resistiveCellReadEnergy(_parameters, context);
        // Due to the shaper inefficiency
        power.cellEnergy[Operation::Reset] = writeEnergy.reset / constant::shaperEfficiencyAggressive;
        power.cellEnergy[Operation::Set] = writeEnergy.set / constant::shaperEfficiencyAggressive;
        power.dynamicEnergy[Operation::Write] = std::max(power.cellEnergy[Operation::Reset], power.cellEnergy[Operation::Set]);

        return power;
    }
};

} // namespace mse::cell

#endif
