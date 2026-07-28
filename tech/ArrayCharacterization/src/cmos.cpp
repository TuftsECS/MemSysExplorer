#include "cmos.hpp"
#include "constants.hpp"
#include "log.hpp"
#include "utility.hpp"
#include "Technology.hpp"
#include "global.hpp"

#include <cassert>

// Common constants in this file
static constexpr mse::unit::Feature contactWidth = mse::constant::contactSize + mse::constant::minimumGapBetweenContactAndPoly * 2.0;
static constexpr mse::unit::Feature diffusionRegionWidth = mse::constant::minimumGapBetweenPoly;
static constexpr mse::unit::Feature polyWidth(1.0);

// Calculates the amount of folding done for transistor region
// 1 = no folding (1 finger)
static int calcFoldDegree(mse::unit::Feature regionHeight, mse::unit::Feature maxRegionHeight) {
    using namespace mse::unit;
    if (regionHeight > maxRegionHeight) {
        // Need to fold
        if (maxRegionHeight < 3.0_feat) {
            mse::outputLog.fatal("Unable to do CMOS folding because CMOS size limitation is less than 3F\n");
            mse::exit(mse::ExitCode::Failure);
        }
        return static_cast<int>(std::ceil(regionHeight / (maxRegionHeight - 3.0_feat))); // 3F for folding overhead
    }
    return 1;
}

mse::CmosTransistor::CmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight, const double* onCurrent, const double* offCurrent, unit::Volt vdsat, unit::Number mobility) {
    using namespace mse::unit;

    const Meter featureSize(tech.featureSize);
    const int fingersAfterFolding = calcFoldDegree(gateWidth, transistorRegionHeight);

    const Feature foldedFingerTotalWidth = fingersAfterFolding * polyWidth + (fingersAfterFolding - 1) * diffusionRegionWidth;
    _width = (2.0 * contactWidth + foldedFingerTotalWidth).toMeters(featureSize);
    _height = std::min(gateWidth, transistorRegionHeight).toMeters(featureSize);

    const Feature foldedFingerDrainWidth = (fingersAfterFolding - 1) * diffusionRegionWidth;
    const Meter drainWidth = (contactWidth + foldedFingerDrainWidth).toMeters(featureSize);
    const Meter drainHeight = _height;
    const MeterExp<2> drainArea = drainWidth * drainHeight;

    const Farad drainJunctionCapacitance = Farad(drainArea.value() * tech.capJunction);
    const Farad drainChannelCapacitance = Farad(fingersAfterFolding * drainHeight.value() * tech.capDrainToChannel);
    Meter drainSidewallLength = 2.0 * drainWidth;
    if (fingersAfterFolding % 2 == 1) {
        drainSidewallLength += drainHeight;
    }
    const Farad drainSidewallCapacitance = Farad(drainSidewallLength.value() * tech.capSidewall);

    _drainCapacitance = drainJunctionCapacitance + drainChannelCapacitance + drainSidewallCapacitance;
    _gateCapacitance = Farad((tech.capIdealGate + tech.capOverlap + 3.0 * tech.capFringe) * gateWidth.toMeters(featureSize).value() + tech.phyGateLength * tech.capPolywire);

    int tempIndex = gInputParameter.temperature - 300;
    if (tempIndex > 100 || tempIndex < 0) {
        outputLog.fatal("Temperature is out of range\n");
        exit(ExitCode::Failure);
    }

    _channelResistance = Ohm(tech.effectiveResistanceMultiplier * tech.vdd / (onCurrent[tempIndex] * gateWidth.toMeters(featureSize).value()));

    Meter width = gateWidth.toMeters(featureSize);
	if (featureSize >= 22_m * 1e-9) {
        Volt vsat = std::min(vdsat, Volt(tech.vdd - tech.vth));
        _transconductance = Siemens((mobility * tech.capOx) / 2 * width.value() / tech.phyGateLength * vsat.value());
	} else if ((featureSize <= 14_m * 1e-9) && (featureSize >= 3_m * 1e-9)) {
		width /= 2.0;
	    _transconductance = Siemens(tech.gm_oncurrent * static_cast<int>(std::ceil(width.value())) * (2.0 * tech.heightFin + tech.widthFin) / tech.PitchFin);
	} else {
		width /= 2.0;
	    _transconductance = Siemens(tech.gm_oncurrent * static_cast<int>(std::ceil(width.value())) * tech.effective_width * tech.max_sheet_num / tech.max_fin_per_GAA);
    }

    _leakageCurrent = Ampere(gateWidth.toMeters(featureSize).value() * offCurrent[tempIndex]);
}

