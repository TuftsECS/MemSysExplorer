#include "cmosDevice.hpp"
#include "constant.hpp"
#include "log.hpp"
#include "utility.hpp"
#include "Technology.hpp"
#include "global.hpp"

#include <cassert>

// Common constants in this file
static constexpr mse::unit::Feature contactWidth = mse::constant::contactSize + mse::constant::minimumGapBetweenContactAndPoly * 2.0;
static constexpr mse::unit::Feature diffusionRegionWidth = mse::constant::minimumGapBetweenPoly;
static constexpr mse::unit::Feature polyWidth(1.0);

// Folding needs at least 3F of region height for the folding overhead
static void checkFolding(mse::unit::Feature regionHeight, mse::unit::Feature maxRegionHeight) {
    using namespace mse::unit;

    if (regionHeight >= maxRegionHeight && maxRegionHeight < 3.0_feat) {
        mse::log::fatal("Unable to do CMOS folding because CMOS size limitation is less than 3F");
    }
}

// Calculates folding for a transistor region and checked with checkFolding during construction
// 1 = no folding, 1 finger
static unsigned calculateFolding(mse::unit::Feature regionHeight, mse::unit::Feature maxRegionHeight) {
    using namespace mse::unit;

    if (regionHeight >= maxRegionHeight) {
        return static_cast<int>(std::ceil(regionHeight / (maxRegionHeight - 3.0_feat))); // 3F for folding overhead
    }

    return 1;
}

static int temperatureIndex() {
    int tempIndex = gInputParameter.temperature - 300;
    if (tempIndex > 100 || tempIndex < 0) {
        mse::log::fatal("Temperature is out of range");
    }

    return tempIndex;
}

static mse::unit::Farad computeDrainCapacitance(const Technology& tech, mse::unit::MeterExp<2> drainArea, mse::unit::Meter drainWidth, mse::unit::Meter drainHeight, unsigned foldDegree) {
    using namespace mse::unit;

    Meter drainSidewallLength = 2.0 * drainWidth;
    if (foldDegree % 2 == 1) {
        drainSidewallLength += drainHeight;
    }
    Farad junction{drainArea.value() * tech.capJunction};
    Farad channel{foldDegree * drainHeight.value() * tech.capDrainToChannel};
    Farad sidewall{drainSidewallLength.value() * tech.capSidewall};

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

//==================================================

mse::CmosTransistor::CmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight, const double* onCurrent, const double* offCurrent, unit::Volt vdsat, unit::Number mobility)
        : _tech(&tech),
          _gateWidth(gateWidth),
          _regionHeight(transistorRegionHeight),
          _onCurrent(onCurrent),
          _offCurrent(offCurrent),
          _vdsat(vdsat),
          _mobility(mobility),
          _tempIndex(temperatureIndex()) {
    checkFolding(_gateWidth, _regionHeight);
}

mse::Dimensions mse::CmosTransistor::dimensions() const noexcept {
    using namespace mse::unit;

    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{_tech->featureSize};
    const int fingersAfterFolding = calculateFolding(_gateWidth, _regionHeight);
    const Feature foldedFingerTotalWidth = fingersAfterFolding * polyWidth + (fingersAfterFolding - 1) * diffusionRegionWidth;

    return Dimensions{
        .width = (2.0 * contactWidth + foldedFingerTotalWidth) * featureSize,
        .height = std::min(_gateWidth, _regionHeight) * featureSize
    };
}

mse::unit::Farad mse::CmosTransistor::drainCapacitance() const noexcept {
    using namespace mse::unit;

    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{_tech->featureSize};
    const int fingersAfterFolding = calculateFolding(_gateWidth, _regionHeight);

    const Feature foldedFingerDrainWidth = (fingersAfterFolding - 1) * diffusionRegionWidth;
    const Meter drainWidth = (contactWidth + foldedFingerDrainWidth) * featureSize;
    const Meter drainHeight = std::min(_gateWidth, _regionHeight) * featureSize;
    const MeterExp<2> drainArea = drainWidth * drainHeight;

    return computeDrainCapacitance(*_tech, drainArea, drainWidth, drainHeight, fingersAfterFolding);
}

mse::unit::Farad mse::CmosTransistor::gateCapacitance() const noexcept {
    return computeGateCapacitance(*_tech, _gateWidth);
}

mse::unit::Ampere mse::CmosTransistor::leakageCurrent() const noexcept {
    using namespace mse::unit;

    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{_tech->featureSize};

    return Ampere((_gateWidth * featureSize).value() * _offCurrent[_tempIndex]);
}

mse::unit::Ohm mse::CmosTransistor::channelResistance() const noexcept {
    return computeOnResistance(*_tech, _onCurrent, _gateWidth, _tempIndex);
}

