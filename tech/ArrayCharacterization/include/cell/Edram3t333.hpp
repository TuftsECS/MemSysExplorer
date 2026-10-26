#ifndef MSE_CELL_EDRAM3T333_HPP
#define MSE_CELL_EDRAM3T333_HPP

#include "cell/Edram3t.hpp"

namespace mse::cell {

// The monolithic 3D 333 eDRAM cell is a 3T eDRAM cell stacked over the peripherals
class Edram3t333Cell final : public Edram3tCell {
public:
    Edram3t333Cell(const Parameters& parameters) : Edram3tCell(parameters) {
        _properties.stackedOverPeripherals = true;
    }

    static constexpr std::string_view name = "eDRAM3T333";
};

} // namespace mse::cell

#endif
