#ifndef MSE_COMPONENT_MAT_HPP
#define MSE_COMPONENT_MAT_HPP

#include "component/RampedComponent.hpp"
#include "component/SubArray.hpp"
#include "component/PredecodeBlock.hpp"
#include "component/Comparator.hpp"
#include "component/BasicValues.hpp"
#include "component/CellPath.hpp"
#include "physics/Horowitz.hpp"
#include "Dimensions.hpp"
#include "typedef.hpp"
#include "unit.hpp"

#include <optional>

namespace mse::component {

struct MatTiming : PathTiming {
    unit::Second predecoderLatency;
};

class Mat final : public RampedComponent<MatTiming, PathPower>, public CellPath {
private:

    int _subarrayRows = 0;
    int _subarrayColumns = 0;
    int _addressBits = 0;
    long _dataBits = 0;
    int _ways = 0;
    int _rowsPerSet = 0;
    int _activeSubarraysPerRow = 0;
    int _activeSubarraysPerColumn = 0;
    int _bitlineMuxInputs = 1;
    bool _internalSenseAmp = false;
    int _senseAmpMuxLevel1Inputs = 1;
    int _senseAmpMuxLevel2Inputs = 1;
    BufferDesignTarget _areaOptimizationLevel = latency_first;
    MemoryType _memoryType = dataT;

    PredecodeBlock _rowPredecoder1;
    PredecodeBlock _rowPredecoder2;
    PredecodeBlock _bitlineMuxPredecoder1;
    PredecodeBlock _bitlineMuxPredecoder2;
    PredecodeBlock _senseAmpMuxLevel1Predecoder1;
    PredecodeBlock _senseAmpMuxLevel1Predecoder2;
    PredecodeBlock _senseAmpMuxLevel2Predecoder1;
    PredecodeBlock _senseAmpMuxLevel2Predecoder2;

public:

    SubArray subarray;
    std::optional<Comparator> comparator;

    Mat() = default;

    Mat(int subarrayRows, int subarrayColumns, int addressBits, long dataBits, int ways, int rowsPerSet, int activeSubarraysPerRow, int activeSubarraysPerColumn, int bitlineMuxInputs, bool internalSenseAmp, int senseAmpMuxLevel1Inputs, int senseAmpMuxLevel2Inputs, BufferDesignTarget areaOptimizationLevel, MemoryType memoryType);

    constexpr std::string_view componentName() const noexcept override { return "Mat"; }

    void forEachSubcomponent(const SubcomponentVisitor& visit) noexcept override;

    bool setup() noexcept override;

    void report(Reporter& reporter) const noexcept override;

    void reset() noexcept override;

    // The operations of the subarray's cell
    cell::OperationSet operations() const noexcept override { return subarray.operations(); }

    Dimensions calculateDimensions() const noexcept override;
    MatTiming calculateTiming(physics::HorowitzRamp inputRamp) const noexcept override;
    PathPower calculatePower() const noexcept override;
};

} // namespace mse::component

#endif
