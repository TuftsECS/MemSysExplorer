#include "component/SenseAmp.hpp"

#include "cmosDevice.hpp"
#include "global.hpp"

#include <array>
#include <cmath>
#include <optional>

static std::optional<mse::unit::Number> mlcValue(int fingers, const std::array<mse::unit::Number, 7>& values) {
    if (fingers % 8 != 0 || fingers < 8 || fingers > 56) {
        return std::nullopt;
    }
    return values[fingers / 8 - 1];
}

static bool usesMlcModel(bool mlc) {
    return mlc && (gCell.memCellType == MLCCTT || gCell.memCellType == MLCRRAM);
}

bool mse::component::SenseAmp::setup() noexcept {
    using namespace mse::unit;

    return _senseAmpPitch > gTech.featureSize * 2_feat;
}

mse::unit::Farad mse::component::SenseAmp::calculateLoadCapacitance() const noexcept {
    using namespace mse::unit;
    using namespace mse::constant;

    const Number finFactor = gTech.featureSizeInNano <= 14 ? 2 : 1;
    const Feature pitch = _senseAmpPitch / gTech.featureSize;

    Farad capacitance = NmosTransistor(gTech, finFactor * (senseAmpAltWidthP + senseAmpAltWidthN), pitch).gateCapacitance()
            + NmosTransistor(gTech, finFactor * senseAmpAltWidthN, pitch).drainCapacitance()
            + PmosTransistor(gTech, finFactor * senseAmpAltWidthP, pitch).drainCapacitance()
            + PmosTransistor(gTech, finFactor * senseAmpAltWidthTop, pitch).drainCapacitance()
            + NmosTransistor(gTech, finFactor * senseAmpAltWidthBottom, pitch).drainCapacitance();

    if (usesMlcModel(_mlc)) {
        if (auto mlcCap = mlcValue(_fingers, {831E-18, 1.092E-15, 1.351E-15, 1.609E-15, 1.868E-15, 2.125E-15, 2.383E-15})) {
            capacitance = Farad(*mlcCap);
        }
        capacitance *= _levels - 1.0;
    }
    return capacitance;
}

mse::Dimensions mse::component::SenseAmp::calculateDimensions() const noexcept {
    using namespace mse::unit;
    using namespace mse::constant;

    const Number finFactor = gTech.featureSizeInNano <= 14 ? 2 : 1;
    const Feature pitch = _senseAmpPitch / gTech.featureSize;

    const Dimensions top = PmosTransistor(gTech, finFactor * senseAmpAltWidthTop, pitch).dimensions();
    const Dimensions p = PmosTransistor(gTech, finFactor * senseAmpAltWidthP, pitch).dimensions();
    const Dimensions n = NmosTransistor(gTech, finFactor * senseAmpAltWidthN, pitch).dimensions();
    const Dimensions bottom = NmosTransistor(gTech, finFactor * senseAmpAltWidthBottom, pitch).dimensions();

    Dimensions dimensions;
    if (usesMlcModel(_mlc)) {
        if (auto mlcWidth = mlcValue(_fingers, {2.4E-6, 3.8E-6, 5.2E-6, 6.8E-6, 8.2E-6, 9.6E-6, 11E-6})) {
            dimensions.width = Meter(*mlcWidth);
        } else {
            dimensions.width = std::max({top.height, p.height, n.height, bottom.height});
        }
        dimensions.height = 1.4E-6_m;
        dimensions.width *= _levels - 1.0;
    } else {
        dimensions.width = std::max({top.height, p.height, n.height, bottom.height});
        dimensions.height = top.width + 2 * p.width + 2 * n.width + 2 * bottom.width;
    }

    dimensions.height = dimensions.height * dimensions.width / _senseAmpPitch;
    dimensions.width = _senseAmpPitch * _columns;

    if (_currentSense) {
        const MeterExp<2> ivConverter = ivConverterArea * gTech.featureSize * gTech.featureSize;
        dimensions.height += ivConverter / dimensions.width;
    }

    return dimensions;
}

