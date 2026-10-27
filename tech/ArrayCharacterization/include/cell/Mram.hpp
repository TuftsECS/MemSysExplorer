#ifndef MSE_CELL_MRAM_HPP
#define MSE_CELL_MRAM_HPP

#include "cell/MemoryCell.hpp"
#include "cell/Resistive.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"

namespace mse::cell {

struct MramParameters final : ResistiveCellParameters {
    MramParameters(YamlInputFile& file) : ResistiveCellParameters(file) {
        fillWriteEnergy(resistanceOn, resistanceOn);
    }
};

class MramTopology final : public ResistiveAccessTopology {
public:
    MramTopology(const MramParameters& parameters)
            : ResistiveAccessTopology(parameters) {}
};

struct MramProperties final : ResistiveCellProperties {
    MramProperties(const MramParameters& parameters, const MramTopology& topology) : ResistiveCellProperties(parameters, topology) {
        symmetricWrite = true;
    }
};

class MramCell final : public ResistiveMemoryCell<MramParameters, MramTopology, MramProperties> {
public:
    using Parameters = MramParameters;

    using ResistiveMemoryCell::ResistiveMemoryCell;

    static constexpr std::string_view name = "MRAM";
};

} // namespace mse::cell

#endif
