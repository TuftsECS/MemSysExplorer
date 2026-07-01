// This file contains visitor types for using on each cell type
// Visitors (within the visitor pattern) have overloaded functions for each type they operate on
// Each visitor is responsible for doing one type of computation, and they all must return the same type

#ifndef MSE_CELL_VISITORS_HPP
#define MSE_CELL_VISITORS_HPP

#include "unit.hpp"
#include "cell/types.hpp"

#include <functional>

namespace mse {

// This struct adds a throwing overload for monostate types to use in visitors
// Inherit from this whenever a variant can hold a monostate type but should not ever encounter it
template <typename T>
struct MonostateThrow {
    [[noreturn]] T operator()(const std::monostate&) const {
        throw std::runtime_error("std::monostate value found in visitor");
    }
};

// Simple visitor type for small use
template <typename... Ts>
struct GenericVisitor : Ts... {
    using Ts::operator()...;
};

// Helper type for some return type T and overload target type U
// Useful with a pack expansion to create pure virtual visitor overloads on demand
template <typename T, typename U>
struct SingleVisitor {
    virtual T operator()(const U& target) = 0;
};

// TODO move somewhere else
template <typename T>
static inline constexpr T* variantBasePtr(auto& variant) {
    return std::visit(
        [](auto& value) -> T* {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::monostate>) {
                return nullptr;
            } else {
                return &value;
            }
        }, variant);
}

// TODO move somewhere else
// a simple class acting as a calculator for delay to hide some local variables
class HorowitzCalculator {
public:

    // tauFalling = tau value of the RC tree during a falling output
    // beta = normalized ratio between transconductance of driviing element and its output resistance, defined as 1 / (gm * Rf), gm = transconductance, Rf = resistance of falling transient
    // rampInput = normalized slope of input voltage ramp curve per second
    // rampOutput = normalized slope of output voltage ramp curve per second
    static unit::Second calculate(unit::Second tauFalling, unit::Number beta, unit::SecondExp<-1> rampInput) {
        unit::Number alpha = 1.0 / (rampInput * tauFalling);
        return sqrt(log(normalizedSwitchingVoltage) * log(normalizedSwitchingVoltage) + 2.0 * alpha * beta * (1.0 - normalizedSwitchingVoltage))
               * tauFalling;
    }

    static unit::Second calculate(unit::Second tauFalling, unit::Number beta, unit::SecondExp<-1> rampInput, unit::SecondExp<-1>& rampOutput) {
        unit::Second delay = HorowitzCalculator::calculate(tauFalling, beta, rampInput);
        rampOutput = (1.0 - normalizedSwitchingVoltage) / delay;
        return delay;
    }

private:

    // normalized voltage where the input voltage changes device operation
    static inline constexpr unit::Number normalizedSwitchingVoltage = 0.5;
};

} // namespace mse

namespace mse::cell {

// Helper type for creating visitors from the MemoryCellList in include/cell/types.hpp
template <typename, typename>
struct MemoryCellVisitor;

// This specialization allows for simple visitor construction from a list of cell types
// It assumes monostate types should not be allowed
// Pure virtual functions for type overloads are automatically added with the SingleVisitor expansion
template <typename T, typename... Us>
struct MemoryCellVisitor<T, CellList<Us...>> : MonostateThrow<T>, SingleVisitor<T, Us>... {
    using MonostateThrow<T>::operator();
    using SingleVisitor<T, Us>::operator()...;
    virtual ~MemoryCellVisitor() = default;
};

//==================================================
// Visitor types below calculate different results for cells
// When a cell is added in include/cell/types.hpp, an overload for it must be provided in these visitors
// Each overload is specific to a single cell and should use respective cell subtypes
//   Meaning overloads for MyCell should return MyCell::Latency, MyCell::Energy, ...

struct LatencyVisitor final : MemoryCellVisitor<MemoryCellLatency, MemoryCellList> {
    using MemoryCellVisitor<MemoryCellLatency, MemoryCellList>::operator();

    std::reference_wrapper<SubArray> subarray;

    LatencyVisitor(SubArray& subarray) : subarray(subarray) {}

    // MemoryCellLatency operator()(const CustomCell&) override {
    //   ...
    // }

