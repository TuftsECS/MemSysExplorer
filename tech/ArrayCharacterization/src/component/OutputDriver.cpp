#include "component/OutputDriver.hpp"
#include "cmosDevice.hpp"
#include "global.hpp"
#include "formula.hpp"

#include <cmath>

mse::component::OutputDriverChain mse::component::OutputDriver::calculateChain() const noexcept {
    using namespace mse::unit;

    OutputDriverChain chain;

    const Meter minWidth = constant::minNmosSize * gTech.featureSize;
    const Meter maxWidth = Feature(gInputParameter.maxNmosSize) * gTech.featureSize;
    const double pnRatio = gTech.pnSizeRatio;

    auto gateCapacitance = [] (Meter width) {
        return NmosTransistor(gTech, width / gTech.featureSize, constant::maxTransistorHeight).gateCapacitance();
    };
    auto stageCount = [] (double totalEffort) {
        return std::max(0, static_cast<int>(std::log(totalEffort) / std::log(constant::optimalStageEffort) + 0.5) - 1);
    };

    const Meter minDriverWidth = std::max(minWidth, Meter(CalculateLayoutWidth((_minDriverCurrent / gTech.currentOnNmos[gInputParameter.temperature - 300]).value(), gTech)));

    if (minDriverWidth > maxWidth) {
        return chain;
    }

    // NMOS widths from input to output, one spare entry so a chain clamped to the maximum length still fits
    std::array<Meter, constant::maxInvChainLength + 1> nmosWidths;
    int stageTotal = 0;
    BufferDesignTarget level = _areaOptimizationLevel;

    // Fills the stages before the last one by shrinking each stage by the per stage effort
    auto taper = [&nmosWidths, &minWidth] (int last, double stageEffort) {
        for (int i = last - 1; i >= 0; i--) {
            nmosWidths[i] = std::max(minWidth, nmosWidths[i+1] / stageEffort);
        }
    };

    if (level == latency_first) {
        double totalEffort = std::max<double>(1.0, _logicEffort * _outputCapacitance / _inputCapacitance);
        int optimalStages = stageCount(totalEffort);
        if ((optimalStages % 2) ^ _inverted) { // Keep the output polarity
            optimalStages += 1;
        }
        optimalStages = std::min(optimalStages, constant::maxInvChainLength);
        stageTotal = optimalStages;

        double stageEffort = std::pow(totalEffort, 1.0 / (optimalStages + 1));
        Meter& last = nmosWidths[optimalStages-1];
        const Meter lastWidth = _outputCapacitance / stageEffort / (gateCapacitance(1_m) / 1_m) / (1.0 + pnRatio);
        last = std::max(minWidth, lastWidth);
        if (last > maxWidth) {
            last = maxWidth;
            // Recalculate the effort with the capped last stage
            totalEffort = _logicEffort * gateCapacitance((1 + pnRatio) * maxWidth) / _inputCapacitance;
            stageEffort = std::pow(totalEffort, 1.0 / optimalStages);
        }

        if (last < minDriverWidth) {
            // Rhe last stage cannot provide the minimum current, so logical effort alone cannot size the chain
            level = latency_area_trade_off;
        } else {
            taper(optimalStages - 1, stageEffort);
        }
    }

    if (level == latency_area_trade_off) {
        Farad newOutputCapacitance = gateCapacitance(minDriverWidth) * (1.0 + pnRatio);
        double totalEffort = std::max<double>(1.0, _logicEffort * newOutputCapacitance / _inputCapacitance);
        int optimalStages = stageCount(totalEffort);
        if (!((optimalStages % 2) ^ _inverted)) { // Keep the output polarity
            optimalStages += 1;
        }
        optimalStages = std::min(optimalStages, constant::maxInvChainLength);
        stageTotal = optimalStages + 1;

        nmosWidths[optimalStages] = minDriverWidth;
        taper(optimalStages, std::pow(totalEffort, 1.0 / (optimalStages + 1)));
    } else if (level == area_first) {
        stageTotal = 1;
        nmosWidths[0] = minDriverWidth;

        if (nmosWidths[0] > constant::areaOptimizationConstraint * maxWidth) {
            return chain;
        }
    }

    for (int i = 0; i < stageTotal; i++) {
        const Meter pmosWidth = nmosWidths[i] * pnRatio;
        CmosInverter gate(gTech, nmosWidths[i] / gTech.featureSize, pmosWidth / gTech.featureSize, constant::maxTransistorHeight);
        chain.stages[i] = OutputDriverStage{
            .nmosWidth = nmosWidths[i],
            .pmosWidth = pmosWidth,
            .inputCapacitance = gate.gateCapacitance(),
            .outputCapacitance = gate.drainCapacitance(),
        };
    }
    chain.size = stageTotal;

    return chain;
}

