#ifndef MSE_COMPONENT_OUTPUTDRIVER_HPP
#define MSE_COMPONENT_OUTPUTDRIVER_HPP

#include "component/RampedComponent.hpp"
#include "component/BasicValues.hpp"
#include "physics/Horowitz.hpp"
#include "Dimensions.hpp"
#include "constant.hpp"
#include "typedef.hpp"
#include "unit.hpp"

#include <array>
#include <span>

namespace mse::component {

struct OutputDriverStage {
    unit::Meter nmosWidth;
    unit::Meter pmosWidth;
    unit::Farad inputCapacitance;
    unit::Farad outputCapacitance;
};

struct OutputDriverChain {
    std::array<OutputDriverStage, constant::maxInvChainLength + 1> stages;
    size_t size = 0;
};

class OutputDriver final : public RampedComponent<> {
private:

    double _logicEffort = 0;
    unit::Farad _inputCapacitance;
    unit::Farad _outputCapacitance;
    unit::Ohm _outputResistance;
    bool _inverted = false;
    BufferDesignTarget _areaOptimizationLevel = latency_first;
    unit::Ampere _minDriverCurrent;

public:

    OutputDriver() = default;

    OutputDriver(double logicEffort, unit::Farad inputCapacitance, unit::Farad outputCapacitance, unit::Ohm outputResistance, bool inverted, BufferDesignTarget areaOptimizationLevel, unit::Ampere minDriverCurrent)
            : _logicEffort(logicEffort),
              _inputCapacitance(inputCapacitance),
              _outputCapacitance(outputCapacitance),
              _outputResistance(outputResistance),
              _inverted(inverted),
              _areaOptimizationLevel(areaOptimizationLevel),
              _minDriverCurrent(minDriverCurrent) {}

    constexpr std::string_view componentName() const noexcept override { return "Output Driver"; }

    bool setup() noexcept override { return !stages().empty(); }

    std::span<const OutputDriverStage> stages() const noexcept {
        const OutputDriverChain& chain = getValue(_chain);
        return {chain.stages.data(), chain.size};
    }

    void reset() noexcept override {
        RampedComponent::reset();
        _chain.reset();
    }

    Dimensions calculateDimensions() const noexcept override;
    RampedTiming calculateTiming(physics::HorowitzRamp inputRamp) const noexcept override;
    BasicPower calculatePower() const noexcept override;

private:

    OutputDriverChain calculateChain() const noexcept;

    Lazy<&OutputDriver::calculateChain> _chain;
};

} // namespace mse::component

#endif