    MemoryCellLatency operator()(const Sram6tCell& sram6tCell) override {
        using namespace mse::unit;
        const Sram6tTopology& topology = dynamic_cast<const Sram6tTopology&>(sram6tCell.topology());
        SubArray& sub = subarray;
        Farad bitlineCapacitance = Farad(sub.capBitline);
        Ohm bitlineResistance = Ohm(sub.resBitline);
        Farad bitlineMuxCapacitance = Farad(sub.bitlineMux.capForPreviousDelayCalculation);

        int bitline = 0; // TODO
        Ohm cellToBitlineResistance = topology.bitlinePullDownResistance(bitline);
        Farad cellToBitlineCapacitance = topology.bitlineCapacitance(bitline);
        Number beta = topology.beta(bitline);
        Volt prechargeVoltage = sram6tCell.properties().prechargeVoltage;
        Volt sensingVoltage = sram6tCell.properties().sensingVoltage;

        Second tau
            = cellToBitlineResistance * cellToBitlineCapacitance
            + cellToBitlineResistance * (bitlineCapacitance / 2.0)
            + (cellToBitlineResistance + bitlineResistance) * (bitlineCapacitance / 2.0)
            + (cellToBitlineResistance + bitlineResistance) * bitlineMuxCapacitance;

        tau *= log(prechargeVoltage / (prechargeVoltage - sensingVoltage / 2.0));
        // one signal raises and the other drops, so sensing voltage / 2 is enough

        mse::unit::SecondExp<-1> inputRamp = mse::unit::SecondExp<-1>(sub.rowDecoder.rampOutput);
        Sram6tCell::Latency latency;
        Second bitlineDelay = HorowitzCalculator::calculate(tau, beta, inputRamp);
        latency.bitline = bitlineDelay;
        latency.read = bitlineDelay + Second(sub.bitlineMux.readLatency);
		double decoderLatency = std::max(sub.rowDecoder.readLatency, sub.columnDecoderLatency);
        latency.write = Second(decoderLatency + bitlineDelay.value() + sub.bitlineMux.readLatency + sub.senseAmp.readLatency + sub.senseAmpMuxLev1.readLatency + sub.senseAmpMuxLev2.readLatency);
        return latency;
    }

    MemoryCellLatency operator()(const DramCell& dramCell) override {
        using namespace mse::unit;
        const DramTopology& topology = dynamic_cast<const DramTopology&>(dramCell.topology());
        SubArray sub = subarray;
        Farad bitlineCapacitance = Farad(sub.capBitline);
        Ohm bitlineResistance = Ohm(sub.resBitline);

        Farad cellCap = topology.bitlineCapacitance(0) + topology.storageCapacitance();
        Farad lineCap = bitlineCapacitance + Farad(sub.bitlineMux.capForPreviousDelayCalculation);
        Farad effectiveCapacitance = cellCap * lineCap / (cellCap + lineCap);

        Second tau = (bitlineResistance + topology.bitlinePullDownResistance(0, 0)) * effectiveCapacitance * 2.3;

        mse::unit::SecondExp<-1> inputRamp = mse::unit::SecondExp<-1>(sub.rowDecoder.rampOutput);
        DramCell::Latency latency;
        Second bitlineDelay = HorowitzCalculator::calculate(tau, Number(0.0), inputRamp);
        latency.bitline = bitlineDelay;
        latency.read = bitlineDelay;
		double decoderLatency = std::max(sub.rowDecoder.readLatency, sub.columnDecoderLatency);
        latency.write = Second(decoderLatency + bitlineDelay.value() + sub.senseAmp.readLatency + sub.senseAmpMuxLev1.readLatency + sub.senseAmpMuxLev2.readLatency);
        // refresh doesnt pass sense amplifier
        latency.refresh = Second(decoderLatency + bitlineDelay.value() + sub.senseAmp.readLatency) * sub.numRow;
        return latency;
    }

    MemoryCellLatency operator()(const EdramCell& edramCell) override {
        using namespace mse::unit;
        const EdramTopology& topology = dynamic_cast<const EdramTopology&>(edramCell.topology());
        SubArray sub = subarray;
        Farad bitlineCapacitance = Farad(sub.capBitline);
        Ohm bitlineResistance = Ohm(sub.resBitline);

        Farad cellCap = topology.bitlineCapacitance(0) + topology.storageCapacitance();
        Farad lineCap = bitlineCapacitance + Farad(sub.bitlineMux.capForPreviousDelayCalculation);
        Farad effectiveCapacitance = cellCap * lineCap / (cellCap + lineCap);

        Second tau = (bitlineResistance + topology.bitlinePullDownResistance(0, 0)) * effectiveCapacitance * 2.3;

        SecondExp<-1> inputRamp = SecondExp<-1>(sub.rowDecoder.rampOutput);
        DramCell::Latency latency;
        Second bitlineDelay = HorowitzCalculator::calculate(tau, Number(0.0), inputRamp);
        latency.bitline = bitlineDelay;
        latency.read = bitlineDelay;
		double decoderLatency = std::max(sub.rowDecoder.readLatency, sub.columnDecoderLatency);
        latency.write = Second(decoderLatency + bitlineDelay.value() + sub.senseAmp.readLatency + sub.senseAmpMuxLev1.readLatency + sub.senseAmpMuxLev2.readLatency);
        // refresh doesnt pass sense amplifier
        latency.refresh = Second(decoderLatency + bitlineDelay.value() + sub.senseAmp.readLatency) * sub.numRow;
        return latency;
    }

