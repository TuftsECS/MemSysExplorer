#ifndef MSE_CELL_FEFET_HPP
#define MSE_CELL_FEFET_HPP

#include "cell/MemoryCell.hpp"
#include "cell/Resistive.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "formula.hpp"

namespace mse::cell {

struct FefetParameters final : ResistiveCellParameters, MultiLevelParameters {
    FefetParameters(YamlInputFile& file) : ResistiveCellParameters(file) {
        fillParameter(levels, senseAmpFingers);

        // Without an access device, the cell resistance while switching is taken at the write voltage
        if (accessType == none_access) {
            fillWriteEnergy(resistanceOnAtResetVoltage, resistanceOnAtSetVoltage);
        } else {
            fillWriteEnergy(resistanceOn, resistanceOn);
        }
    }
};

class FefetTopology final : public ResistiveAccessTopology {
public:
    FefetTopology(const FefetParameters& parameters)
            : ResistiveAccessTopology(parameters, ferroelectricGateCapacitance(parameters)) {}

private:

    // The ferroelectric gate of the access transistor is the wordline load
    static unit::Farad ferroelectricGateCapacitance(const FefetParameters& parameters) {
        using namespace mse::unit;

        const Technology& tech = parameters.tech;
        const Meter gateWidth = parameters.accessNmosGateWidth.value() * tech.featureSize;

        return Farad(CalculateFeFETGateCap(gateWidth.value(), tech));
    }
};

struct FefetProperties final : ResistiveCellProperties {
    FefetProperties(const FefetParameters& parameters, const FefetTopology& topology) : ResistiveCellProperties(parameters, topology) {
        using namespace mse::unit;

        senseAmpNeedsBitlineMux = true;

        // TODO the sense amps only have measured values for single level cells and 4 or 8 levels, other levels use the general model
        multiLevel = parameters.multiLevel();
        if (!multiLevel) {
            senseAmp.readLatency = 1.2e-10_s;
            senseAmp.readEnergy = 7.3e-16_J;
        } else if (multiLevel->levels == 4) {
            senseAmp.readLatency = 4.5e-10_s;
            senseAmp.readEnergy = 1.092e-14_J;
        } else if (multiLevel->levels == 8) {
            senseAmp.readLatency = 7.0e-10_s;
            senseAmp.readEnergy = 3.823e-14_J;
        }
    }
};

class FefetCell final : public ResistiveMemoryCell<FefetParameters, FefetTopology, FefetProperties> {
public:
    using Parameters = FefetParameters;

    using ResistiveMemoryCell::ResistiveMemoryCell;

    static constexpr std::string_view name = "FeFET";
};

} // namespace mse::cell

#endif