mse::unit::Siemens mse::CmosTransistor::transconductance() const noexcept {
    using namespace mse::unit;

    const Technology& tech = *_tech;
    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{tech.featureSize};

    if (featureSize.value() >= 22 * 1e-9) {
        const Meter width = _gateWidth * featureSize;
        const Volt vsat = std::min(_vdsat, Volt(tech.vdd - tech.vth));

        return Siemens((_mobility * tech.capOx) / 2 * width.value() / tech.phyGateLength * vsat.value());
    }

    if ((featureSize.value() <= 14 * 1e-9) && (featureSize.value() >= 3 * 1e-9)) {
        return Siemens(tech.gm_oncurrent * static_cast<int>(std::ceil(_gateWidth.value() / 2.0)) * (2.0 * tech.heightFin + tech.widthFin) / tech.PitchFin);
    }

    return Siemens(tech.gm_oncurrent * static_cast<int>(std::ceil(_gateWidth.value() / 2.0)) * tech.effective_width * tech.max_sheet_num / tech.max_fin_per_GAA);
}

mse::NmosTransistor::NmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight)
        : CmosTransistor(tech, gateWidth, transistorRegionHeight, tech.currentOnNmos, tech.currentOffNmos, unit::Volt(tech.vdsatNmos), unit::Number(tech.effectiveElectronMobility)) {}

mse::PmosTransistor::PmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight)
        : CmosTransistor(tech, gateWidth, transistorRegionHeight, tech.currentOnPmos, tech.currentOffPmos, unit::Volt(tech.vdsatPmos), unit::Number(tech.effectiveHoleMobility)) {}

//==================================================

template <mse::GateType Gate, int Inputs>
mse::CmosGate<Gate, Inputs>::CmosGate(const Technology& tech, unit::Feature unitNmosGateWidth, unit::Feature unitPmosGateWidth, unit::Feature gateRegionHeight) requires (Inputs > 0)
        : CmosGate(tech, Inputs, unitNmosGateWidth, unitPmosGateWidth, gateRegionHeight) {}

template <mse::GateType Gate, int Inputs>
mse::CmosGate<Gate, Inputs>::CmosGate(const Technology& tech, int inputs, unit::Feature unitNmosGateWidth, unit::Feature unitPmosGateWidth, unit::Feature gateRegionHeight)
        : _tech(&tech),
          _inputs(inputs),
          _unitNmosWidth(unitNmosGateWidth),
          _unitPmosWidth(unitPmosGateWidth),
          _tempIndex(temperatureIndex()) {
    using namespace mse::unit;

    if constexpr (Gate != GateType::Inverter) {
        assert(inputs == 2 || inputs == 3);
    }

    _nmosWidth = unitNmosGateWidth;
    _pmosWidth = unitPmosGateWidth;
    _nmosWidth *= sequentialNmos ? inputs : 1;
    _pmosWidth *= sequentialNmos ? 1 : inputs;

    _logicalEffort = (_nmosWidth + _pmosWidth) / (unitNmosGateWidth + unitPmosGateWidth);

    const Feature remainingHeight = gateRegionHeight - constant::minimumGapBetweenPandNDiffusions;
    _nmosMaxWidth = _nmosWidth / (_nmosWidth + _pmosWidth) * remainingHeight;
    _pmosMaxWidth = _pmosWidth / (_nmosWidth + _pmosWidth) * remainingHeight;

    checkFolding(_nmosWidth, _nmosMaxWidth);
    checkFolding(_pmosWidth, _pmosMaxWidth);
}

template <mse::GateType Gate, int Inputs>
typename mse::CmosGate<Gate, Inputs>::Layout mse::CmosGate<Gate, Inputs>::layout() const noexcept {
    return Layout{
        .nmosFoldDegree = static_cast<int>(calculateFolding(_nmosWidth, _nmosMaxWidth)),
        .pmosFoldDegree = static_cast<int>(calculateFolding(_pmosWidth, _pmosMaxWidth)),
        .nmosRegionHeight = std::min(_nmosWidth, _nmosMaxWidth),
        .pmosRegionHeight = std::min(_pmosWidth, _pmosMaxWidth)
    };
}

template <mse::GateType Gate, int Inputs>
mse::Dimensions mse::CmosGate<Gate, Inputs>::dimensions() const noexcept {
    using namespace mse::unit;

    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{_tech->featureSize};
    const Layout gate = layout();

    const Feature nmosFoldedFingerRegionWidth = gate.nmosFoldDegree * polyWidth + (gate.nmosFoldDegree - 1) * diffusionRegionWidth;
    const Feature pmosFoldedFingerRegionWidth = gate.pmosFoldDegree * polyWidth + (gate.pmosFoldDegree - 1) * diffusionRegionWidth;

    const Feature nmosGap = sequentialNmos ? diffusionRegionWidth : contactWidth;
    const Feature pmosGap = sequentialNmos ? contactWidth : diffusionRegionWidth;
    const Feature nmosRegionWidth = 2.0 * contactWidth + _inputs * nmosFoldedFingerRegionWidth + (_inputs - 1) * nmosGap;
    const Feature pmosRegionWidth = 2.0 * contactWidth + _inputs * pmosFoldedFingerRegionWidth + (_inputs - 1) * pmosGap;

    return Dimensions{
        .width = std::max(nmosRegionWidth, pmosRegionWidth) * featureSize,
        .height = (gate.nmosRegionHeight + gate.pmosRegionHeight + (constant::minimumGapBetweenPandNDiffusions + constant::minimumPowerRailWidth * 2.0)) * featureSize
    };
}

