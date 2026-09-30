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

// Calculates folding for a transistor region
// 1 = no folding, 1 finger
static unsigned calculateFolding(mse::unit::Feature regionHeight, mse::unit::Feature maxRegionHeight) {
    using namespace mse::unit;

    if (regionHeight >= maxRegionHeight) {
        // Fold
        if (maxRegionHeight < 3.0_feat) {
            mse::log::fatal("Unable to do CMOS folding because CMOS size limitation is less than 3F");
        }
        return static_cast<int>(std::ceil(regionHeight / (maxRegionHeight - 3.0_feat))); // 3F for folding overhead
    }

    return 1;
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
    Meter featureSize(tech.featureSize);
    return Farad((tech.capIdealGate + tech.capOverlap + 3.0 * tech.capFringe) * gateWidth.toMeters(featureSize).value() + tech.phyGateLength * tech.capPolywire);
}

static mse::unit::Ohm computeOnResistance(const Technology& tech, const double* onCurrent, mse::unit::Feature gateWidth, int tempIndex) {
    using namespace mse::unit;
    Meter featureSize(tech.featureSize);
    return Ohm(tech.effectiveResistanceMultiplier * tech.vdd / (onCurrent[tempIndex] * gateWidth.toMeters(featureSize).value()));
}

mse::CmosTransistor::CmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight, const double* onCurrent, const double* offCurrent, unit::Volt vdsat, unit::Number mobility) {
    using namespace mse::unit;

    const Meter featureSize(tech.featureSize);
    const int fingersAfterFolding = calculateFolding(gateWidth, transistorRegionHeight);

    const Feature foldedFingerTotalWidth = fingersAfterFolding * polyWidth + (fingersAfterFolding - 1) * diffusionRegionWidth;
    _dimensions = {
        .width = (2.0 * contactWidth + foldedFingerTotalWidth).toMeters(featureSize),
        .height = std::min(gateWidth, transistorRegionHeight).toMeters(featureSize)
    };

    const Feature foldedFingerDrainWidth = (fingersAfterFolding - 1) * diffusionRegionWidth;
    const Meter drainWidth = (contactWidth + foldedFingerDrainWidth).toMeters(featureSize);
    const Meter drainHeight = _dimensions.height;
    const MeterExp<2> drainArea = drainWidth * drainHeight;

    _drainCapacitance = computeDrainCapacitance(tech, drainArea, drainWidth, drainHeight, fingersAfterFolding);
    _gateCapacitance = computeGateCapacitance(tech, gateWidth);

    int tempIndex = gInputParameter.temperature - 300;
    if (tempIndex > 100 || tempIndex < 0) {
        log::fatal("Temperature is out of range");
    }

    _channelResistance = computeOnResistance(tech, onCurrent, gateWidth, tempIndex);

    Meter width = gateWidth.toMeters(featureSize);
	if (featureSize >= 22_m * 1e-9) {
        Volt vsat = std::min(vdsat, Volt(tech.vdd - tech.vth));
        _transconductance = Siemens((mobility * tech.capOx) / 2 * width.value() / tech.phyGateLength * vsat.value());
	} else if ((featureSize <= 14_m * 1e-9) && (featureSize >= 3_m * 1e-9)) {
	    _transconductance = Siemens(tech.gm_oncurrent * static_cast<int>(std::ceil(gateWidth.value() / 2.0)) * (2.0 * tech.heightFin + tech.widthFin) / tech.PitchFin);
	} else {
	    _transconductance = Siemens(tech.gm_oncurrent * static_cast<int>(std::ceil(gateWidth.value() / 2.0)) * tech.effective_width * tech.max_sheet_num / tech.max_fin_per_GAA);
    }

    _leakageCurrent = Ampere(gateWidth.toMeters(featureSize).value() * offCurrent[tempIndex]);
}

mse::NmosTransistor::NmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight)
        : CmosTransistor(tech, gateWidth, transistorRegionHeight, tech.currentOnNmos, tech.currentOffNmos, unit::Volt(tech.vdsatNmos), unit::Number(tech.effectiveElectronMobility)) {}

mse::PmosTransistor::PmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight)
        : CmosTransistor(tech, gateWidth, transistorRegionHeight, tech.currentOnPmos, tech.currentOffPmos, unit::Volt(tech.vdsatPmos), unit::Number(tech.effectiveHoleMobility)) {}

template <mse::GateType Gate, int Inputs>
mse::CmosGate<Gate, Inputs>::CmosGate(const Technology& tech, unit::Feature unitNmosGateWidth, unit::Feature unitPmosGateWidth, unit::Feature gateRegionHeight) requires (Inputs > 0)
        : CmosGate(tech, Inputs, unitNmosGateWidth, unitPmosGateWidth, gateRegionHeight) {}

