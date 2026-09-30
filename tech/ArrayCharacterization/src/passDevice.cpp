#include "passDevice.hpp"
#include "constant.hpp"
#include "log.hpp"
#include "Technology.hpp"
#include "global.hpp"

static constexpr mse::unit::Feature contactWidth = mse::constant::contactSize + mse::constant::minimumGapBetweenContactAndPoly * 2.0;
static constexpr mse::unit::Feature diffusionRegionWidth = mse::constant::minimumGapBetweenPoly;
static constexpr mse::unit::Feature polyWidth(1.0);

static unsigned calculateFolding(mse::unit::Feature regionHeight, mse::unit::Feature maxRegionHeight) {
    using namespace mse::unit;
    if (regionHeight >= maxRegionHeight) {
        if (maxRegionHeight < 3.0_feat) {
            mse::log::fatal("Unable to do CMOS folding because size limitation is less than 3F");
        }
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
    const Meter featureSize(tech.featureSize);
    return Farad((tech.capIdealGate + tech.capOverlap + 3.0 * tech.capFringe) * gateWidth.toMeters(featureSize).value() + tech.phyGateLength * tech.capPolywire);
}

static mse::unit::Ohm computeOnResistance(const Technology& tech, const double* onCurrent, mse::unit::Feature gateWidth, int tempIndex) {
    using namespace mse::unit;
    const Meter featureSize(tech.featureSize);
    return Ohm(tech.effectiveResistanceMultiplier * tech.vdd / (onCurrent[tempIndex] * gateWidth.toMeters(featureSize).value()));
}

template <mse::PassType Type, int Inputs>
mse::PassStack<Type, Inputs>::PassStack(const Technology& tech, unit::Feature gateWidth, unit::Feature maxRegionHeight) {
    using namespace mse::unit;

    const Meter featureSize(tech.featureSize);
    const unsigned foldDegree = calculateFolding(gateWidth, maxRegionHeight);

    const Feature foldedFingerWidth = foldDegree * polyWidth + (foldDegree - 1) * diffusionRegionWidth;
    _dimensions = {
        .width = (2.0 * contactWidth + Inputs * foldedFingerWidth + (Inputs - 1) * diffusionRegionWidth).toMeters(featureSize),
        .height = std::min(gateWidth, maxRegionHeight).toMeters(featureSize)
    };

    const Feature foldedFingerDrainWidth = (foldDegree - 1) * diffusionRegionWidth;
    const Meter drainWidth = (contactWidth + Inputs * foldedFingerDrainWidth + (Inputs - 1) * diffusionRegionWidth).toMeters(featureSize);
    const Meter drainHeight = _dimensions.height;
    const MeterExp<2> drainArea = drainWidth * drainHeight;
    _drainCapacitance = computeDrainCapacitance(tech, drainArea, drainWidth, drainHeight, foldDegree);

    _gateCapacitance = computeGateCapacitance(tech, gateWidth) * Inputs;

    int tempIndex = gInputParameter.temperature - 300;
    if (tempIndex > 100 || tempIndex < 0) {
        log::fatal("Temperature is out of range");
    }

    const double* onCurrent = Type == PassType::Nmos ? tech.currentOnNmos : tech.currentOnPmos;
    const double* offCurrent = Type == PassType::Nmos ? tech.currentOffNmos : tech.currentOffPmos;
    _resistance = computeOnResistance(tech, onCurrent, gateWidth, tempIndex) * Inputs;

    _leakageCurrent = Ampere(gateWidth.toMeters(featureSize).value() * offCurrent[tempIndex]);
}

template class mse::PassStack<mse::PassType::Nmos, 2>;
template class mse::PassStack<mse::PassType::Nmos, 3>;
template class mse::PassStack<mse::PassType::Pmos, 2>;
template class mse::PassStack<mse::PassType::Pmos, 3>;
