#include "cmosDevice.hpp"
#include "constant.hpp"
#include "log.hpp"
#include "utility.hpp"
#include "Technology.hpp"
#include "global.hpp"
#include "formula.hpp"

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
    Farad junction = drainArea * tech.capJunction;
    Farad channel = foldDegree * drainHeight * tech.capDrainToChannel;
    Farad sidewall = drainSidewallLength * tech.capSidewall;

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

// FinFET and GAA layout from fin tracks and contacted poly pitches, after NeuroSim 1.4
static mse::Dimensions finfetDimensions(const Technology& tech, GateType type, int inputs, mse::unit::Feature nmosWidth, mse::unit::Feature pmosWidth, mse::unit::Feature regionHeight) {
    double height = 0;
    double width = 0;
    CalculateFinFETGateArea(type, inputs, (nmosWidth * tech.featureSize).value(), (pmosWidth * tech.featureSize).value(),
            (regionHeight * tech.featureSize).value(), tech, &height, &width);

    return mse::Dimensions{
        .width = mse::unit::Meter(width),
        .height = mse::unit::Meter(height)
    };
}

//==================================================

mse::CmosTransistor::CmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight, const Technology::TemperatureTable& onCurrent, const Technology::TemperatureTable& offCurrent, unit::Volt vdsat, Technology::Mobility mobility)
        : _tech(&tech),
          _gateWidth(gateWidth),
          _regionHeight(transistorRegionHeight),
          _onCurrent(&onCurrent),
          _offCurrent(&offCurrent),
          _vdsat(vdsat),
          _mobility(mobility),
          _tempIndex(temperatureIndex()) {
    checkFolding(_gateWidth, _regionHeight);
}

mse::Dimensions mse::CmosTransistor::dimensions() const noexcept {
    using namespace mse::unit;

    if (!IsPlanar(*_tech)) {
        return finfetDimensions(*_tech, ::GateType::INV, 1, _gateWidth, Feature(), _regionHeight);
    }

    const int fingersAfterFolding = calculateFolding(_gateWidth, _regionHeight);
    const Feature foldedFingerTotalWidth = fingersAfterFolding * polyWidth + (fingersAfterFolding - 1) * diffusionRegionWidth;

    return Dimensions{
        .width = (2.0 * contactWidth + foldedFingerTotalWidth) * _tech->featureSize,
        .height = std::min(_gateWidth, _regionHeight) * _tech->featureSize
    };
}

mse::unit::Farad mse::CmosTransistor::drainCapacitance() const noexcept {
    using namespace mse::unit;

    if (!IsPlanar(*_tech)) {
        return effectiveWidth(*_tech, _gateWidth) * _tech->capDrainTotal;
    }

    const int fingersAfterFolding = calculateFolding(_gateWidth, _regionHeight);

    const Feature foldedFingerDrainWidth = (fingersAfterFolding - 1) * diffusionRegionWidth;
    const Meter drainWidth = (contactWidth + foldedFingerDrainWidth) * _tech->featureSize;
    const Meter drainHeight = std::min(_gateWidth, _regionHeight) * _tech->featureSize;
    const MeterExp<2> drainArea = drainWidth * drainHeight;

    return computeDrainCapacitance(*_tech, drainArea, drainWidth, drainHeight, fingersAfterFolding);
}

mse::unit::Farad mse::CmosTransistor::gateCapacitance() const noexcept {
    return computeGateCapacitance(*_tech, _gateWidth);
}

mse::unit::Ampere mse::CmosTransistor::leakageCurrent() const noexcept {
    using namespace mse::unit;

    return effectiveWidth(*_tech, _gateWidth) * (*_offCurrent)[_tempIndex];
}

mse::unit::Ohm mse::CmosTransistor::channelResistance() const noexcept {
    return computeOnResistance(*_tech, *_onCurrent, _gateWidth, _tempIndex);
}

mse::unit::Siemens mse::CmosTransistor::transconductance() const noexcept {
    using namespace mse::unit;

    const Technology& tech = *_tech;

    if (tech.featureSizeInNano >= 22) {
        const Meter width = _gateWidth * tech.featureSize;
        const Volt vsat = std::min(_vdsat, tech.vdd - tech.vth);

        return (_mobility * tech.capOx) / 2 * width / tech.phyGateLength * vsat;
    }

    return tech.onCurrentTransconductance * effectiveWidth(tech, _gateWidth);
}