mse::NmosTransistor::NmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight)
        : CmosTransistor(tech, gateWidth, transistorRegionHeight, tech.currentOnNmos, tech.currentOffNmos, unit::Volt(tech.vdsatNmos), unit::Number(tech.effectiveElectronMobility)) {}

mse::PmosTransistor::PmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight)
        : CmosTransistor(tech, gateWidth, transistorRegionHeight, tech.currentOnPmos, tech.currentOffPmos, unit::Volt(tech.vdsatPmos), unit::Number(tech.effectiveHoleMobility)) {}

template <typename Derived, int Inputs>
mse::CmosGate<Derived, Inputs>::CmosGate(const Technology& tech, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight) {
    using namespace mse::unit;

    const Meter featureSize(tech.featureSize);

    const Feature remainingHeight = gateRegionHeight - constant::minimumGapBetweenPandNDiffusions;
    const Feature nmosGateMaxWidth = nmosGateWidth / (nmosGateWidth + pmosGateWidth) * remainingHeight;
    const Feature pmosGateMaxWidth = pmosGateWidth / (nmosGateWidth + pmosGateWidth) * remainingHeight;

    const int nmosFoldDegree = calcFoldDegree(nmosGateWidth, nmosGateMaxWidth);
    const int pmosFoldDegree = calcFoldDegree(pmosGateWidth, pmosGateMaxWidth);

    const Feature nmosRegionHeight = std::min(nmosGateWidth, nmosGateMaxWidth);
    const Feature pmosRegionHeight = std::min(pmosGateWidth, pmosGateMaxWidth);
    const Feature nmosFoldedFingerRegionWidth = nmosFoldDegree * polyWidth + (nmosFoldDegree - 1) * diffusionRegionWidth;
    const Feature pmosFoldedFingerRegionWidth = pmosFoldDegree * polyWidth + (pmosFoldDegree - 1) * diffusionRegionWidth;

    Feature nmosRegionWidth, pmosRegionWidth;
    if constexpr (Derived::sequentialNmos) {
        // Sequential nmos and parallel pmos (NAND gate)
        nmosRegionWidth = 2.0 * contactWidth + Inputs * nmosFoldedFingerRegionWidth + (Inputs - 1) * diffusionRegionWidth;
        pmosRegionWidth = 2.0 * contactWidth + Inputs * pmosFoldedFingerRegionWidth + (Inputs - 1) * contactWidth;
    } else {
        // Parallel nmos and sequential pmos (NOR gate)
        nmosRegionWidth = 2.0 * contactWidth + Inputs * nmosFoldedFingerRegionWidth + (Inputs - 1) * contactWidth;
        pmosRegionWidth = 2.0 * contactWidth + Inputs * pmosFoldedFingerRegionWidth + (Inputs - 1) * diffusionRegionWidth;
    }

    _width = std::max(nmosRegionWidth, pmosRegionWidth).toMeters(featureSize);
    _height = (nmosRegionHeight + pmosRegionHeight + (constant::minimumGapBetweenPandNDiffusions + constant::minimumPowerRailWidth * 2.0)).toMeters(featureSize);

    const Feature nmosFoldedFingerDrainWidth = (nmosFoldDegree - 1) * diffusionRegionWidth;
    const Feature pmosFoldedFingerDrainWidth = (pmosFoldDegree - 1) * diffusionRegionWidth;

    Feature nmosDrainWidth, pmosDrainWidth;
    if constexpr (Derived::sequentialNmos) {
        nmosDrainWidth = contactWidth + Inputs * nmosFoldedFingerDrainWidth + (Inputs - 1) * diffusionRegionWidth;
        pmosDrainWidth = contactWidth + Inputs * pmosFoldedFingerDrainWidth + (Inputs - 1) * contactWidth;
    } else {
        nmosDrainWidth = contactWidth + Inputs * nmosFoldedFingerDrainWidth + (Inputs - 1) * contactWidth;
        pmosDrainWidth = contactWidth + Inputs * pmosFoldedFingerDrainWidth + (Inputs - 1) * diffusionRegionWidth;
    }
    const Feature nmosDrainHeight = nmosRegionHeight;
    const Feature pmosDrainHeight = pmosRegionHeight;
    const FeatureExp<2> nmosDrainArea = nmosDrainWidth * nmosDrainHeight;
    const FeatureExp<2> pmosDrainArea = pmosDrainWidth * pmosDrainHeight;

    Farad nmosJunctionCapacitance = Farad(nmosDrainArea.toMeters(featureSize).value() * tech.capJunction);
    Farad pmosJunctionCapacitance = Farad(pmosDrainArea.toMeters(featureSize).value() * tech.capJunction);
    Feature nmosSidewallLength = 2.0 * nmosDrainWidth;
    Feature pmosSidewallLength = 2.0 * pmosDrainWidth;
    if (nmosFoldDegree % 2 == 1) {
        nmosSidewallLength += nmosDrainHeight;
    }
    if (pmosFoldDegree % 2 == 1) {
        pmosSidewallLength += pmosDrainHeight;
    }
    Farad nmosSidewallCapacitance = Farad(nmosSidewallLength.toMeters(featureSize).value() * tech.capSidewall);
    Farad pmosSidewallCapacitance = Farad(pmosSidewallLength.toMeters(featureSize).value() * tech.capSidewall);

    Farad nmosChannelCapacitance = Farad(nmosFoldDegree * nmosRegionHeight.toMeters(featureSize).value() * tech.capDrainToChannel);
    Farad pmosChannelCapacitance = Farad(pmosFoldDegree * pmosRegionHeight.toMeters(featureSize).value() * tech.capDrainToChannel);

    Farad nmosDrainCapacitance = Farad(nmosJunctionCapacitance + nmosSidewallCapacitance + nmosChannelCapacitance);
    Farad pmosDrainCapacitance = Farad(pmosJunctionCapacitance + pmosSidewallCapacitance + pmosChannelCapacitance);
    _drainCapacitance = nmosDrainCapacitance + pmosDrainCapacitance;

    _gateCapacitance = Farad((tech.capIdealGate + tech.capOverlap + 3.0 * tech.capFringe) * nmosGateWidth.toMeters(featureSize).value() + tech.phyGateLength * tech.capPolywire);
    _gateCapacitance += Farad((tech.capIdealGate + tech.capOverlap + 3.0 * tech.capFringe) * pmosGateWidth.toMeters(featureSize).value() + tech.phyGateLength * tech.capPolywire);

    int tempIndex = gInputParameter.temperature - 300;
    if (tempIndex > 100 || tempIndex < 0) {
        outputLog.fatal("Temperature is out of range\n");
        exit(ExitCode::Failure);
    }

    // TODO account for number of inputs, this is the same code as a transistor right now
    _nmosResistance = Ohm(tech.effectiveResistanceMultiplier * tech.vdd / (tech.currentOnNmos[tempIndex] * nmosGateWidth.toMeters(featureSize).value()));
    _pmosResistance = Ohm(tech.effectiveResistanceMultiplier * tech.vdd / (tech.currentOnPmos[tempIndex] * pmosGateWidth.toMeters(featureSize).value()));
}

