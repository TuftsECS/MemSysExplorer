#include "component/RowDecoder.hpp"
#include "cmosDevice.hpp"
#include "global.hpp"

mse::component::RowDecoder::RowDecoder(int rows, unit::Farad loadCapacitance, unit::Ohm loadResistance, bool multipleRowsPerSet,
                                       BufferDesignTarget areaOptimizationLevel, unit::Ampere minDriverCurrent)
        : _rows(rows),
          _loadCapacitance(loadCapacitance),
          _loadResistance(loadResistance),
          _multipleRowsPerSet(multipleRowsPerSet),
          _areaOptimizationLevel(areaOptimizationLevel),
          _minDriverCurrent(minDriverCurrent) {}

bool mse::component::RowDecoder::setup() noexcept {
    using namespace mse::unit;

    if (nandInputs() > 0) {
        CmosNand<> gate(gTech, nandInputs(), constant::minNmosSize, gTech.pnSizeRatio * constant::minNmosSize, constant::maxTransistorHeight);

        return build(_outputDriver, gate.logicalEffort(), gate.gateCapacitance(), _loadCapacitance, _loadResistance, true, _areaOptimizationLevel, _minDriverCurrent);
    }

    // Only a 1 level output buffer drives the wordline
    Farad inverterCapacitance = CmosInverter(gTech, constant::minNmosSize, gTech.pnSizeRatio * constant::minNmosSize, constant::maxTransistorHeight).gateCapacitance();

    return build(_outputDriver, 1, inverterCapacitance, _loadCapacitance, _loadResistance, true, _areaOptimizationLevel, _minDriverCurrent);
}

int mse::component::RowDecoder::nandInputs() const noexcept {
    if (_rows <= 8) {
        // The predecoder output is used directly, or NANDed with the way select
        return _multipleRowsPerSet ? 2 : 0;
    }

    // NAND two predecoder outputs, plus the way select
    return _multipleRowsPerSet ? 3 : 2;
}

mse::component::RowDecoder::NandCapacitance mse::component::RowDecoder::calculateNandCapacitance() const noexcept {
    using namespace mse::unit;

    NandCapacitance capacitance;
    if (nandInputs() > 0) {
        CmosNand<> gate(gTech, nandInputs(), constant::minNmosSize, gTech.pnSizeRatio * constant::minNmosSize, constant::maxTransistorHeight);
        capacitance.input = gate.gateCapacitance();
        capacitance.output = gate.drainCapacitance();
    }

    return capacitance;
}

mse::Dimensions mse::component::RowDecoder::calculateDimensions() const noexcept {
    using namespace mse::unit;

    const Dimensions& driver = _outputDriver.dimensions();

    if (nandInputs() == 0) {
        return Dimensions{.width = driver.width, .height = driver.height * _rows};
    }

    const Dimensions gate = CmosNand<>(gTech, nandInputs(), constant::minNmosSize, gTech.pnSizeRatio * constant::minNmosSize, 40_feat).dimensions();

    return Dimensions{.width = gate.width + driver.width, .height = std::max(gate.height, driver.height) * _rows};
}

mse::component::RampedTiming mse::component::RowDecoder::calculateTiming(physics::HorowitzRamp inputRamp) const noexcept {
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

mse::component::BasicPower mse::component::RowDecoder::calculatePower() const noexcept {
    using namespace mse::unit;

    const BasicPower& driver = _outputDriver.power();

    BasicPower power;
    power.leakage = driver.leakage;
    power.dynamicReadEnergy = driver.dynamicReadEnergy;
    if (nandInputs() > 0) {
        const Watt nandLeakage = CmosNand<>(gTech, nandInputs(), constant::minNmosSize, gTech.pnSizeRatio * constant::minNmosSize, constant::maxTransistorHeight).leakageCurrent() * gTech.vdd;
        // DRAM wordlines are overdriven to Vpp, only one row switches each time
        const Volt swing(gCell.memCellType == DRAM || gCell.memCellType == eDRAM ? gTech.vpp : gTech.vdd);
        const Joule nandEnergy = (nandOutputCapacitance() + _outputDriver.stages().front().inputCapacitance) * swing * swing;
        power.leakage = nandLeakage + driver.leakage;
        power.dynamicReadEnergy = nandEnergy + driver.dynamicReadEnergy;
    }
    power.dynamicWriteEnergy = power.dynamicReadEnergy;
    power.leakage *= _rows;

    return power;
}
