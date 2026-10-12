#include "passDevice.hpp"
#include "constant.hpp"
#include "log.hpp"
#include "Technology.hpp"
#include "global.hpp"

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
    const Farad junction = Farad(drainArea.value() * tech.capJunction);
    const Farad channel = Farad(foldDegree * drainHeight.value() * tech.capDrainToChannel);
    const Farad sidewall = Farad(drainSidewallLength.value() * tech.capSidewall);

    return junction + channel + sidewall;
}

static mse::unit::Farad computeGateCapacitance(const Technology& tech, mse::unit::Feature gateWidth) {
    using namespace mse::unit;
    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{tech.featureSize};
    return Farad((tech.capIdealGate + tech.capOverlap + 3.0 * tech.capFringe) * (gateWidth * featureSize).value() + tech.phyGateLength * tech.capPolywire);
}

static mse::unit::Ohm computeOnResistance(const Technology& tech, const double* onCurrent, mse::unit::Feature gateWidth, int tempIndex) {
    using namespace mse::unit;
    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{tech.featureSize};
    return Ohm(tech.effectiveResistanceMultiplier * tech.vdd / (onCurrent[tempIndex] * (gateWidth * featureSize).value()));
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

    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{_tech->featureSize};
    const unsigned foldDegree = calculateFolding(_gateWidth, _maxRegionHeight);
    const Feature foldedFingerWidth = foldDegree * polyWidth + (foldDegree - 1) * diffusionRegionWidth;

    return Dimensions{
        .width = (2.0 * contactWidth + Inputs * foldedFingerWidth + (Inputs - 1) * diffusionRegionWidth) * featureSize,
        .height = std::min(_gateWidth, _maxRegionHeight) * featureSize
    };
}

template <mse::PassType Type, int Inputs>
mse::unit::Farad mse::PassStack<Type, Inputs>::drainCapacitance() const noexcept {
    using namespace mse::unit;

    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{_tech->featureSize};
    const unsigned foldDegree = calculateFolding(_gateWidth, _maxRegionHeight);

    const Feature foldedFingerDrainWidth = (foldDegree - 1) * diffusionRegionWidth;
    const Meter drainWidth = (contactWidth + Inputs * foldedFingerDrainWidth + (Inputs - 1) * diffusionRegionWidth) * featureSize;
    const Meter drainHeight = std::min(_gateWidth, _maxRegionHeight) * featureSize;
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

    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{_tech->featureSize};
    const double* offCurrent = Type == PassType::Nmos ? _tech->currentOffNmos : _tech->currentOffPmos;

    return Ampere((_gateWidth * featureSize).value() * offCurrent[_tempIndex]);
}

template <mse::PassType Type, int Inputs>
mse::unit::Ohm mse::PassStack<Type, Inputs>::resistance() const noexcept {
    const double* onCurrent = Type == PassType::Nmos ? _tech->currentOnNmos : _tech->currentOnPmos;

    return computeOnResistance(*_tech, onCurrent, _gateWidth, _tempIndex) * Inputs;
}

template class mse::PassStack<mse::PassType::Nmos, 2>;
template class mse::PassStack<mse::PassType::Nmos, 3>;
template class mse::PassStack<mse::PassType::Pmos, 2>;
template class mse::PassStack<mse::PassType::Pmos, 3>;