template <typename Derived>
mse::CmosGate<Derived, 0>::CmosGate(const Technology& tech, int inputs, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight)
        : _inputs(inputs) {
    using namespace mse::unit;

    const Meter featureSize(tech.featureSize);

    const Feature remainingHeight = gateRegionHeight - constant::minimumGapBetweenPandNDiffusions;
    const Feature nmosGateMaxWidth = nmosGateWidth / (nmosGateWidth + pmosGateWidth) * remainingHeight;
    const Feature pmosGateMaxWidth = pmosGateWidth / (nmosGateWidth + pmosGateWidth) * remainingHeight;

    const int nmosFoldDegree = calcFoldDegree(nmosGateWidth, nmosGateMaxWidth);
    const int pmosFoldDegree = calcFoldDegree(pmosGateWidth, pmosGateMaxWidth);

    const Feature nmosRegionHeight = std::min(nmosGateWidth, nmosGateMaxWidth);
    const Feature pmosRegionHeight = std::min(pmosGateWidth, pmosGateMaxWidth);
    const Feature nmosFoldedFingerRegionWidth = nmosFoldDegree * polyWidth + (nmosFoldDegree - 1) * diffusionRegionWidth;
    const Feature pmosFoldedFingerRegionWidth = pmosFoldDegree * polyWidth + (pmosFoldDegree - 1) * diffusionRegionWidth;

    Feature nmosRegionWidth, pmosRegionWidth;
    if constexpr (Derived::sequentialNmos) {
        // Sequential nmos and parallel pmos (NAND gate)
        nmosRegionWidth = 2.0 * contactWidth + inputs * nmosFoldedFingerRegionWidth + (inputs - 1) * diffusionRegionWidth;
        pmosRegionWidth = 2.0 * contactWidth + inputs * pmosFoldedFingerRegionWidth + (inputs - 1) * contactWidth;
    } else {
        // Parallel nmos and sequential pmos (NOR gate)
        nmosRegionWidth = 2.0 * contactWidth + inputs * nmosFoldedFingerRegionWidth + (inputs - 1) * contactWidth;
        pmosRegionWidth = 2.0 * contactWidth + inputs * pmosFoldedFingerRegionWidth + (inputs - 1) * diffusionRegionWidth;
    }

    _width = std::max(nmosRegionWidth, pmosRegionWidth).toMeters(featureSize);
    _height = (nmosRegionHeight + pmosRegionHeight + (constant::minimumGapBetweenPandNDiffusions + constant::minimumPowerRailWidth * 2.0)).toMeters(featureSize);

    const Feature nmosFoldedFingerDrainWidth = (nmosFoldDegree - 1) * diffusionRegionWidth;
    const Feature pmosFoldedFingerDrainWidth = (pmosFoldDegree - 1) * diffusionRegionWidth;

    Feature nmosDrainWidth, pmosDrainWidth;
    if constexpr (Derived::sequentialNmos) {
        nmosDrainWidth = contactWidth + inputs * nmosFoldedFingerDrainWidth + (inputs - 1) * diffusionRegionWidth;
        pmosDrainWidth = contactWidth + inputs * pmosFoldedFingerDrainWidth + (inputs - 1) * contactWidth;
    } else {
        nmosDrainWidth = contactWidth + inputs * nmosFoldedFingerDrainWidth + (inputs - 1) * contactWidth;
        pmosDrainWidth = contactWidth + inputs * pmosFoldedFingerDrainWidth + (inputs - 1) * diffusionRegionWidth;
    }
    const Feature nmosDrainHeight = nmosRegionHeight;
    const Feature pmosDrainHeight = pmosRegionHeight;
    const FeatureExp<2> nmosDrainArea = nmosDrainWidth * nmosDrainHeight;
    const FeatureExp<2> pmosDrainArea = pmosDrainWidth * pmosDrainHeight;

    Farad nmosJunctionCapacitance = Farad(nmosDrainArea.toMeters(featureSize).value() * tech.capJunction);
    Farad pmosJunctionCapacitance = Farad(pmosDrainArea.toMeters(featureSize).value() * tech.capJunction);
    Feature nmosSidewallLength = 2.0 * nmosDrainWidth;
    Feature pmosSidewallLength = 2.0 * pmosDrainWidth;
    if (nmosFoldDegree % 2 == 1) {
        nmosSidewallLength += nmosDrainHeight;
    }
    if (pmosFoldDegree % 2 == 1) {
        pmosSidewallLength += pmosDrainHeight;
    }
    Farad nmosSidewallCapacitance = Farad(nmosSidewallLength.toMeters(featureSize).value() * tech.capSidewall);
    Farad pmosSidewallCapacitance = Farad(pmosSidewallLength.toMeters(featureSize).value() * tech.capSidewall);

    Farad nmosChannelCapacitance = Farad(nmosFoldDegree * nmosRegionHeight.toMeters(featureSize).value() * tech.capDrainToChannel);
    Farad pmosChannelCapacitance = Farad(pmosFoldDegree * pmosRegionHeight.toMeters(featureSize).value() * tech.capDrainToChannel);

    Farad nmosDrainCapacitance = Farad(nmosJunctionCapacitance + nmosSidewallCapacitance + nmosChannelCapacitance);
    Farad pmosDrainCapacitance = Farad(pmosJunctionCapacitance + pmosSidewallCapacitance + pmosChannelCapacitance);
    _drainCapacitance = nmosDrainCapacitance + pmosDrainCapacitance;

    _gateCapacitance = Farad((tech.capIdealGate + tech.capOverlap + 3.0 * tech.capFringe) * nmosGateWidth.toMeters(featureSize).value() + tech.phyGateLength * tech.capPolywire);
    _gateCapacitance += Farad((tech.capIdealGate + tech.capOverlap + 3.0 * tech.capFringe) * pmosGateWidth.toMeters(featureSize).value() + tech.phyGateLength * tech.capPolywire);

    int tempIndex = gInputParameter.temperature - 300;
    if (tempIndex > 100 || tempIndex < 0) {
        outputLog.fatal("Temperature is out of range\n");
        exit(ExitCode::Failure);
    }

    // TODO account for number of inputs, this is the same code as a transistor right now
    _nmosResistance = Ohm(tech.effectiveResistanceMultiplier * tech.vdd / (tech.currentOnNmos[tempIndex] * nmosGateWidth.toMeters(featureSize).value()));
    _pmosResistance = Ohm(tech.effectiveResistanceMultiplier * tech.vdd / (tech.currentOnPmos[tempIndex] * pmosGateWidth.toMeters(featureSize).value()));
}

