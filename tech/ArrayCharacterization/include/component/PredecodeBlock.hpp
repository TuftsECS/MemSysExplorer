#ifndef MSE_COMPONENT_PREDECODEBLOCK_HPP
#define MSE_COMPONENT_PREDECODEBLOCK_HPP

#include "component/RampedComponent.hpp"
#include "component/RowDecoder.hpp"
#include "component/BasicDecoder.hpp"
#include "component/BasicValues.hpp"
#include "physics/Horowitz.hpp"
#include "Dimensions.hpp"
#include "unit.hpp"

#include <optional>

namespace mse::component {

class PredecodeBlock final : public RampedComponent<> {
private:

    int _addressBits = 0;
    unit::Farad _loadCapacitance;
    unit::Ohm _loadResistance;

    int _nandInputsStage1A = 0;
    int _nandInputsStage1B = 0;
    int _nandInputsStage1C = 0;
    int _basicDecodersA1 = 0;
    int _basicDecodersA2 = 0;

    std::optional<RowDecoder> _rowDecoderStage1A;
    std::optional<RowDecoder> _rowDecoderStage1B;
    std::optional<RowDecoder> _rowDecoderStage1C;
    std::optional<RowDecoder> _rowDecoderStage2;
    std::optional<BasicDecoder> _basicDecoderA1;
    std::optional<BasicDecoder> _basicDecoderA2;
    std::optional<BasicDecoder> _basicDecoderB;
    std::optional<BasicDecoder> _basicDecoderC;

public:

    PredecodeBlock() = default;

    PredecodeBlock(int addressBits, unit::Farad loadCapacitance, unit::Ohm loadResistance);

    constexpr std::string_view componentName() const noexcept override { return "Predecode Block"; }

    void forEachSubcomponent(const SubcomponentVisitor& visit) noexcept override;

    bool setup() noexcept override;

    void reset() noexcept override;

    Dimensions calculateDimensions() const noexcept override;
    RampedTiming calculateTiming(physics::HorowitzRamp inputRamp) const noexcept override;
    BasicPower calculatePower() const noexcept override;
};

} // namespace mse::component

#endif
