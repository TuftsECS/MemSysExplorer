#ifndef MSE_COMPONENT_MUX_HPP
#define MSE_COMPONENT_MUX_HPP

#include "component/BasicComponent.hpp"
#include "component/BasicValues.hpp"
#include "Dimensions.hpp"
#include "unit.hpp"

#include <optional>

namespace mse::component {

struct MuxRC {
    unit::Farad passCapacitance;
    unit::Ohm passResistance;
    unit::Farad outputCapacitance;
    unit::Farad delayLoadCapacitance;
    unit::Farad powerLoadCapacitance;
};

class Mux final : public BasicComponent<> {
private:

    int _inputs = 0;
    long long _muxes = 0;
    unit::Farad _loadCapacitance;
    unit::Farad _nextStageCapacitance;
    unit::Ampere _minDriverCurrent;
    // The pass transistors are sized to stay below this resistance when it is given
    std::optional<unit::Ohm> _maxResistance;

public:

    Mux() = default;

    Mux(int inputs, long long muxes, unit::Farad loadCapacitance, unit::Farad nextStageCapacitance, unit::Ampere minDriverCurrent,
        std::optional<unit::Ohm> maxResistance)
            : _inputs(inputs),
              _muxes(muxes),
              _loadCapacitance(loadCapacitance),
              _nextStageCapacitance(nextStageCapacitance),
              _minDriverCurrent(minDriverCurrent),
              _maxResistance(maxResistance) {}

    constexpr std::string_view componentName() const noexcept override { return "Mux"; }

    const MuxRC& rc() const noexcept { return getValue(_rc); }

    void reset() noexcept override {
        BasicComponent::reset();
        _passTransistorWidth.reset();
        _rc.reset();
    }

    Dimensions calculateDimensions() const noexcept override;
    BasicTiming calculateTiming() const noexcept override;
    BasicPower calculatePower() const noexcept override;

private:

    bool enabled() const noexcept { return _inputs > 1 && _muxes > 0; } // TODO remove

    const unit::Meter& passTransistorWidth() const noexcept { return getValue(_passTransistorWidth); }

    unit::Meter calculatePassTransistorWidth() const noexcept;
    MuxRC calculateRC() const noexcept;

    Lazy<&Mux::calculatePassTransistorWidth> _passTransistorWidth;
    Lazy<&Mux::calculateRC> _rc;
};

} // namespace mse::component

#endif