mse::CmosInverter::CmosInverter(const Technology& tech, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight)
        : CmosGate<CmosInverter, 1>(tech, nmosGateWidth, pmosGateWidth, gateRegionHeight) {
    int tempIndex = gInputParameter.temperature - 300;
    if (tempIndex > 100 || tempIndex < 0) {
        outputLog.fatal("Temperature is out of range\n");
        exit(ExitCode::Failure);
    }
    _leakageCurrent = unit::Ampere(std::max(nmosGateWidth.toMeters(unit::Meter(tech.featureSize)).value() * tech.currentOffNmos[tempIndex], pmosGateWidth.toMeters(unit::Meter(tech.featureSize)).value() * tech.currentOffPmos[tempIndex]));
}

template <int Inputs>
mse::CmosNand<Inputs>::CmosNand(const Technology& tech, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight)
        : CmosGate<CmosNand<Inputs>, Inputs>(tech, nmosGateWidth, pmosGateWidth, gateRegionHeight) {
    int tempIndex = gInputParameter.temperature - 300;
    if (tempIndex > 100 || tempIndex < 0) {
        outputLog.fatal("Temperature is out of range\n");
        exit(ExitCode::Failure);
    }

    auto& _leakageCurrent = CmosGate<CmosNand<Inputs>, Inputs>::_leakageCurrent;
    _leakageCurrent = unit::Ampere(pmosGateWidth.toMeters(unit::Meter(tech.featureSize)).value() * tech.currentOffPmos[tempIndex] * Inputs);
    if constexpr (Inputs == 2) {
        _leakageCurrent *= constant::averageLeakRatioNand2;
    } else {
        _leakageCurrent *= constant::averageLeakRatioNand3;
    }
}

