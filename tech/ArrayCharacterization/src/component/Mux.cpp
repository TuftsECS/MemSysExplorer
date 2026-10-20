#include "component/Mux.hpp"
#include "cmosDevice.hpp"
#include "global.hpp"

mse::unit::Meter mse::component::Mux::calculatePassTransistorWidth() const noexcept {
    using namespace mse::unit;

    if (!enabled()) {
        return Meter();
    }

    const Number finFactor = gTech.featureSize <= 14*1e-9 ? 2 : 1;
    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{gTech.featureSize};
    const Meter minWidth = 6 * constant::minNmosSize * featureSize;
    const Meter minDriverWidth(_minDriverCurrent.value() / gTech.currentOnNmos[gInputParameter.temperature - 300]);

    const bool resistiveCell = gCell.memCellType == MRAM || gCell.memCellType == PCRAM || gCell.memCellType == memristor
            || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM;

    if (!resistiveCell) {
        return std::max(minWidth, minDriverWidth);
    }

    // Mux resistance should be small enough for voltage dividing, capped to avoid severe IR drop
    const Ohm maxResistance(gCell.resistanceOn * constant::irDropTolerance);
    const Ohm unitResistance = NmosTransistor(gTech, Feature(finFactor), 40_feat * finFactor).channelResistance();
    Meter width = unitResistance * Meter(gTech.featureSize) / maxResistance;
    width = std::min(width, Feature(gInputParameter.maxNmosSize) * featureSize);

    return std::max({width, minDriverWidth, minWidth});
}

mse::component::MuxRC mse::component::Mux::calculateRC() const noexcept {
    using namespace mse::unit;

    MuxRC rc;
    if (enabled()) {
        const Number finFactor = gTech.featureSize <= 14*1e-9 ? 2 : 1;
        const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{gTech.featureSize};
        NmosTransistor transistor(gTech, passTransistorWidth() / featureSize, 40_feat * finFactor);
        rc.passCapacitance = transistor.drainCapacitance();
        rc.passResistance = transistor.channelResistance();
        rc.outputCapacitance = _inputs * rc.passCapacitance;
        rc.delayLoadCapacitance = rc.outputCapacitance + rc.passCapacitance + _loadCapacitance;
        rc.powerLoadCapacitance = rc.passCapacitance;
    }

    return rc;
}

mse::Dimensions mse::component::Mux::calculateDimensions() const noexcept {
    using namespace mse::unit;

    if (!enabled()) {
        return Dimensions();
    }

    const Number finFactor = gTech.featureSize <= 14*1e-9 ? 2 : 1;
    const Unit<MeterAtom<>, FeatureAtom<-1>> featureSize{gTech.featureSize};
    Dimensions transistor = NmosTransistor(gTech, passTransistorWidth() / featureSize, 40_feat * finFactor).dimensions();

    return Dimensions{
        .width = _muxes * _inputs * transistor.width,
        .height = transistor.height
    };
}

mse::component::BasicTiming mse::component::Mux::calculateTiming() const noexcept {
    using namespace mse::unit;

    BasicTiming timing;
    if (enabled()) {
        timing.readLatency = timing.writeLatency = 2.3 * rc().passResistance * (rc().outputCapacitance + _loadCapacitance);
    }

    return timing;
}

mse::component::BasicPower mse::component::Mux::calculatePower() const noexcept {
    using namespace mse::unit;

    BasicPower power; // TODO leakage
    if (enabled()) {
        const Volt vdd(gTech.vdd);
        Joule energy = (rc().outputCapacitance + _nextStageCapacitance) * vdd * (vdd - Volt(gTech.vth));
        power.dynamicReadEnergy = power.dynamicWriteEnergy = energy * _muxes;
    }

    return power;
}
