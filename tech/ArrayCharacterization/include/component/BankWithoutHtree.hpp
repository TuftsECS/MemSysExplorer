#ifndef MSE_COMPONENT_BANKWITHOUTHTREE_HPP
#define MSE_COMPONENT_BANKWITHOUTHTREE_HPP

#include "component/Bank.hpp"
#include "component/Comparator.hpp"
#include "component/SenseAmp.hpp"
#include "component/Mux.hpp"
#include "Dimensions.hpp"
#include "typedef.hpp"
#include "unit.hpp"

#include <optional>

namespace mse::component {

class BankWithoutHtree final : public Bank {
private:

    int _matAddressBits = 0;
    long _matDataBits = 0;
    int _ways = 1;

    // Only with external sensing
    std::optional<Mux> _globalBitlineMux;
    std::optional<SenseAmp> _globalSenseAmp;
    std::optional<Comparator> _globalComparator;

public:

    BankWithoutHtree() = default;

    BankWithoutHtree(
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

    void forEachSubcomponent(const SubcomponentVisitor& visit) noexcept override;

    bool setup() noexcept override;

    Dimensions calculateDimensions() const noexcept override;
    BankTiming calculateTiming() const noexcept override;
    BankPower calculatePower() const noexcept override;
};

} // namespace mse::component

#endif