mse::CmosNand<0>::CmosNand(const Technology& tech, int inputs, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight)
        : CmosGate<CmosNand<0>, 0>(tech, inputs, nmosGateWidth, pmosGateWidth, gateRegionHeight) {
    assert(inputs == 2 || inputs == 3);
    int tempIndex = gInputParameter.temperature - 300;
    if (tempIndex > 100 || tempIndex < 0) {
        outputLog.fatal("Temperature is out of range\n");
        exit(ExitCode::Failure);
    }

    _leakageCurrent = unit::Ampere(pmosGateWidth.toMeters(unit::Meter(tech.featureSize)).value() * tech.currentOffPmos[tempIndex] * inputs);
    if (inputs == 2) {
        _leakageCurrent *= constant::averageLeakRatioNand2;
    } else {
        _leakageCurrent *= constant::averageLeakRatioNand3;
    }
}

template <int Inputs>
mse::CmosNor<Inputs>::CmosNor(const Technology& tech, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight)
        : CmosGate<CmosNor<Inputs>, Inputs>(tech, nmosGateWidth, pmosGateWidth, gateRegionHeight) {
    int tempIndex = gInputParameter.temperature - 300;
    if (tempIndex > 100 || tempIndex < 0) {
        outputLog.fatal("Temperature is out of range\n");
        exit(ExitCode::Failure);
    }

    auto& _leakageCurrent = CmosGate<CmosNand<Inputs>, Inputs>::_leakageCurrent;
    _leakageCurrent = unit::Ampere(nmosGateWidth.toMeters(unit::Meter(tech.featureSize)).value() * tech.currentOffNmos[tempIndex] * Inputs);
    if constexpr (Inputs == 2) {
        _leakageCurrent *= constant::averageLeakRatioNor2;
    } else {
        _leakageCurrent *= constant::averageLeakRatioNor3;
    }
}

