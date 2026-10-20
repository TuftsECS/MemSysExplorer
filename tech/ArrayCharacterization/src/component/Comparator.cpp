#include "component/Comparator.hpp"
#include "cmosDevice.hpp"
#include "passDevice.hpp"
#include "global.hpp"

static constexpr mse::unit::Feature inverterNmosWidths[mse::constant::comparatorInvChainLength] = {mse::unit::Feature(7.5), mse::unit::Feature(15.0), mse::unit::Feature(30.0), mse::unit::Feature(50.0)};
static constexpr mse::unit::Feature inverterPmosWidths[mse::constant::comparatorInvChainLength] = {mse::unit::Feature(12.5), mse::unit::Feature(25.0), mse::unit::Feature(50.0), mse::unit::Feature(100.0)};
static constexpr mse::unit::Feature compareNmosWidth{12.5};
static constexpr mse::unit::Feature comparePmosWidth{37.5};

mse::component::Comparator::RC mse::component::Comparator::calculateRC() const noexcept {
    using namespace mse::unit;

    RC rc;
    for (int i = 0; i < chainLength; i++) {
        CmosInverter inverter(gTech, inverterNmosWidths[i], inverterPmosWidths[i], constant::maxTransistorHeight);
        rc.inputCapacitances[i] = inverter.gateCapacitance();
        rc.outputCapacitances[i] = inverter.drainCapacitance();
    }

    // Each bit compares through two series NMOS, there is no PMOS so it does not leak
    const Farad compareCapacitance = PassNmosStack<2>(gTech, compareNmosWidth, 40_feat).drainCapacitance();
    rc.bottomCapacitance = rc.outputCapacitances[chainLength-1] + quarterTagBits() * compareCapacitance;
    rc.topCapacitance = quarterTagBits() * compareCapacitance + PmosTransistor(gTech, comparePmosWidth, constant::maxTransistorHeight).drainCapacitance() + _loadCapacitance;
    rc.bottomResistance = NmosTransistor(gTech, inverterNmosWidths[chainLength-1], constant::maxTransistorHeight).channelResistance();
    rc.topResistance = PassNmosStack<2>(gTech, compareNmosWidth, constant::maxTransistorHeight).resistance();

    return rc;
}

mse::Dimensions mse::component::Comparator::calculateDimensions() const noexcept {
    using namespace mse::unit;

    Meter chainWidth;
    Meter chainHeight;
    for (int i = 0; i < chainLength; i++) {
        Dimensions inverter = CmosInverter(gTech, inverterNmosWidths[i], inverterPmosWidths[i], 40_feat).dimensions();
        chainHeight = std::max(chainHeight, inverter.height);
        chainWidth += inverter.width;
    }
    Dimensions compare = PassNmosStack<2>(gTech, compareNmosWidth, 40_feat).dimensions();

    // 4 quarter comparators side by side, they could be placed differently
    return Dimensions{
        .width = std::max(chainWidth, quarterTagBits() * compare.width) * 4,
        .height = chainHeight + compare.height
    };
}

mse::component::RampedTiming mse::component::Comparator::calculateTiming(physics::HorowitzRamp inputRamp) const noexcept {
    using namespace mse::unit;

    RampedTiming timing;
    timing.outputRamp = inputRamp;
    Second latency;
    for (int i = 0; i < chainLength - 1; i++) {
        NmosTransistor pullDown(gTech, inverterNmosWidths[i], constant::maxTransistorHeight);
        Ohm pullDownResistance = pullDown.channelResistance();
        Second tau = pullDownResistance * (rc().outputCapacitances[i] + rc().inputCapacitances[i+1]);
        double gain = pullDownResistance * pullDown.transconductance();
        physics::HorowitzResult stage = physics::horowitz(tau, 1 / gain, timing.outputRamp);
        latency += stage.delay;
        timing.outputRamp = stage.outputRamp;
    }

    Second tau = rc().bottomResistance * rc().bottomCapacitance + (rc().bottomResistance + rc().topResistance) * rc().topCapacitance;
    physics::HorowitzResult compare = physics::horowitz(tau, 0, timing.outputRamp);
    latency += compare.delay;
    timing.outputRamp = compare.outputRamp;

    timing.readLatency = timing.writeLatency = latency;

    return timing;
}

mse::component::BasicPower mse::component::Comparator::calculatePower() const noexcept {
    using namespace mse::unit;

    const Volt vdd(gTech.vdd);

    Ampere leakageCurrent;
    for (int i = 0; i < chainLength; i++) {
        leakageCurrent += CmosInverter(gTech, inverterNmosWidths[i], inverterPmosWidths[i], constant::maxTransistorHeight).leakageCurrent();
    }

    Farad switchedCapacitance;
    for (int i = 0; i < chainLength - 1; i++) {
        switchedCapacitance += rc().outputCapacitances[i] + rc().inputCapacitances[i+1];
    }
    switchedCapacitance += rc().bottomCapacitance + rc().topCapacitance;

    // 4 quarter comparators
    BasicPower power;
    power.dynamicReadEnergy = power.dynamicWriteEnergy = switchedCapacitance * vdd * vdd * 4;
    power.leakage = leakageCurrent * vdd * 4;

    return power;
}