    MemoryCellLatency operator()(const Edram3tCell& edram3tCell) override {
        using namespace mse::unit;
        const Edram3tTopology& topology = dynamic_cast<const Edram3tTopology&>(edram3tCell.topology());
        SubArray sub = subarray;
        Farad bitlineCapacitance = Farad(sub.capBitline);
        Ohm bitlineResistance = Ohm(sub.resBitline);

        Farad capR = topology.bitlineCapacitance(0) + topology.storageCapacitance() + bitlineCapacitance + Farad(sub.bitlineMux.capForPreviousDelayCalculation);
        Farad capW = topology.bitlineCapacitance(1) + topology.storageCapacitance() + bitlineCapacitance + Farad(sub.bitlineMux.capForPreviousDelayCalculation);
        Second tauR = 2.3 * (topology.bitlinePullDownResistance(0, 0) * capR + bitlineResistance * (Farad(sub.bitlineMux.capForPreviousDelayCalculation) + bitlineCapacitance / 2.0));
        Second tauW = 2.3 * (topology.bitlinePullDownResistance(1, 0) * capW + bitlineResistance * (Farad(sub.bitlineMux.capForPreviousDelayCalculation) + bitlineCapacitance / 2.0));
        SecondExp<-1> inputRamp = SecondExp<-1>(sub.rowDecoder.rampOutput);
        Second bitlineDelayR = HorowitzCalculator::calculate(tauR, Number(0.0), inputRamp);
        Second bitlineDelayW = HorowitzCalculator::calculate(tauW, Number(0.0), inputRamp);
        Edram3tCell::Latency latency;
        latency.bitlineR = bitlineDelayR;
        latency.bitlineW = bitlineDelayW;
        latency.read = bitlineDelayR;
		double decoderLatency = std::max(sub.rowDecoder.readLatency, sub.columnDecoderLatency);
        latency.write = bitlineDelayW + Second(decoderLatency);
        latency.refresh = Second(decoderLatency + bitlineDelayW.value() + sub.senseAmp.readLatency) * sub.numRow;
        return latency;
    }

    MemoryCellLatency operator()(const Edram3t333Cell& edram3t333Cell) override {
        using namespace mse::unit;
        const Edram3t333Topology& topology = dynamic_cast<const Edram3t333Topology&>(edram3t333Cell.topology());
        SubArray sub = subarray;
        Farad bitlineCapacitance = Farad(sub.capBitline);
        Ohm bitlineResistance = Ohm(sub.resBitline);

        Farad capR = topology.bitlineCapacitance(0) + topology.storageCapacitance() + bitlineCapacitance + Farad(sub.bitlineMux.capForPreviousDelayCalculation);
        Farad capW = topology.bitlineCapacitance(1) + topology.storageCapacitance() + bitlineCapacitance + Farad(sub.bitlineMux.capForPreviousDelayCalculation);
        Second tauR = 2.3 * (topology.bitlinePullDownResistance(0, 0) * capR + bitlineResistance * (Farad(sub.bitlineMux.capForPreviousDelayCalculation) + bitlineCapacitance / 2.0));
        Second tauW = 2.3 * (topology.bitlinePullDownResistance(1, 0) * capW + bitlineResistance * (Farad(sub.bitlineMux.capForPreviousDelayCalculation) + bitlineCapacitance / 2.0));
        SecondExp<-1> inputRamp = SecondExp<-1>(sub.rowDecoder.rampOutput);
        Second bitlineDelayR = HorowitzCalculator::calculate(tauR, Number(0.0), inputRamp);
        Second bitlineDelayW = HorowitzCalculator::calculate(tauW, Number(0.0), inputRamp);
        Edram3t333Cell::Latency latency;
        latency.bitlineR = bitlineDelayR;
        latency.bitlineW = bitlineDelayW;
        latency.read = bitlineDelayR;
		double decoderLatency = std::max(sub.rowDecoder.readLatency, sub.columnDecoderLatency);
        latency.write = bitlineDelayW + Second(decoderLatency);
        latency.refresh = Second(decoderLatency + bitlineDelayW.value() + sub.senseAmp.readLatency) * sub.numRow;
        return latency;
    }
};

struct EnergyVisitor final : MemoryCellVisitor<MemoryCellEnergy, MemoryCellList> {
    using MemoryCellVisitor<MemoryCellEnergy, MemoryCellList>::operator();

    std::reference_wrapper<SubArray> subarray;

    EnergyVisitor(SubArray& subarray) : subarray(subarray) {}

    // MemoryCellEnergy operator()(const CustomCell&) override {
    //   ...
    // }