mse::CmosNor<0>::CmosNor(const Technology& tech, int inputs, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight)
        : CmosGate<CmosNor<0>, 0>(tech, inputs, nmosGateWidth, pmosGateWidth, gateRegionHeight) {
    assert(inputs == 2 || inputs == 3);
    int tempIndex = gInputParameter.temperature - 300;
    if (tempIndex > 100 || tempIndex < 0) {
        outputLog.fatal("Temperature is out of range\n");
        exit(ExitCode::Failure);
    }

    _leakageCurrent = unit::Ampere(nmosGateWidth.toMeters(unit::Meter(tech.featureSize)).value() * tech.currentOffNmos[tempIndex] * inputs);
    if (inputs == 2) {
        _leakageCurrent *= constant::averageLeakRatioNor2;
    } else {
        _leakageCurrent *= constant::averageLeakRatioNor3;
    }
}

// Explicit instantiations force creation of only valid templates
template class mse::CmosGate<mse::CmosInverter, 1>;
template class mse::CmosGate<mse::CmosNand<0>, 0>;
template class mse::CmosGate<mse::CmosNand<2>, 2>;
template class mse::CmosGate<mse::CmosNand<3>, 3>;
template class mse::CmosGate<mse::CmosNor<0>, 0>;
template class mse::CmosGate<mse::CmosNor<2>, 2>;
template class mse::CmosGate<mse::CmosNor<3>, 3>;
