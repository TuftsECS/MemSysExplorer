#ifndef MSE_COMPONENT_PRECHARGER_HPP
#define MSE_COMPONENT_PRECHARGER_HPP

#include "component/RampedComponent.hpp"
#include "component/OutputDriver.hpp"
#include "component/BasicValues.hpp"
#include "physics/Horowitz.hpp"
#include "Dimensions.hpp"
#include "unit.hpp"

namespace mse::component {

class Precharger final : public RampedComponent<> {
private:

    unit::Volt _prechargeVoltage;
    long long _columns = 0;
    unit::Farad _bitlineCapacitance;
    unit::Ohm _bitlineResistance;

    OutputDriver _outputDriver;

public:

    Precharger() = default;

    Precharger(unit::Volt prechargeVoltage, long long columns, unit::Farad bitlineCapacitance, unit::Ohm bitlineResistance);

    constexpr std::string_view componentName() const noexcept override { return "Precharger"; }

    void forEachSubcomponent(const SubcomponentVisitor& visit) noexcept override { visit(_outputDriver); }

    bool setup() noexcept override;

    void reset() noexcept override {
        RampedComponent::reset();
        _outputDriver.reset();
        _rc.reset();
    }

    Dimensions calculateDimensions() const noexcept override;
    RampedTiming calculateTiming(physics::HorowitzRamp inputRamp) const noexcept override;
    BasicPower calculatePower() const noexcept override;

private:

    struct RC {
        unit::Farad inverterLoadCapacitance;
        unit::Farad prechargerOutputCapacitance;
    };

    const RC& rc() const noexcept { return getValue(_rc); }
    RC calculateRC() const noexcept;

    Lazy<&Precharger::calculateRC> _rc;
};

} // namespace mse::component

#endif
