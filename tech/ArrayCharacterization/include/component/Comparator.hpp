#ifndef MSE_COMPONENT_COMPARATOR_HPP
#define MSE_COMPONENT_COMPARATOR_HPP

#include "component/RampedComponent.hpp"
#include "component/BasicValues.hpp"
#include "physics/Horowitz.hpp"
#include "Dimensions.hpp"
#include "constant.hpp"
#include "unit.hpp"

#include <array>

namespace mse::component {

class Comparator final : public RampedComponent<> {
private:

    int _tagBits = 0;
    unit::Farad _loadCapacitance;

public:

    Comparator() = default;

    Comparator(int tagBits, unit::Farad loadCapacitance)
            : _tagBits(tagBits),
              _loadCapacitance(loadCapacitance) {}

    constexpr std::string_view componentName() const noexcept override { return "Comparator"; }

    void reset() noexcept override {
        RampedComponent::reset();
        _rc.reset();
    }

    Dimensions calculateDimensions() const noexcept override;
    RampedTiming calculateTiming(physics::HorowitzRamp inputRamp) const noexcept override;
    BasicPower calculatePower() const noexcept override;

private:

    static constexpr int chainLength = mse::constant::comparatorInvChainLength;

    struct RC {
        std::array<unit::Farad, chainLength> inputCapacitances;
        std::array<unit::Farad, chainLength> outputCapacitances;
        unit::Farad bottomCapacitance;
        unit::Farad topCapacitance;
        unit::Ohm bottomResistance;
        unit::Ohm topResistance;
    };

    int quarterTagBits() const noexcept { return _tagBits / 4; }

    const RC& rc() const noexcept { return getValue(_rc); }
    RC calculateRC() const noexcept;

    Lazy<&Comparator::calculateRC> _rc;
};

} // namespace mse::component

#endif