mse::Dimensions mse::component::OutputDriver::calculateDimensions() const noexcept {
    using namespace mse::unit;

    if (stages().empty()) {
        return Dimensions{.width = Meter(constant::invalidValue), .height = Meter(constant::invalidValue)};
    }

    Dimensions dimensions;
    for (const OutputDriverStage& stage : stages()) {
        Dimensions stageDimensions = CmosInverter(gTech, stage.nmosWidth / gTech.featureSize, stage.pmosWidth / gTech.featureSize, 40_feat).dimensions();
        dimensions.width += stageDimensions.width;
        dimensions.height = std::max(dimensions.height, stageDimensions.height);
    }

    return dimensions;
}

mse::component::RampedTiming mse::component::OutputDriver::calculateTiming(physics::HorowitzRamp inputRamp) const noexcept {
    using namespace mse::unit;

    RampedTiming timing;
    timing.outputRamp = inputRamp;

    if (stages().empty()) {
        timing.readLatency = timing.writeLatency = Second(constant::invalidValue);

        return timing;
    }

    const std::span<const OutputDriverStage> chain = stages();
    Second latency;
    for (size_t i = 0; i < chain.size(); i++) {
        const bool lastStage = i + 1 == chain.size();
        NmosTransistor pullDown(gTech, chain[i].nmosWidth / gTech.featureSize, constant::maxTransistorHeight);
        Ohm pullDownResistance = pullDown.channelResistance();
        Farad loadCapacitance = chain[i].outputCapacitance + (lastStage ? _outputCapacitance : chain[i+1].inputCapacitance);
        Second tau = pullDownResistance * loadCapacitance;
        if (lastStage) {
            tau += Second(_outputCapacitance * _outputResistance) / 2;
        }
        double gain = pullDownResistance * pullDown.transconductance();
        physics::HorowitzResult stage = physics::horowitz(tau, 1 / gain, timing.outputRamp);
        latency += stage.delay;
        timing.outputRamp = stage.outputRamp;
    }

    timing.readLatency = timing.writeLatency = latency;

    return timing;
}

mse::component::BasicPower mse::component::OutputDriver::calculatePower() const noexcept {
    using namespace mse::unit;

    BasicPower power;

    if (stages().empty()) {
        power.dynamicReadEnergy = power.dynamicWriteEnergy = Joule(constant::invalidValue);
        power.leakage = Watt(constant::invalidValue);

        return power;
    }

    const std::span<const OutputDriverStage> chain = stages();
    Ampere leakageCurrent;
    Farad switchedCapacitance;
    for (size_t i = 0; i < chain.size(); i++) {
        leakageCurrent += CmosInverter(gTech, chain[i].nmosWidth / gTech.featureSize, chain[i].pmosWidth / gTech.featureSize, constant::maxTransistorHeight).leakageCurrent();
        // The last stage drives the final load capacitance
        switchedCapacitance += chain[i].outputCapacitance + (i + 1 == chain.size() ? _outputCapacitance : chain[i+1].inputCapacitance);
    }

    power.dynamicReadEnergy = power.dynamicWriteEnergy = switchedCapacitance * gTech.vdd * gTech.vdd;
    power.leakage = leakageCurrent * gTech.vdd;

    return power;
}
