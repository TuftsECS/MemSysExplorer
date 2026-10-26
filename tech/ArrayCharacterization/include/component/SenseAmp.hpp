#ifndef MSE_COMPONENT_SENSEAMP_HPP
#define MSE_COMPONENT_SENSEAMP_HPP

#include "component/BasicComponent.hpp"
#include "component/BasicValues.hpp"
#include "cell/MemoryCell.hpp"
#include "Dimensions.hpp"
#include "unit.hpp"

#include <optional>

namespace mse::component {

class SenseAmp final : public BasicComponent<> {
private:

    long long _columns = 0;
    bool _currentSense = false;
    unit::Meter _senseAmpPitch;
    // The levels of multi-level cells, which need a reference for each level
    std::optional<cell::MultiLevel> _multiLevel;
    cell::SenseAmpModel _model;

public:

    unit::Volt senseVoltage;

    SenseAmp() = default;

    SenseAmp(long long columns, bool currentSense, unit::Volt senseVoltage, unit::Meter senseAmpPitch,
             std::optional<cell::MultiLevel> multiLevel, const cell::SenseAmpModel& model)
            : _columns(columns),
              _currentSense(currentSense),
              _senseAmpPitch(senseAmpPitch),
              _multiLevel(multiLevel),
              _model(model),
              senseVoltage(senseVoltage) {}

    constexpr std::string_view componentName() const noexcept override { return "Sense Amp"; }

    bool setup() noexcept override;

    const unit::Farad& loadCapacitance() const noexcept { return getValue(_loadCapacitance); }

    void reset() noexcept override {
        BasicComponent::reset();
        _loadCapacitance.reset();
    }

    Dimensions calculateDimensions() const noexcept override;
    BasicTiming calculateTiming() const noexcept override;
    BasicPower calculatePower() const noexcept override;

private:

    unit::Farad calculateLoadCapacitance() const noexcept;

    Lazy<&SenseAmp::calculateLoadCapacitance> _loadCapacitance;
};

} // namespace mse::component

#endif