template <mse::GateType Gate, int Inputs>
mse::CmosGate<Gate, Inputs>::CmosGate(const Technology& tech, int inputs, unit::Feature unitNmosGateWidth, unit::Feature unitPmosGateWidth, unit::Feature gateRegionHeight)
        : _inputs(inputs) {
    using namespace mse::unit;

    const Meter featureSize(tech.featureSize);
    constexpr bool sequentialNmos = Gate != GateType::Nor;

    _nmosWidth = unitNmosGateWidth;
    _pmosWidth = unitPmosGateWidth;
    _nmosWidth *= sequentialNmos ? inputs : 1;
    _pmosWidth *= sequentialNmos ? 1 : inputs;

    _logicalEffort = (_nmosWidth + _pmosWidth) / (unitNmosGateWidth + unitPmosGateWidth);

    const Feature remainingHeight = gateRegionHeight - constant::minimumGapBetweenPandNDiffusions;
    const Feature nmosGateMaxWidth = _nmosWidth / (_nmosWidth + _pmosWidth) * remainingHeight;
    const Feature pmosGateMaxWidth = _pmosWidth / (_nmosWidth + _pmosWidth) * remainingHeight;

    const int nmosFoldDegree = calculateFolding(_nmosWidth, nmosGateMaxWidth);
    const int pmosFoldDegree = calculateFolding(_pmosWidth, pmosGateMaxWidth);

    const Feature nmosRegionHeight = std::min(_nmosWidth, nmosGateMaxWidth);
    const Feature pmosRegionHeight = std::min(_pmosWidth, pmosGateMaxWidth);
    const Feature nmosFoldedFingerRegionWidth = nmosFoldDegree * polyWidth + (nmosFoldDegree - 1) * diffusionRegionWidth;
    const Feature pmosFoldedFingerRegionWidth = pmosFoldDegree * polyWidth + (pmosFoldDegree - 1) * diffusionRegionWidth;

    const Feature nmosGap = sequentialNmos ? diffusionRegionWidth : contactWidth;
    const Feature pmosGap = sequentialNmos ? contactWidth : diffusionRegionWidth;
    const Feature nmosRegionWidth = 2.0 * contactWidth + inputs * nmosFoldedFingerRegionWidth + (inputs - 1) * nmosGap;
    const Feature pmosRegionWidth = 2.0 * contactWidth + inputs * pmosFoldedFingerRegionWidth + (inputs - 1) * pmosGap;

    _dimensions = {
        .width = std::max(nmosRegionWidth, pmosRegionWidth).toMeters(featureSize),
        .height = (nmosRegionHeight + pmosRegionHeight + (constant::minimumGapBetweenPandNDiffusions + constant::minimumPowerRailWidth * 2.0)).toMeters(featureSize)
    };

    const Feature nmosFoldedFingerDrainWidth = (nmosFoldDegree - 1) * diffusionRegionWidth;
    const Feature pmosFoldedFingerDrainWidth = (pmosFoldDegree - 1) * diffusionRegionWidth;

    const Feature nmosDrainWidth = contactWidth + inputs * nmosFoldedFingerDrainWidth + (inputs - 1) * nmosGap;
    const Feature pmosDrainWidth = contactWidth + inputs * pmosFoldedFingerDrainWidth + (inputs - 1) * pmosGap;
    const Feature nmosDrainHeight = nmosRegionHeight;
    const Feature pmosDrainHeight = pmosRegionHeight;
    const FeatureExp<2> nmosDrainArea = nmosDrainWidth * nmosDrainHeight;
    const FeatureExp<2> pmosDrainArea = pmosDrainWidth * pmosDrainHeight;

    _drainCapacitance = computeDrainCapacitance(tech, nmosDrainArea.toMeters(featureSize), nmosDrainWidth.toMeters(featureSize), nmosDrainHeight.toMeters(featureSize), nmosFoldDegree)
                      + computeDrainCapacitance(tech, pmosDrainArea.toMeters(featureSize), pmosDrainWidth.toMeters(featureSize), pmosDrainHeight.toMeters(featureSize), pmosFoldDegree);

    _gateCapacitance = computeGateCapacitance(tech, _nmosWidth) + computeGateCapacitance(tech, _pmosWidth);

    int tempIndex = gInputParameter.temperature - 300;
    if (tempIndex > 100 || tempIndex < 0) {
        log::fatal("Temperature is out of range");
    }

    _nmosResistance = computeOnResistance(tech, tech.currentOnNmos, _nmosWidth, tempIndex);
    _pmosResistance = computeOnResistance(tech, tech.currentOnPmos, _pmosWidth, tempIndex);

    _nmosResistance *= sequentialNmos ? inputs : 1;
    _pmosResistance *= sequentialNmos ? 1 : inputs;

    if constexpr (Gate == GateType::Inverter) {
        _leakageCurrent = Ampere(std::max(unitNmosGateWidth.toMeters(featureSize).value() * tech.currentOffNmos[tempIndex], unitPmosGateWidth.toMeters(featureSize).value() * tech.currentOffPmos[tempIndex]));
    } else if constexpr (Gate == GateType::Nand) {
        assert(inputs == 2 || inputs == 3);
        _leakageCurrent = Ampere(unitPmosGateWidth.toMeters(featureSize).value() * tech.currentOffPmos[tempIndex] * inputs);
        _leakageCurrent *= inputs == 2 ? constant::averageLeakRatioNand2 : constant::averageLeakRatioNand3;
    } else {
        assert(inputs == 2 || inputs == 3);
        _leakageCurrent = Ampere(unitNmosGateWidth.toMeters(featureSize).value() * tech.currentOffNmos[tempIndex] * inputs);
        _leakageCurrent *= inputs == 2 ? constant::averageLeakRatioNor2 : constant::averageLeakRatioNor3;
    }
}

template class mse::CmosGate<mse::GateType::Inverter, 1>;
template class mse::CmosGate<mse::GateType::Nand, 0>;
template class mse::CmosGate<mse::GateType::Nand, 2>;
template class mse::CmosGate<mse::GateType::Nand, 3>;
template class mse::CmosGate<mse::GateType::Nor, 0>;
template class mse::CmosGate<mse::GateType::Nor, 2>;
template class mse::CmosGate<mse::GateType::Nor, 3>;
