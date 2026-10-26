#ifndef MSE_COMPONENT_BANKWITHHTREE_HPP
#define MSE_COMPONENT_BANKWITHHTREE_HPP

#include "component/Bank.hpp"
#include "Dimensions.hpp"
#include "typedef.hpp"
#include "unit.hpp"

#include <array>

namespace mse::component {

class BankWithHtree final : public Bank {
private:

    static constexpr int maxLevels = 32;

    struct HtreeLevel {
        int addressBits = 0;
        int dataDistributeBits = 0;
        int dataBroadcastBits = 0;
        int wires = 0;
        int totalWires = 0;
        int activeWires = 0;

        int bits() const noexcept { return addressBits + dataDistributeBits + dataBroadcastBits; }
    };

    struct HtreeWires {
        int wiresSharingWidth = 1;
        unit::Meter effectivePitch;
        std::array<unit::Meter, maxLevels> horizontalLengths;
        std::array<unit::Meter, maxLevels> verticalLengths;
    };

    int _horizontalLevels = 0;
    int _verticalLevels = 0;
    std::array<HtreeLevel, maxLevels> _horizontal{};
    std::array<HtreeLevel, maxLevels> _vertical{};

public:

    BankWithHtree() = default;

    BankWithHtree(
        int matRows,
        int matColumns,
        long long capacity,
        long blockSize,
        int associativity,
        int rowsPerSet,
        int activeMatsPerRow,
        int activeMatsPerColumn,
        int bitlineMuxInputs,
        bool internalSenseAmp,
        int senseAmpMuxLevel1Inputs,
        int senseAmpMuxLevel2Inputs,
        int subarrayRows,
        int subarrayColumns,
        int activeSubarraysPerRow,
        int activeSubarraysPerColumn,
        BufferDesignTarget areaOptimizationLevel,
        MemoryType memoryType);

    bool setup() noexcept override;

    void reset() noexcept override {
        Bank::reset();
        _wires.reset();
    }

    Dimensions calculateDimensions() const noexcept override;
    PathTiming calculateTiming() const noexcept override;
    PathPower calculatePower() const noexcept override;

private:

    const HtreeWires& wires() const noexcept { return getValue(_wires); }
    HtreeWires calculateWires() const noexcept;

    Lazy<&BankWithHtree::calculateWires> _wires;
};

} // namespace mse::component

#endif
