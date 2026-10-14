#include "passDevice.hpp"
#include "constant.hpp"
#include "log.hpp"
#include "Technology.hpp"
#include "global.hpp"
#include "formula.hpp"

static constexpr mse::unit::Feature contactWidth = mse::constant::contactSize + mse::constant::minimumGapBetweenContactAndPoly * 2.0;
static constexpr mse::unit::Feature diffusionRegionWidth = mse::constant::minimumGapBetweenPoly;
static constexpr mse::unit::Feature polyWidth(1.0);

// Folding needs at least 3F of region height for the folding overhead
static void checkFolding(mse::unit::Feature regionHeight, mse::unit::Feature maxRegionHeight) {
    using namespace mse::unit;
    if (regionHeight >= maxRegionHeight && maxRegionHeight < 3.0_feat) {
        mse::log::fatal("Unable to do CMOS folding because size limitation is less than 3F");
    }
}

// Calculates folding for a transistor region and checked by checkFolding during construction
static unsigned calculateFolding(mse::unit::Feature regionHeight, mse::unit::Feature maxRegionHeight) {
    using namespace mse::unit;
    if (regionHeight >= maxRegionHeight) {
        return static_cast<unsigned>(std::ceil(regionHeight / (maxRegionHeight - 3.0_feat))); // 3F for folding overhead
    }
    return 1;
}

static mse::unit::Farad computeDrainCapacitance(const Technology& tech, mse::unit::MeterExp<2> drainArea, mse::unit::Meter drainWidth, mse::unit::Meter drainHeight, unsigned foldDegree) {
    using namespace mse::unit;

    Meter drainSidewallLength = 2.0 * drainWidth;
    if (foldDegree % 2 == 1) {
        drainSidewallLength += drainHeight;
    }
    const Farad junction = drainArea * tech.capJunction;
    const Farad channel = foldDegree * drainHeight * tech.capDrainToChannel;
    const Farad sidewall = drainSidewallLength * tech.capSidewall;

    return junction + channel + sidewall;
}

// Below 22nm a transistor only conducts through whole fins or nanosheet stacks
static mse::unit::Meter effectiveWidth(const Technology& tech, mse::unit::Feature gateWidth) {
    return mse::unit::Meter(CalculateEffectiveWidth((gateWidth * tech.featureSize).value(), tech));
}

static mse::unit::Farad computeGateCapacitance(const Technology& tech, mse::unit::Feature gateWidth) {
    using namespace mse::unit;
    return (tech.capIdealGate + tech.capOverlap + 3.0 * tech.capFringe) * effectiveWidth(tech, gateWidth) + tech.phyGateLength * tech.capPolywire;
}

static mse::unit::Ohm computeOnResistance(const Technology& tech, const Technology::TemperatureTable& onCurrent, mse::unit::Feature gateWidth, int tempIndex) {
    using namespace mse::unit;
    return tech.effectiveResistanceMultiplier * tech.vdd / (onCurrent[tempIndex] * effectiveWidth(tech, gateWidth));
}

template <mse::PassType Type, int Inputs>
mse::PassStack<Type, Inputs>::PassStack(const Technology& tech, unit::Feature gateWidth, unit::Feature maxRegionHeight)
        : _tech(&tech),
          _gateWidth(gateWidth),
          _maxRegionHeight(maxRegionHeight),
          _tempIndex(gInputParameter.temperature - 300) {
    if (_tempIndex > 100 || _tempIndex < 0) {
        log::fatal("Temperature is out of range");
    }

    checkFolding(_gateWidth, _maxRegionHeight);
}

template <mse::PassType Type, int Inputs>
mse::Dimensions mse::PassStack<Type, Inputs>::dimensions() const noexcept {
    using namespace mse::unit;

    if (!IsPlanar(*_tech)) {
        // FinFET and GAA stacks are laid out as a gate missing its other half, a NAND for NMOS and a NOR for PMOS
        const double width = (_gateWidth * _tech->featureSize).value();
        double height = 0;
        double stackWidth = 0;
        if constexpr (Type == PassType::Nmos) {
            CalculateFinFETGateArea(::GateType::NAND, Inputs, width, 0, (_maxRegionHeight * _tech->featureSize).value(), *_tech, &height, &stackWidth);
        } else {
            CalculateFinFETGateArea(::GateType::NOR, Inputs, 0, width, (_maxRegionHeight * _tech->featureSize).value(), *_tech, &height, &stackWidth);
        }

        return Dimensions{
            .width = Meter(stackWidth),
            .height = Meter(height)
        };
    }

    const unsigned foldDegree = calculateFolding(_gateWidth, _maxRegionHeight);
    const Feature foldedFingerWidth = foldDegree * polyWidth + (foldDegree - 1) * diffusionRegionWidth;

    return Dimensions{
        .width = (2.0 * contactWidth + Inputs * foldedFingerWidth + (Inputs - 1) * diffusionRegionWidth) * _tech->featureSize,
        .height = std::min(_gateWidth, _maxRegionHeight) * _tech->featureSize
    };
}

template <mse::PassType Type, int Inputs>
mse::unit::Farad mse::PassStack<Type, Inputs>::drainCapacitance() const noexcept {
    using namespace mse::unit;

    if (!IsPlanar(*_tech)) {
        return effectiveWidth(*_tech, _gateWidth) * _tech->capDrainTotal;
    }

    const unsigned foldDegree = calculateFolding(_gateWidth, _maxRegionHeight);

    const Feature foldedFingerDrainWidth = (foldDegree - 1) * diffusionRegionWidth;
    const Meter drainWidth = (contactWidth + Inputs * foldedFingerDrainWidth + (Inputs - 1) * diffusionRegionWidth) * _tech->featureSize;
    const Meter drainHeight = std::min(_gateWidth, _maxRegionHeight) * _tech->featureSize;
    const MeterExp<2> drainArea = drainWidth * drainHeight;

    return computeDrainCapacitance(*_tech, drainArea, drainWidth, drainHeight, foldDegree);
}

template <mse::PassType Type, int Inputs>
mse::unit::Farad mse::PassStack<Type, Inputs>::gateCapacitance() const noexcept {
    return computeGateCapacitance(*_tech, _gateWidth) * Inputs;
}

template <mse::PassType Type, int Inputs>
mse::unit::Ampere mse::PassStack<Type, Inputs>::leakageCurrent() const noexcept {
    using namespace mse::unit;

    const Technology::TemperatureTable& offCurrent = Type == PassType::Nmos ? _tech->currentOffNmos : _tech->currentOffPmos;

    return effectiveWidth(*_tech, _gateWidth) * offCurrent[_tempIndex];
}

template <mse::PassType Type, int Inputs>
mse::unit::Ohm mse::PassStack<Type, Inputs>::resistance() const noexcept {
    const Technology::TemperatureTable& onCurrent = Type == PassType::Nmos ? _tech->currentOnNmos : _tech->currentOnPmos;

    return computeOnResistance(*_tech, onCurrent, _gateWidth, _tempIndex) * Inputs;
}

template class mse::PassStack<mse::PassType::Nmos, 2>;
template class mse::PassStack<mse::PassType::Nmos, 3>;
template class mse::PassStack<mse::PassType::Pmos, 2>;
template class mse::PassStack<mse::PassType::Pmos, 3>;
