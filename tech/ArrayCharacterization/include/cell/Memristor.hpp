#ifndef MSE_CELL_MEMRISTOR_HPP
#define MSE_CELL_MEMRISTOR_HPP

#include "cell/MemoryCell.hpp"
#include "cell/Resistive.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"

namespace mse::cell {

struct MemristorParameters final : ResistiveCellParameters, MultiLevelParameters {
    MemristorParameters(YamlInputFile& file) : ResistiveCellParameters(file) {
        fillParameter(levels, senseAmpFingers);

        // Without an access device, the cell resistance while switching is taken at the write voltage
        if (accessType == none_access) {
            fillWriteEnergy(resistanceOnAtResetVoltage, resistanceOnAtSetVoltage);
        } else {
            fillWriteEnergy(resistanceOn, resistanceOn);
        }
    }
};

class MemristorTopology final : public ResistiveAccessTopology {
public:
    MemristorTopology(const MemristorParameters& parameters)
            : ResistiveAccessTopology(parameters) {}
};

struct MemristorProperties final : ResistiveCellProperties {
    MemristorProperties(const MemristorParameters& parameters, const MemristorTopology& topology) : ResistiveCellProperties(parameters, topology) {
        senseAmpNeedsBitlineMux = true;

        multiLevel = parameters.multiLevel();
        senseAmp.fittedMultiLevel = multiLevel.has_value();
    }
};

class MemristorCell final : public ResistiveMemoryCell<MemristorParameters, MemristorTopology, MemristorProperties> {
public:
    using Parameters = MemristorParameters;

    using ResistiveMemoryCell::ResistiveMemoryCell;

    static constexpr std::string_view name = "memristor";
};

} // namespace mse::cell

#endif
