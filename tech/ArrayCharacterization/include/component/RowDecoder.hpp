#ifndef MSE_COMPONENT_ROWDECODER_HPP
#define MSE_COMPONENT_ROWDECODER_HPP

#include "component/RampedComponent.hpp"
#include "component/OutputDriver.hpp"
#include "component/BasicValues.hpp"
#include "physics/Horowitz.hpp"
#include "Dimensions.hpp"
#include "typedef.hpp"
#include "unit.hpp"

namespace mse::component {

class RowDecoder final : public RampedComponent<> {
private:

    int _rows = 0;
    unit::Farad _loadCapacitance;
    unit::Ohm _loadResistance;
    bool _multipleRowsPerSet = false;
    BufferDesignTarget _areaOptimizationLevel = latency_first;
    unit::Ampere _minDriverCurrent;

    OutputDriver _outputDriver;

public:

    RowDecoder() = default;

    RowDecoder(int rows, unit::Farad loadCapacitance, unit::Ohm loadResistance, bool multipleRowsPerSet,
               BufferDesignTarget areaOptimizationLevel, unit::Ampere minDriverCurrent);

    constexpr std::string_view componentName() const noexcept override { return "Row Decoder"; }

    void forEachSubcomponent(const SubcomponentVisitor& visit) noexcept override { visit(_outputDriver); }

    bool setup() noexcept override;

    const unit::Farad& nandInputCapacitance() const noexcept { return getValue(_nandCapacitance).input; }

    void reset() noexcept override {
        RampedComponent::reset();
        _outputDriver.reset();
        _nandCapacitance.reset();
    }

    Dimensions calculateDimensions() const noexcept override;
    RampedTiming calculateTiming(physics::HorowitzRamp inputRamp) const noexcept override;
    BasicPower calculatePower() const noexcept override;

private:

    struct NandCapacitance {
        unit::Farad input;
        unit::Farad output;
    };

    int nandInputs() const noexcept;

    const unit::Farad& nandOutputCapacitance() const noexcept { return getValue(_nandCapacitance).output; }
    NandCapacitance calculateNandCapacitance() const noexcept;

    Lazy<&RowDecoder::calculateNandCapacitance> _nandCapacitance;
};

} // namespace mse::component

#endif