template <mse::GateType Gate, int Inputs>
mse::unit::Farad mse::CmosGate<Gate, Inputs>::drainCapacitance() const noexcept {
    using namespace mse::unit;

    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{_tech->featureSize};
    const Layout gate = layout();

    const Feature nmosGap = sequentialNmos ? diffusionRegionWidth : contactWidth;
    const Feature pmosGap = sequentialNmos ? contactWidth : diffusionRegionWidth;
    const Feature nmosFoldedFingerDrainWidth = (gate.nmosFoldDegree - 1) * diffusionRegionWidth;
    const Feature pmosFoldedFingerDrainWidth = (gate.pmosFoldDegree - 1) * diffusionRegionWidth;

    const Feature nmosDrainWidth = contactWidth + _inputs * nmosFoldedFingerDrainWidth + (_inputs - 1) * nmosGap;
    const Feature pmosDrainWidth = contactWidth + _inputs * pmosFoldedFingerDrainWidth + (_inputs - 1) * pmosGap;
    const Feature nmosDrainHeight = gate.nmosRegionHeight;
    const Feature pmosDrainHeight = gate.pmosRegionHeight;
    const FeatureExp<2> nmosDrainArea = nmosDrainWidth * nmosDrainHeight;
    const FeatureExp<2> pmosDrainArea = pmosDrainWidth * pmosDrainHeight;

    const auto featureSize2 = featureSize * featureSize;

    return computeDrainCapacitance(*_tech, nmosDrainArea * featureSize2, nmosDrainWidth * featureSize, nmosDrainHeight * featureSize, gate.nmosFoldDegree)
         + computeDrainCapacitance(*_tech, pmosDrainArea * featureSize2, pmosDrainWidth * featureSize, pmosDrainHeight * featureSize, gate.pmosFoldDegree);
}

template <mse::GateType Gate, int Inputs>
mse::unit::Farad mse::CmosGate<Gate, Inputs>::gateCapacitance() const noexcept {
    return computeGateCapacitance(*_tech, _nmosWidth) + computeGateCapacitance(*_tech, _pmosWidth);
}

template <mse::GateType Gate, int Inputs>
mse::unit::Ohm mse::CmosGate<Gate, Inputs>::nmosResistance() const noexcept {
    using namespace mse::unit;

    Ohm resistance = computeOnResistance(*_tech, _tech->currentOnNmos, _nmosWidth, _tempIndex);
    resistance *= sequentialNmos ? _inputs : 1;

    return resistance;
}

template <mse::GateType Gate, int Inputs>
mse::unit::Ohm mse::CmosGate<Gate, Inputs>::pmosResistance() const noexcept {
    using namespace mse::unit;

    Ohm resistance = computeOnResistance(*_tech, _tech->currentOnPmos, _pmosWidth, _tempIndex);
    resistance *= sequentialNmos ? 1 : _inputs;

    return resistance;
}

template <mse::GateType Gate, int Inputs>
mse::unit::Ampere mse::CmosGate<Gate, Inputs>::leakageCurrent() const noexcept {
    using namespace mse::unit;

    const Technology& tech = *_tech;
    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{tech.featureSize};

    if constexpr (Gate == GateType::Inverter) {
        return Ampere(std::max((_unitNmosWidth * featureSize).value() * tech.currentOffNmos[_tempIndex], (_unitPmosWidth * featureSize).value() * tech.currentOffPmos[_tempIndex]));
    } else if constexpr (Gate == GateType::Nand) {
        Ampere leakageCurrent((_unitPmosWidth * featureSize).value() * tech.currentOffPmos[_tempIndex] * _inputs);
        leakageCurrent *= _inputs == 2 ? constant::averageLeakRatioNand2 : constant::averageLeakRatioNand3;

        return leakageCurrent;
    } else {
        Ampere leakageCurrent((_unitNmosWidth * featureSize).value() * tech.currentOffNmos[_tempIndex] * _inputs);
        leakageCurrent *= _inputs == 2 ? constant::averageLeakRatioNor2 : constant::averageLeakRatioNor3;

        return leakageCurrent;
    }
}

template class mse::CmosGate<mse::GateType::Inverter, 1>;
template class mse::CmosGate<mse::GateType::Nand, 0>;
template class mse::CmosGate<mse::GateType::Nand, 2>;
template class mse::CmosGate<mse::GateType::Nand, 3>;
template class mse::CmosGate<mse::GateType::Nor, 0>;
template class mse::CmosGate<mse::GateType::Nor, 2>;
template class mse::CmosGate<mse::GateType::Nor, 3>;
