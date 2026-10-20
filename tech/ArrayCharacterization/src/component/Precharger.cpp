#include "component/Precharger.hpp"
#include "cmosDevice.hpp"
#include "global.hpp"

static constexpr mse::unit::Feature equalizerWidth = mse::constant::minNmosSize;
static constexpr mse::unit::Feature prechargerWidth{6.0};

mse::component::Precharger::Precharger(unit::Volt prechargeVoltage, long long columns, unit::Farad bitlineCapacitance, unit::Ohm bitlineResistance)
        : _prechargeVoltage(prechargeVoltage),
          _columns(columns),
          _bitlineCapacitance(bitlineCapacitance),
          _bitlineResistance(bitlineResistance) {}

bool mse::component::Precharger::setup() noexcept {
    using namespace mse::unit;

    const Meter cellWidth = Feature(gCell.widthInFeatureSize) * Unit<MeterAtom<>, FeatureAtom<-1>>(gTech.featureSize);
    const Farad wireLoadPerColumn(cellWidth.value() * gLocalWire.capWirePerUnit);
    const Farad inverterInputCapacitance = CmosInverter(gTech, constant::minNmosSize, constant::minNmosSize * gTech.pnSizeRatio, 40_feat).gateCapacitance();
    const Farad loadPerColumn = inverterInputCapacitance + wireLoadPerColumn;

    return build(_outputDriver, 1, inverterInputCapacitance, _columns * loadPerColumn, Ohm(), true, latency_first, Ampere()); // Always Latency First, TODO output resistance
}

mse::component::Precharger::RC mse::component::Precharger::calculateRC() const noexcept {
    using namespace mse::unit;

    const PmosTransistor equalizer(gTech, equalizerWidth, 40_feat);
    const PmosTransistor precharger(gTech, prechargerWidth, 40_feat);

    RC rc;
    rc.inverterLoadCapacitance = equalizer.gateCapacitance() + 2 * precharger.gateCapacitance()
            + NmosTransistor(gTech, constant::minNmosSize, 40_feat).drainCapacitance()
            + PmosTransistor(gTech, constant::minNmosSize * gTech.pnSizeRatio, 40_feat).drainCapacitance();
    rc.prechargerOutputCapacitance = precharger.drainCapacitance() + equalizer.drainCapacitance();

    return rc;
}

mse::Dimensions mse::component::Precharger::calculateDimensions() const noexcept {
    using namespace mse::unit;

    const Dimensions precharger = PmosTransistor(gTech, prechargerWidth, 40_feat).dimensions();
    const Dimensions equalizer = PmosTransistor(gTech, equalizerWidth, 40_feat).dimensions();
    const Dimensions inverter = CmosInverter(gTech, constant::minNmosSize, constant::minNmosSize * gTech.pnSizeRatio, 40_feat).dimensions();
    const Dimensions& driver = _outputDriver.dimensions();

    const Meter columnWidth = std::max(2 * precharger.width + equalizer.width, inverter.width);

    return Dimensions{
        .width = std::max(columnWidth * _columns, driver.width),
        .height = std::max(std::max(precharger.height, equalizer.height) + inverter.height, driver.height)
    };
}

mse::component::RampedTiming mse::component::Precharger::calculateTiming(physics::HorowitzRamp inputRamp) const noexcept {
    using namespace mse::unit;

    _outputDriver.setInputRamp(inputRamp);

    // The enable inverter switches the precharger gates
    NmosTransistor pullDown(gTech, constant::minNmosSize, 40_feat);
    Ohm pullDownResistance = pullDown.channelResistance();
    double pullDownGain = pullDownResistance * pullDown.transconductance();
    physics::HorowitzResult enable = physics::horowitz(pullDownResistance * rc().inverterLoadCapacitance, 1 / pullDownGain, _outputDriver.timing().outputRamp);

    // The precharger pulls up the bitline
    PmosTransistor pullUp(gTech, prechargerWidth, 40_feat);
    Ohm pullUpResistance = pullUp.channelResistance();
    double pullUpGain = pullUpResistance * pullUp.transconductance();
    Second tau = pullUpResistance * (_bitlineCapacitance + rc().prechargerOutputCapacitance) + _bitlineResistance * _bitlineCapacitance / 2;
    physics::HorowitzResult precharge = physics::horowitz(tau, 1 / pullUpGain, enable.outputRamp);

    RampedTiming timing;
    timing.readLatency = timing.writeLatency = precharge.delay;
    timing.outputRamp = precharge.outputRamp;

    return timing;
}

mse::component::BasicPower mse::component::Precharger::calculatePower() const noexcept {
    using namespace mse::unit;

    const Volt vdd(gTech.vdd);
    const Watt inverterLeakage = _columns * vdd * CmosInverter(gTech, constant::minNmosSize, constant::minNmosSize * gTech.pnSizeRatio, 40_feat).leakageCurrent();
    const Watt prechargerLeakage = _columns * _prechargeVoltage * PmosTransistor(gTech, prechargerWidth, 40_feat).leakageCurrent();
    const Joule inverterEnergy = rc().inverterLoadCapacitance * vdd * vdd;

    // Bitline precharge energy is not counted because it is a charging process, and no precharging is needed during the write operation
    BasicPower power;
    power.leakage = _outputDriver.power().leakage + inverterLeakage + prechargerLeakage;
    power.dynamicReadEnergy = _outputDriver.power().dynamicReadEnergy + inverterEnergy * _columns;

    return power;
}