mse::NmosTransistor::NmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight)
        : CmosTransistor(tech, gateWidth, transistorRegionHeight, tech.currentOnNmos, tech.currentOffNmos, tech.vdsatNmos, tech.effectiveElectronMobility) {}

mse::PmosTransistor::PmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight)
        : CmosTransistor(tech, gateWidth, transistorRegionHeight, tech.currentOnPmos, tech.currentOffPmos, tech.vdsatPmos, tech.effectiveHoleMobility) {}

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
          _regionHeight(gateRegionHeight),
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

// The layout gate type for a gate type
static constexpr ::GateType layoutGateType(mse::GateType gate) {
    switch (gate) {
        case mse::GateType::Nand: return ::GateType::NAND;
        case mse::GateType::Nor: return ::GateType::NOR;
        default: return ::GateType::INV;
    }
}

template <mse::GateType Gate, int Inputs>
mse::Dimensions mse::CmosGate<Gate, Inputs>::dimensions() const noexcept {
    using namespace mse::unit;

    if (!IsPlanar(*_tech)) {
        return finfetDimensions(*_tech, layoutGateType(Gate), _inputs, _nmosWidth, _pmosWidth, _regionHeight);
    }

    const Layout gate = layout();

    const Feature nmosFoldedFingerRegionWidth = gate.nmosFoldDegree * polyWidth + (gate.nmosFoldDegree - 1) * diffusionRegionWidth;
    const Feature pmosFoldedFingerRegionWidth = gate.pmosFoldDegree * polyWidth + (gate.pmosFoldDegree - 1) * diffusionRegionWidth;

    const Feature nmosGap = sequentialNmos ? diffusionRegionWidth : contactWidth;
    const Feature pmosGap = sequentialNmos ? contactWidth : diffusionRegionWidth;
    const Feature nmosRegionWidth = 2.0 * contactWidth + _inputs * nmosFoldedFingerRegionWidth + (_inputs - 1) * nmosGap;
    const Feature pmosRegionWidth = 2.0 * contactWidth + _inputs * pmosFoldedFingerRegionWidth + (_inputs - 1) * pmosGap;

    return Dimensions{
        .width = std::max(nmosRegionWidth, pmosRegionWidth) * _tech->featureSize,
        .height = (gate.nmosRegionHeight + gate.pmosRegionHeight + (constant::minimumGapBetweenPandNDiffusions + constant::minimumPowerRailWidth * 2.0)) * _tech->featureSize
    };
}

template <mse::GateType Gate, int Inputs>
mse::unit::Farad mse::CmosGate<Gate, Inputs>::drainCapacitance() const noexcept {
    using namespace mse::unit;

    if (!IsPlanar(*_tech)) {
        const Farad nmosDrain = effectiveWidth(*_tech, _nmosWidth) * _tech->capDrainTotal;
        const Farad pmosDrain = effectiveWidth(*_tech, _pmosWidth) * _tech->capDrainTotal;
        if constexpr (Gate == GateType::Inverter) {
            return nmosDrain + pmosDrain;
        } else if constexpr (Gate == GateType::Nor) {
            return nmosDrain * _inputs + pmosDrain;
        } else {
            return nmosDrain + pmosDrain * _inputs;
        }
    }

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

    const auto featureSize2 = _tech->featureSize * _tech->featureSize;

    return computeDrainCapacitance(*_tech, nmosDrainArea * featureSize2, nmosDrainWidth * _tech->featureSize, nmosDrainHeight * _tech->featureSize, gate.nmosFoldDegree)
         + computeDrainCapacitance(*_tech, pmosDrainArea * featureSize2, pmosDrainWidth * _tech->featureSize, pmosDrainHeight * _tech->featureSize, gate.pmosFoldDegree);
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

    if constexpr (Gate == GateType::Inverter) {
        return std::max(effectiveWidth(tech, _unitNmosWidth) * tech.currentOffNmos[_tempIndex], effectiveWidth(tech, _unitPmosWidth) * tech.currentOffPmos[_tempIndex]);
    } else if constexpr (Gate == GateType::Nand) {
        Ampere leakageCurrent = effectiveWidth(tech, _unitPmosWidth) * tech.currentOffPmos[_tempIndex] * _inputs;
        leakageCurrent *= _inputs == 2 ? constant::averageLeakRatioNand2 : constant::averageLeakRatioNand3;

        return leakageCurrent;
    } else {
        Ampere leakageCurrent = effectiveWidth(tech, _unitNmosWidth) * tech.currentOffNmos[_tempIndex] * _inputs;
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
