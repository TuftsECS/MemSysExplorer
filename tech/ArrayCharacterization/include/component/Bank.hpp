#ifndef MSE_COMPONENT_BANK_HPP
#define MSE_COMPONENT_BANK_HPP

#include "component/BasicComponent.hpp"
#include "component/Mat.hpp"
#include "component/BasicValues.hpp"
#include "Dimensions.hpp"
#include "typedef.hpp"
#include "unit.hpp"

namespace mse::component {

struct BankTiming : BasicTiming {
    unit::Second setLatency;
    unit::Second resetLatency;
    unit::Second refreshLatency;
};

struct BankPower : BasicPower {
    unit::Joule setDynamicEnergy;
    unit::Joule resetDynamicEnergy;
    unit::Joule refreshDynamicEnergy;
    unit::Joule cellReadEnergy;
    unit::Joule cellSetEnergy;
    unit::Joule cellResetEnergy;
};

class Bank : public BasicComponent<BankTiming, BankPower> {
public:

    int matRows = 0;
    int matColumns = 0;
    long long capacity = 0;
    long blockSize = 0;
    int associativity = 0;
    int rowsPerSet = 0;
    int activeMatsPerRow = 0;
    int activeMatsPerColumn = 0;
    int bitlineMuxInputs = 1;
    bool internalSenseAmp = false;
    int senseAmpMuxLevel1Inputs = 1;
    int senseAmpMuxLevel2Inputs = 1;
    int subarrayRows = 0;
    int subarrayColumns = 0;
    int activeSubarraysPerRow = 0;
    int activeSubarraysPerColumn = 0;
    BufferDesignTarget areaOptimizationLevel = latency_first;
    MemoryType memoryType = dataT;

    Mat mat;

    constexpr std::string_view componentName() const noexcept override { return "Bank"; }

    void forEachSubcomponent(const SubcomponentVisitor& visit) noexcept override;

    void report(Reporter& reporter) const noexcept override;

    void reset() noexcept override;

    // EDRAM has to refresh every row within the cell retention time
    bool meetsRetention() const noexcept;

    // Data banks cannot be much longer in one direction than the other
    bool meetsAspectRatio() const noexcept;

protected:

    struct MuxInputs {
        int bitline;
        int senseAmpLevel1;
        int senseAmpLevel2;
    };

    Bank() = default;

    Bank(
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

    // Extra mux levels that let one row hold several cache ways
    MuxInputs muxInputsForWays(int waysPerRow) const noexcept;
};

} // namespace mse::component

#endif