    MemoryCellEnergy operator()(const Sram6tCell& sram6tCell) override {
        using namespace mse::unit;
        const Sram6tTopology& topology = dynamic_cast<const Sram6tTopology&>(sram6tCell.topology());
        SubArray sub = subarray;
        Farad bitlineCapacitance = Farad(sub.capBitline);

        Sram6tCell::Energy energy;
        energy.read = Joule((topology.bitlineCapacitance(0).value() + bitlineCapacitance.value() + sub.bitlineMux.capForPreviousPowerCalculation) * sram6tCell.properties().prechargeVoltage.value() * sram6tCell.properties().prechargeVoltage.value() * sub.numColumn);
        energy.write = Joule((topology.bitlineCapacitance(0).value() + bitlineCapacitance.value() + sub.bitlineMux.capForPreviousPowerCalculation) * sram6tCell.properties().prechargeVoltage.value() * sram6tCell.properties().prechargeVoltage.value() * sub.numColumn / sub.muxSenseAmp / sub.muxOutputLev1 / sub.muxOutputLev2);
        return energy;
    }

    MemoryCellEnergy operator()(const DramCell& dramCell) override {
        using namespace mse::unit;
        const DramTopology& topology = dynamic_cast<const DramTopology&>(dramCell.topology());
        SubArray sub = subarray;
        Farad bitlineCapacitance = Farad(sub.capBitline);

        DramCell::Energy energy;
        energy.read = Joule((topology.bitlineCapacitance(0).value() + bitlineCapacitance.value() + sub.bitlineMux.capForPreviousPowerCalculation) * sub.senseVoltage * gTech.vdd * sub.numColumn);
		double writeVoltage = gTech.vpp; // should also equal to setVoltage, for DRAM, it is Vdd
        energy.write = Joule((bitlineCapacitance.value() + sub.bitlineMux.capForPreviousPowerCalculation) * writeVoltage * writeVoltage * sub.numColumn);
        energy.refresh = energy.read + energy.write;
        return energy;
    }

    MemoryCellEnergy operator()(const EdramCell& edramCell) override {
        using namespace mse::unit;
        const EdramTopology& topology = dynamic_cast<const EdramTopology&>(edramCell.topology());
        SubArray sub = subarray;
        Farad bitlineCapacitance = Farad(sub.capBitline);

        DramCell::Energy energy;
        energy.read = Joule((topology.bitlineCapacitance(0).value() + bitlineCapacitance.value() + sub.bitlineMux.capForPreviousPowerCalculation) * sub.senseVoltage * gTech.vdd * sub.numColumn);
		double writeVoltage = gTech.vpp; // should also equal to setVoltage, for DRAM, it is Vdd
        energy.write = Joule((bitlineCapacitance.value() + sub.bitlineMux.capForPreviousPowerCalculation) * writeVoltage * writeVoltage * sub.numColumn);
        energy.refresh = energy.read + energy.write;
        return energy;
    }

    MemoryCellEnergy operator()(const Edram3tCell& edram3tCell) override {
        using namespace mse::unit;
        const Edram3tTopology& topology = dynamic_cast<const Edram3tTopology&>(edram3tCell.topology());
        SubArray sub = subarray;
        Farad bitlineCapacitance = Farad(sub.capBitline);

        DramCell::Energy energy;
        energy.read = Joule((topology.bitlineCapacitance(0).value() + bitlineCapacitance.value() + sub.bitlineMux.capForPreviousPowerCalculation) * sub.senseVoltage * gTech.vdd * sub.numColumn);
		double writeVoltage = gTech.vpp; // should also equal to setVoltage, for DRAM, it is Vdd
        energy.write = Joule((bitlineCapacitance.value() + sub.bitlineMux.capForPreviousPowerCalculation) * writeVoltage * writeVoltage * sub.numColumn);
        energy.refresh = energy.read + energy.write;
        return energy;
    }

    MemoryCellEnergy operator()(const Edram3t333Cell& edram3t333Cell) override {
        using namespace mse::unit;
        const Edram3t333Topology& topology = dynamic_cast<const Edram3t333Topology&>(edram3t333Cell.topology());
        SubArray sub = subarray;
        Farad bitlineCapacitance = Farad(sub.capBitline);

        DramCell::Energy energy;
        energy.read = Joule((topology.bitlineCapacitance(0).value() + bitlineCapacitance.value() + sub.bitlineMux.capForPreviousPowerCalculation) * sub.senseVoltage * gTech.vdd * sub.numColumn);
		double writeVoltage = gTech.vpp; // should also equal to setVoltage, for DRAM, it is Vdd
        energy.write = Joule((bitlineCapacitance.value() + sub.bitlineMux.capForPreviousPowerCalculation) * writeVoltage * writeVoltage * sub.numColumn);
        energy.refresh = energy.read + energy.write;
        return energy;
    }
};

} // namespace mse::cell

#endif
