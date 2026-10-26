#ifndef MSE_COMPONENT_SUBARRAY_HPP
#define MSE_COMPONENT_SUBARRAY_HPP

#include "component/RampedComponent.hpp"
#include "component/RowDecoder.hpp"
#include "component/Precharger.hpp"
#include "component/SenseAmp.hpp"
#include "component/Buffer.hpp"
#include "component/Mux.hpp"
#include "component/BasicValues.hpp"
#include "component/CellPath.hpp"
#include "physics/Horowitz.hpp"
#include "cell/types.hpp"
#include "Dimensions.hpp"
#include "typedef.hpp"
#include "unit.hpp"

#include <optional>

namespace mse::component {

struct SubArrayTiming : PathTiming {
    unit::Second bitlineLatency;
    unit::Second bitlineReadLatency;
    unit::Second bitlineWriteLatency;
    unit::Second bitlineOnLatency;
    unit::Second bitlineOffLatency;
    unit::Second chargeLatency;
    unit::Second columnDecoderLatency;
};

struct SubArrayPower : PathPower {
    // Energy spent in the cells themselves, which is part of each operation's dynamic energy
    cell::OperationValues<unit::Joule> cellEnergy;
    // The row decoder energy of each operation, replaced by the cell's own when it drives wordlines past vdd
    cell::OperationValues<unit::Joule> rowDecoderEnergy;
};

struct SubArrayRC {
    unit::Farad wordlineCapacitance;
    unit::Farad readWordlineCapacitance; // Cells with a separate read wordline, which has its own row decoder
    unit::Ohm wordlineResistance;
    unit::Farad bitlineCapacitance;
    unit::Ohm bitlineResistance;
    unit::Ohm cellOnResistance;
    unit::Ohm cellOffResistance;
    unit::Ohm equivalentOnResistance;
    unit::Ohm equivalentOffResistance;
    unit::Volt cellOnVoltage;
    unit::Volt cellOffVoltage;
    unit::Volt prechargeVoltage;
};

class SubArray final : public RampedComponent<SubArrayTiming, SubArrayPower>, public CellPath {
private:

    bool _multipleRowsPerSet = false;
    int _bitlineMuxInputs = 1;
    bool _internalSenseAmp = false;
    int _senseAmpMuxLevel1Inputs = 1;
    int _senseAmpMuxLevel2Inputs = 1;
    BufferDesignTarget _areaOptimizationLevel = latency_first;

    bool _voltageSense = false;
    unit::Volt _senseVoltage;
    unit::Meter _wordlineLength;
    unit::Meter _bitlineLength;
    unit::Ampere _maxBitlineCurrent;
    SubArrayRC _rc;

public:

    long long rows = 0;
    long long columns = 0;

    RowDecoder rowDecoder;
    std::optional<RowDecoder> readRowDecoder; // Cells with a separate read wordline
    RowDecoder bitlineMuxDecoder;
    RowDecoder senseAmpMuxLevel1Decoder;
    RowDecoder senseAmpMuxLevel2Decoder;
    std::optional<SenseAmp> senseAmp;
    Precharger precharger;
    Precharger writeDriver; // Write bitline drivers, modelled as a precharger (as in NS-Cache)
    Mux bitlineMux;
    Mux senseAmpMuxLevel1;
    Mux senseAmpMuxLevel2;
    std::optional<Buffer> buffer; // Differential write buffer, an NVM write-reduction circuit

    SubArray() = default;

    SubArray(long long rows,
        long long columns,
        bool multipleRowsPerSet,
        int bitlineMuxInputs,
        bool internalSenseAmp,
        int senseAmpMuxLevel1Inputs,
        int senseAmpMuxLevel2Inputs,
        BufferDesignTarget areaOptimizationLevel);

    constexpr std::string_view componentName() const noexcept override { return "Subarray"; }

    void forEachSubcomponent(const SubcomponentVisitor& visit) noexcept override;

    bool setup() noexcept override;

    void report(Reporter& reporter) const noexcept override;

    void reset() noexcept override;

    // The operations the cell supports
    cell::OperationSet operations() const noexcept override;

    const SubArrayRC& rc() const noexcept { return _rc; }

    Dimensions calculateDimensions() const noexcept override;
    SubArrayTiming calculateTiming(physics::HorowitzRamp inputRamp) const noexcept override;
    SubArrayPower calculatePower() const noexcept override;
};

} // namespace mse::component

#endif
