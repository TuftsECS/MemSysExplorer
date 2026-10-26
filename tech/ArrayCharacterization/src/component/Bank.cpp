#include "component/Bank.hpp"
#include "global.hpp"

#include <cmath>
#include <format>

mse::component::Bank::Bank(
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
    MemoryType memoryType)
        : matRows(matRows),
          matColumns(matColumns),
          capacity(capacity),
          blockSize(blockSize),
          associativity(associativity),
          rowsPerSet(rowsPerSet),
          activeMatsPerRow(activeMatsPerRow),
          activeMatsPerColumn(activeMatsPerColumn),
          bitlineMuxInputs(bitlineMuxInputs),
          internalSenseAmp(internalSenseAmp),
          senseAmpMuxLevel1Inputs(senseAmpMuxLevel1Inputs),
          senseAmpMuxLevel2Inputs(senseAmpMuxLevel2Inputs),
          subarrayRows(subarrayRows),
          subarrayColumns(subarrayColumns),
          activeSubarraysPerRow(activeSubarraysPerRow),
          activeSubarraysPerColumn(activeSubarraysPerColumn),
          areaOptimizationLevel(areaOptimizationLevel),
          memoryType(memoryType) {
    if (this->activeMatsPerRow > this->matColumns) {
        warn(std::format("{} active mats per row is more than the {} mats per row", this->activeMatsPerRow, this->matColumns));
        this->activeMatsPerRow = this->matColumns;
    }
    if (this->activeMatsPerColumn > this->matRows) {
        warn(std::format("{} active mats per column is more than the {} mats per column", this->activeMatsPerColumn, this->matRows));
        this->activeMatsPerColumn = this->matRows;
    }
    if (this->activeSubarraysPerRow > this->subarrayColumns) {
        warn(std::format("{} active subarrays per row is more than the {} subarrays per row", this->activeSubarraysPerRow, this->subarrayColumns));
        this->activeSubarraysPerRow = this->subarrayColumns;
    }
    if (this->activeSubarraysPerColumn > this->subarrayRows) {
        warn(std::format("{} active subarrays per column is more than the {} subarrays per column", this->activeSubarraysPerColumn, this->subarrayRows));
        this->activeSubarraysPerColumn = this->subarrayRows;
    }
}

void mse::component::Bank::forEachSubcomponent(const SubcomponentVisitor& visit) noexcept {
    visit(mat);
}

bool mse::component::Bank::meetsAspectRatio() const noexcept {
    // Only data arrays have the aspect ratio constraint
    if (memoryType != dataT) {
        return true;
    }

    const Dimensions& dimensions = this->dimensions();
    const double heightRatio = dimensions.height / dimensions.width;
    const double widthRatio = dimensions.width / dimensions.height;

    return !(heightRatio > constant::bankAspectRatioConstraint || widthRatio > constant::bankAspectRatioConstraint);
}

void mse::component::Bank::report(Reporter& reporter) const noexcept {
    BasicComponent::report(reporter);
    reportOperations(reporter, timing(), power());
}

void mse::component::Bank::reset() noexcept {
    BasicComponent::reset();
    forEachSubcomponent([] (Component& subcomponent) noexcept { subcomponent.reset(); });
}

bool mse::component::Bank::meetsRetention() const noexcept {
    // Only refreshing cells with a retention time, given or found from the cell leakage, have to meet it
    const std::optional<unit::Second>& retentionTime = cell::memoryCell().properties().retentionTime;
    if (!retentionTime || !(retentionTime->value() > 0) || retentionTime->value() >= constant::invalidValue) {
        return true;
    }

    return !(timing().latency[cell::Operation::Refresh].value() > retentionTime->value());
}

mse::component::Bank::MuxInputs mse::component::Bank::muxInputsForWays(int waysPerRow) const noexcept {
    MuxInputs inputs{bitlineMuxInputs, senseAmpMuxLevel1Inputs, senseAmpMuxLevel2Inputs};
    if (waysPerRow <= 1) {
        return inputs;
    }

    const int waysPerRowLog = static_cast<int>(std::log2(static_cast<double>(waysPerRow)) + 0.1);
    if (cell::memoryCell().properties().destructiveRead) {
        // Destructive reads can't be muxed before the sense amp, so only the output mux levels take the ways
        const int extraLevel2Inputs = static_cast<int>(std::pow(2, waysPerRowLog / 2));
        const int extraLevel1Inputs = waysPerRow / extraLevel2Inputs;
        inputs.senseAmpLevel1 *= extraLevel1Inputs;
        inputs.senseAmpLevel2 *= extraLevel2Inputs;
    } else {
        const int extraLevel2Inputs = static_cast<int>(std::pow(2, waysPerRowLog / 3));
        const int extraLevel1Inputs = extraLevel2Inputs;
        const int extraBitlineInputs = waysPerRow / extraLevel1Inputs / extraLevel2Inputs;
        inputs.bitline *= extraBitlineInputs;
        inputs.senseAmpLevel1 *= extraLevel1Inputs;
        inputs.senseAmpLevel2 *= extraLevel2Inputs;
    }

    return inputs;
}
