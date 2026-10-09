#ifndef MSE_COMPONENT_BASICDECODER_HPP
#define MSE_COMPONENT_BASICDECODER_HPP

#include "component/RampedComponent.hpp"
#include "component/OutputDriver.hpp"
#include "component/BasicValues.hpp"
#include "physics/Horowitz.hpp"
#include "Dimensions.hpp"
#include "unit.hpp"

namespace mse::component {

class BasicDecoder final : public RampedComponent<> {
private:

    int _addressBits = 0;
    unit::Farad _loadCapacitance;
    unit::Ohm _loadResistance;

    OutputDriver _outputDriver;

public:

    BasicDecoder() = default;

    BasicDecoder(int addressBits, unit::Farad loadCapacitance, unit::Ohm loadResistance);

    constexpr std::string_view componentName() const noexcept override { return "Basic Decoder"; }

    void forEachSubcomponent(const SubcomponentVisitor& visit) noexcept override { visit(_outputDriver); }

    bool setup() noexcept override;

    void reset() noexcept override {
        RampedComponent::reset();
        _outputDriver.reset();
        _nandOutputCapacitance.reset();
    }

    Dimensions calculateDimensions() const noexcept override;
    RampedTiming calculateTiming(physics::HorowitzRamp inputRamp) const noexcept override;
    BasicPower calculatePower() const noexcept override;

private:

    int nandInputs() const noexcept { return _addressBits == 1 ? 0 : _addressBits; }
    int nandGates() const noexcept { return nandInputs() == 0 ? 0 : 1 << nandInputs(); }

    const unit::Farad& nandOutputCapacitance() const noexcept { return getValue(_nandOutputCapacitance); }
    unit::Farad calculateNandOutputCapacitance() const noexcept;

    Lazy<&BasicDecoder::calculateNandOutputCapacitance> _nandOutputCapacitance;
};

} // namespace mse::component

#endif
