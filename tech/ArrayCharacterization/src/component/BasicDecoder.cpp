#include "component/BasicDecoder.hpp"
#include "cmosDevice.hpp"
#include "global.hpp"

mse::component::BasicDecoder::BasicDecoder(int addressBits, unit::Farad loadCapacitance, unit::Ohm loadResistance)
        : _addressBits(addressBits),
          _loadCapacitance(loadCapacitance),
          _loadResistance(loadResistance) {}

bool mse::component::BasicDecoder::setup() noexcept {
    using namespace mse::unit;

    // A single address bit only needs inverters, otherwise one NAND input per address bit, always Latency First
    if (nandInputs() == 0) {
        Farad inverterCapacitance = CmosInverter(gTech, constant::minNmosSize, gTech.pnSizeRatio * constant::minNmosSize, constant::maxTransistorHeight).gateCapacitance();

        return build(_outputDriver, 1, inverterCapacitance, _loadCapacitance, _loadResistance, true, latency_first, Ampere());
    }

    CmosNand<> gate(gTech, nandInputs(), constant::minNmosSize, gTech.pnSizeRatio * constant::minNmosSize, constant::maxTransistorHeight);

    return build(_outputDriver, gate.logicalEffort(), gate.gateCapacitance(), _loadCapacitance, _loadResistance, true, latency_first, Ampere());
}

mse::unit::Farad mse::component::BasicDecoder::calculateNandOutputCapacitance() const noexcept {
    using namespace mse::unit;

    if (nandInputs() == 0) {
        return Farad();
    }

    return CmosNand<>(gTech, nandInputs(), constant::minNmosSize, gTech.pnSizeRatio * constant::minNmosSize, constant::maxTransistorHeight).drainCapacitance();
}

mse::Dimensions mse::component::BasicDecoder::calculateDimensions() const noexcept {
    using namespace mse::unit;

    const Dimensions& driver = _outputDriver.dimensions();

    if (nandInputs() == 0) {
        return Dimensions{.width = driver.width, .height = 2 * driver.height};
    }

    const Dimensions gate = CmosNand<>(gTech, nandInputs(), constant::minNmosSize, gTech.pnSizeRatio * constant::minNmosSize, 40_feat).dimensions();

    return Dimensions{.width = gate.width + driver.width, .height = std::max(gate.height, driver.height) * nandGates()};
}

mse::component::RampedTiming mse::component::BasicDecoder::calculateTiming(physics::HorowitzRamp inputRamp) const noexcept {
    using namespace mse::unit;

    Second latency;
    physics::HorowitzRamp driverRamp = inputRamp;
    if (nandInputs() > 0) {
        CmosNand<> gate(gTech, nandInputs(), constant::minNmosSize, gTech.pnSizeRatio * constant::minNmosSize, constant::maxTransistorHeight);
        Ohm pullDownResistance = gate.nmosResistance();
        double gain = pullDownResistance * NmosTransistor(gTech, gate.nmosWidth(), constant::maxTransistorHeight).transconductance();
        Second tau = pullDownResistance * (nandOutputCapacitance() + _outputDriver.stages().front().inputCapacitance);
        physics::HorowitzResult stage = physics::horowitz(tau, 1 / gain, inputRamp);
        latency = stage.delay;
        driverRamp = stage.outputRamp;
    }
    _outputDriver.setInputRamp(driverRamp);

    RampedTiming timing;
    timing.readLatency = timing.writeLatency = latency + _outputDriver.timing().readLatency;
    timing.outputRamp = _outputDriver.timing().outputRamp;

    return timing;
}

mse::component::BasicPower mse::component::BasicDecoder::calculatePower() const noexcept {
    using namespace mse::unit;

    const BasicPower& driver = _outputDriver.power();
    const OutputDriverStage& firstStage = _outputDriver.stages().front();

    // Only one output switches each time
    BasicPower power;
    if (nandInputs() == 0) {
        const Joule inverterEnergy = (firstStage.inputCapacitance + firstStage.outputCapacitance) * gTech.vdd * gTech.vdd;
        power.leakage = 2 * driver.leakage;
        power.dynamicReadEnergy = inverterEnergy + driver.dynamicReadEnergy;
    } else {
        const Watt nandLeakage = CmosNand<>(gTech, nandInputs(), constant::minNmosSize, gTech.pnSizeRatio * constant::minNmosSize, constant::maxTransistorHeight).leakageCurrent() * gTech.vdd;
        const Joule nandEnergy = (nandOutputCapacitance() + firstStage.inputCapacitance) * gTech.vdd * gTech.vdd;
        power.leakage = (nandLeakage + driver.leakage) * nandGates();
        power.dynamicReadEnergy = nandEnergy + driver.dynamicReadEnergy;
    }
    power.dynamicWriteEnergy = power.dynamicReadEnergy;

    return power;
}
