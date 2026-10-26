#ifndef MSE_CELL_EDRAM_HPP
#define MSE_CELL_EDRAM_HPP

#include "cell/Dram.hpp"

namespace mse::cell {

// Embedded DRAM uses the DRAM cell model
class EdramCell final : public DramCell {
public:
    using DramCell::DramCell;

    static constexpr std::string_view name = "eDRAM";
};

} // namespace mse::cell

#endif