mse::component::BasicTiming mse::component::SenseAmp::calculateTiming() const noexcept {
    using namespace mse::unit;
    using namespace mse::constant;

    Second readLatency;

    if (_currentSense) {
        if (gTech.featureSizeInNano >= 179) {
            readLatency = 0.46e-9_s;
        } else if (gTech.featureSizeInNano >= 119) {
            readLatency = 0.49e-9_s;
        } else if (gTech.featureSizeInNano >= 89) {
            readLatency = 0.53e-9_s;
        } else if (gTech.featureSizeInNano >= 64) {
            readLatency = 0.62e-9_s;
        } else if (gTech.featureSizeInNano >= 44) {
            readLatency = 0.80e-9_s;
        } else if (gTech.featureSizeInNano >= 31) {
            readLatency = 1.07e-9_s;
        } else {
            readLatency = 1.0e-10_s;
        }
    } else {
        const Number finFactor = gTech.featureSizeInNano <= 14 ? 2 : 1;
        const Feature height = maxTransistorHeight;
        Siemens gm;
        if (auto mlcGm = usesMlcModel(_mlc) ? mlcValue(_fingers, {240.3E-6, 340.5E-6, 385.9E-6, 408.7E-6, 421.7E-6, 430.0E-6, 435.8E-6}) : std::nullopt) {
            gm = Siemens(*mlcGm);
        } else {
            gm = NmosTransistor(gTech, finFactor * senseAmpWidthN, height).transconductance() + PmosTransistor(gTech, finFactor * senseAmpWidthP, height).transconductance();
        }
        const Second tau = loadCapacitance() / OhmExp<-1>(gm);
        readLatency = tau * std::log(gTech.vdd / senseVoltage);
    }

    if (gCell.memCellType == FeFET) {
        readLatency = 1.2e-10_s;
    } else if (gCell.memCellType == MLCFeFET && _levels == 4) {
        readLatency = 4.5e-10_s;
    } else if (gCell.memCellType == MLCFeFET && _levels == 8) {
        readLatency = 7.0e-10_s;
    }

    BasicTiming timing;
    timing.readLatency = readLatency;
    timing.writeLatency = 0_s;

    return timing;
}

mse::component::BasicPower mse::component::SenseAmp::calculatePower() const noexcept {
    using namespace mse::unit;
    using namespace mse::constant;

    Joule readEnergy;
    Watt leakage;

    if (_currentSense) {
        if (gTech.featureSizeInNano >= 119) {
            readEnergy = 8.52e-14_J;
            leakage = 1.40e-8_W;
        } else if (gTech.featureSizeInNano >= 89) {
            readEnergy = 8.72e-14_J;
            leakage = 1.87e-8_W;
        } else if (gTech.featureSizeInNano >= 64) {
            readEnergy = 9.00e-14_J;
            leakage = 2.57e-8_W;
        } else if (gTech.featureSizeInNano >= 44) {
            readEnergy = 10.26e-14_J;
            leakage = 4.41e-9_W;
        } else if (gTech.featureSizeInNano >= 31) {
            readEnergy = 12.56e-14_J;
            leakage = 12.54e-8_W;
        } else {
            readEnergy = 8e-15_J;
            leakage = 0_W;
        }
    } else {
        readEnergy = loadCapacitance() * gTech.vdd * gTech.vdd;
        Ampere idleCurrent;
        if (_mlc) {
            idleCurrent = 1.35E-11_A * (_levels - 1.0);
        } else {
            const Number finFactor = gTech.featureSizeInNano <= 14 ? 2 : 1;
            idleCurrent = NmosTransistor(gTech, finFactor * senseAmpWidthEnable, maxTransistorHeight).leakageCurrent();
        }
        leakage = idleCurrent * gTech.vdd;
    }

    if (gCell.memCellType == FeFET) {
        readEnergy = 7.3e-16_J;
    } else if (gCell.memCellType == MLCFeFET && _levels == 4) {
        readEnergy = 1.092e-14_J;
    } else if (gCell.memCellType == MLCFeFET && _levels == 8) {
        readEnergy = 3.823e-14_J;
    }

    BasicPower power;
    power.dynamicReadEnergy = readEnergy * _columns;
    power.dynamicWriteEnergy = 0_J;
    power.leakage = leakage * _columns;

    return power;
}
