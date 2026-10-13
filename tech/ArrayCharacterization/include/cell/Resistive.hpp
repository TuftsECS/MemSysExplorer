// This file declares shared types for resistive memory cells (MRAM, PCRAM, RRAM, FeFET, FBRAM, ...)
// These cells store a bit as a low (on) or high (off) resistance state and are read by sensing that resistance
// Each resistive cell derives its own parameters, properties, and topology from the base types here
// The read and write models common to resistive cells are also defined here for each cell to use

#ifndef MSE_CELL_RESISTIVE_HPP
#define MSE_CELL_RESISTIVE_HPP

#include "cell/MemoryCell.hpp"
#include "input.hpp"
#include "unit.hpp"
#include "global.hpp"
#include "typedef.hpp"
#include "physics/Horowitz.hpp"
#include "constant.hpp"

#include <algorithm>
#include <cmath>

namespace mse::cell {

//==================================================
// Parameters shared by all resistive cells
// Derived parameters fill in the reset and set energy per cell with fillWriteEnergy when it isn't given

struct ResistiveCellParameters : MemoryCellParameters {
    std::reference_wrapper<const Technology> tech;

    CellAccessType accessType;
    unit::Feature accessNmosGateWidth;
    unit::Feature transistorRegionHeight;
    unit::Volt voltageDropAccessDevice;

    unit::Ohm resistanceOn;
    unit::Ohm resistanceOff;
    unit::Ohm resistanceOnAtResetVoltage;
    unit::Ohm resistanceOnAtSetVoltage;
    unit::Farad capacitanceOn;
    unit::Farad capacitanceOff;

    bool readMode; // true = voltage sensing, false = current sensing
    unit::Volt readVoltage; // 0 means current-in voltage sensing when readMode is true
    unit::Ampere readCurrent;
    unit::Watt readPower; // Read power per bitline

    bool resetMode; // true = voltage mode, false = current mode
    unit::Volt resetVoltage;
    unit::Ampere resetCurrent;
    unit::Second resetPulse;
    unit::Joule resetEnergy;

    bool setMode; // true = voltage mode, false = current mode
    unit::Volt setVoltage;
    unit::Ampere setCurrent;
    unit::Second setPulse;
    unit::Joule setEnergy;

    ResistiveCellParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech) {
        // TODO use specific cell fields in config file
        // for now just steal from gCell, figure out new input style later
        using namespace mse::unit;

        accessType = gCell.accessType;
        accessNmosGateWidth = ((gTech.featureSizeInNano <= 14) ? 2_feat : 1_feat) * gCell.widthAccessCMOS;
        transistorRegionHeight = Feature(gCell.widthInFeatureSize);
        voltageDropAccessDevice = Volt(gCell.voltageDropAccessDevice);

        resistanceOn = Ohm(gCell.resistanceOn);
        resistanceOff = Ohm(gCell.resistanceOff);
        resistanceOnAtResetVoltage = Ohm(gCell.resistanceOnAtResetVoltage);
        resistanceOnAtSetVoltage = Ohm(gCell.resistanceOnAtSetVoltage);
        capacitanceOn = Farad(gCell.capacitanceOn);
        capacitanceOff = Farad(gCell.capacitanceOff);

        readMode = gCell.readMode;
        readVoltage = Volt(gCell.readVoltage);
        readCurrent = Ampere(gCell.readCurrent);
        readPower = Watt(gCell.readPower == 0 ? calculateReadPower() : gCell.readPower);

        resetMode = gCell.resetMode;
        resetVoltage = Volt(gCell.resetVoltage);
        resetCurrent = Ampere(gCell.resetCurrent);
        resetPulse = Second(gCell.resetPulse);
        resetEnergy = Joule(gCell.resetEnergy);

        setMode = gCell.setMode;
        setVoltage = Volt(gCell.setVoltage);
        setCurrent = Ampere(gCell.setCurrent);
        setPulse = Second(gCell.setPulse);
        setEnergy = Joule(gCell.setEnergy);
    }

protected:

    // Fills in reset and set energy per cell for any not given in the cell file
    // In voltage mode the cell conducts through the given switching resistance for the whole pulse
    void fillWriteEnergy(unit::Ohm resetSwitchingResistance, unit::Ohm setSwitchingResistance) {
        if (resetEnergy == unit::Joule(0)) {
            resetEnergy = resetMode
                ? voltagePulseEnergy(resetVoltage, resetSwitchingResistance, resetPulse)
                : currentPulseEnergy(resetVoltage == unit::Volt(0), resetVoltage, resetCurrent, resetPulse);
        }
        if (setEnergy == unit::Joule(0)) {
            // Current mode checks the reset voltage to decide whether the set pulse is driven from vdd, kept from NVSim
            setEnergy = setMode
                ? voltagePulseEnergy(setVoltage, setSwitchingResistance, setPulse)
                : currentPulseEnergy(resetVoltage == unit::Volt(0), setVoltage, setCurrent, setPulse);
        }
    }

    // Energy of a current pulse driven from either vdd or the given voltage
    static unit::Joule currentPulseEnergy(bool fromVdd, unit::Volt voltage, unit::Ampere current, unit::Second pulse) {
        double drive = fromVdd ? gTech.vdd.value() : std::fabs(voltage.value());
        return unit::Joule(drive * std::fabs(current.value()) * pulse.value());
    }

private:

    unit::Joule voltagePulseEnergy(unit::Volt voltage, unit::Ohm switchingResistance, unit::Second pulse) const {
        double v = std::fabs(voltage.value());
        return unit::Joule(v * (v - voltageDropAccessDevice.value()) / switchingResistance.value() * pulse.value());
    }

    // Read power per bitline when the cell file doesn't give one
    double calculateReadPower() const {
        if (gCell.readMode) {
            if (gCell.readVoltage == 0) { // Current-in voltage sensing
                return gTech.vdd.value() * gCell.readCurrent;
            }
            if (gCell.readCurrent == 0) { // Voltage-divider sensing
                double resInSerialForSenseAmp = std::sqrt(gCell.resistanceOn * gCell.resistanceOff);
                double maxBitlineCurrent = (gCell.readVoltage - gCell.voltageDropAccessDevice) / (gCell.resistanceOn + resInSerialForSenseAmp);
                return gTech.vdd.value() * maxBitlineCurrent;
            }
            return -1.0;
        }
        double maxBitlineCurrent = (gCell.readVoltage - gCell.voltageDropAccessDevice) / gCell.resistanceOn;
        return gTech.vdd.value() * maxBitlineCurrent;
    }
};

//==================================================
// Topology base for resistive cells
// Derived topologies provide the resistance of the access device in series with the cell
// The cell resistance and read voltages seen by the sense amplifier are then derived here

class ResistiveCellTopology : public MemoryCellTopology {
public:

    // Resistance of the access path in series with the resistive element when reading
    virtual unit::Ohm accessResistance() const noexcept = 0;

    unit::Ampere leakageCurrent() const noexcept override {
        return unit::Ampere(0); // TODO cell leaks during read/write operation
    }

    // Total cell resistance in the low (on) and high (off) resistance states
    unit::Ohm resMemCellOn() const noexcept { return accessResistance() + _resistanceOn; }
    unit::Ohm resMemCellOff() const noexcept { return accessResistance() + _resistanceOff; }

    // Voltage across the cell while reading, only defined for voltage sensing
    unit::Volt voltageMemCellOn() const noexcept { return readVoltageAcross(resMemCellOn()); }
    unit::Volt voltageMemCellOff() const noexcept { return readVoltageAcross(resMemCellOff()); }

    // Cell resistance in parallel with the voltage divider resistance, only defined for voltage-in voltage sensing
    unit::Ohm resEquivalentOn() const noexcept { return equivalentResistance(resMemCellOn()); }
    unit::Ohm resEquivalentOff() const noexcept { return equivalentResistance(resMemCellOff()); }

protected:

    ResistiveCellTopology(const ResistiveCellParameters& parameters)
            : _readMode(parameters.readMode),
              _readVoltage(parameters.readVoltage),
              _readCurrent(parameters.readCurrent),
              _resistanceOn(parameters.resistanceOn),
              _resistanceOff(parameters.resistanceOff) {}

private:

    bool voltageInput() const noexcept { return _readMode && _readVoltage != unit::Volt(0); }

    // Serial resistance of voltage-in voltage sensing, forms a voltage divider with the cell
    unit::Ohm resInSerialForSenseAmp() const noexcept {
        return unit::Ohm(std::sqrt(resMemCellOn().value() * resMemCellOff().value()));
    }

    unit::Volt readVoltageAcross(unit::Ohm cellResistance) const noexcept {
        if (!_readMode) {
            return unit::Volt(0);
        }
        if (!voltageInput()) {
            return _readCurrent * cellResistance;
        }
        double serial = resInSerialForSenseAmp().value();
        return unit::Volt(_readVoltage.value() * cellResistance.value() / (cellResistance.value() + serial));
    }

    unit::Ohm equivalentResistance(unit::Ohm cellResistance) const noexcept {
        if (!voltageInput()) {
            return unit::Ohm(0);
        }
        double serial = resInSerialForSenseAmp().value();
        return unit::Ohm(cellResistance.value() * serial / (cellResistance.value() + serial));
    }

    bool _readMode;
    unit::Volt _readVoltage;
    unit::Ampere _readCurrent;
    unit::Ohm _resistanceOn;
    unit::Ohm _resistanceOff;
};

//==================================================
// Topology for resistive cells selected through an access device (MRAM, PCRAM, RRAM, FeFET, ...)
// CMOS access puts a transistor on the wordline and bitline
// Diode or no access leaves the resistive element directly on both lines
// BJT access is not modeled and adds nothing to either line

class ResistiveAccessTopology : public ResistiveCellTopology {
public:
    CellAccessType accessType;
    NmosTransistor accessTransistor;

    CellLineAdapter<CmosTransistor> accessTransistorAdapter = accessTransistor;
    CellLineAdapter<unit::Farad> accessGateAdapter;
    CellLineAdapter<unit::Farad> cellCapacitanceAdapter;

    WordlineList<1> wordlineList;
    BitlineList<1> bitlineList;

    WordlineListSpan wordlineControlledDevices() const noexcept override {
        return wordlineList;
    }

    BitlineListSpan bitlineAccessDevices() const noexcept override {
        return bitlineList;
    }

    unit::Ohm accessResistance() const noexcept override {
        using namespace mse::unit;
        return (accessType == CMOS_access) ? accessTransistor.channelResistance() : 0.0_Ohm;
    }

protected:

    // accessGateCapacitance is the wordline load of the access transistor with CMOS access
    ResistiveAccessTopology(const ResistiveCellParameters& parameters, unit::Farad accessGateCapacitance)
            : ResistiveCellTopology(parameters),
              accessType(parameters.accessType),
              accessTransistor(makeAccessTransistor(parameters)),
              accessGateAdapter(accessGateCapacitance),
              cellCapacitanceAdapter(std::max(parameters.capacitanceOn, parameters.capacitanceOff)) {
        switch (accessType) {
            case CMOS_access:
                wordlineList[0] = {accessGateAdapter};
                bitlineList[0] = {accessTransistorAdapter};
                break;
            case diode_access:
            case none_access:
                wordlineList[0] = {cellCapacitanceAdapter};
                bitlineList[0] = {cellCapacitanceAdapter};
                break;
            case BJT_access:
                break; // TODO
        }
    }

    // Uses the gate capacitance of the access transistor as the wordline load
    ResistiveAccessTopology(const ResistiveCellParameters& parameters)
            : ResistiveAccessTopology(parameters, accessTransistorGateCapacitance(parameters)) {}

    static NmosTransistor makeAccessTransistor(const ResistiveCellParameters& parameters) {
        if (parameters.accessType != CMOS_access) {
            return NmosTransistor();
        }
        return NmosTransistor(parameters.tech, parameters.accessNmosGateWidth, parameters.transistorRegionHeight);
    }

    // A default constructed transistor has no technology to calculate with, so only CMOS access has a gate load
    static unit::Farad accessTransistorGateCapacitance(const ResistiveCellParameters& parameters) {
        if (parameters.accessType != CMOS_access) {
            return unit::Farad();
        }

        return makeAccessTransistor(parameters).gateCapacitance();
    }
};

//==================================================
// Properties shared by resistive cells
// With voltage sensing, the bitline is precharged between the voltages of the on and off states

struct ResistiveCellProperties : MemoryCellProperties {
    ResistiveCellProperties(const ResistiveCellParameters& parameters, const ResistiveCellTopology& topology) {
        using namespace mse::unit;

        sensingMethod = parameters.readMode ? SenseMethod::Voltage : SenseMethod::Current;
        sensingVoltage = Volt(gCell.minSenseVoltage);
        if (parameters.readMode) {
            prechargeVoltage = std::min(gTech.vdd, (topology.voltageMemCellOff() + topology.voltageMemCellOn()) / 2.0);
        }

        sharedContact = (parameters.accessType == CMOS_access);
        isMLC = false;
        columnLeak = false;
    }
};

//==================================================
// Read and write models used by resistive cells
// Each takes the pieces of a cell it needs so any resistive cell can use them

struct ResistiveBitlineDelay {
    unit::Second bitline;
    unit::Second bitlineOn; // Only calculated for voltage sensing
    unit::Second bitlineOff; // Only calculated for voltage sensing
};

inline ResistiveBitlineDelay resistiveBitlineDelay(
        const ResistiveCellParameters& parameters,
        const ResistiveCellTopology& topology,
        const MemoryCellProperties& properties,
        const MemoryCellLatencyContext& context) {
    using namespace mse::unit;

    ResistiveBitlineDelay delay;

    double resBitline = context.bitlineResistance.value();
    double capBitline = context.bitlineCapacitance.value();

    if (!parameters.readMode) { // Current sensing
        // Use ICCAD 2009 model
        double resMemCellOff = topology.resMemCellOff().value();
        Second tau(resBitline * capBitline / 2 * (resMemCellOff + resBitline / 3) / (resMemCellOff + resBitline));
        delay.bitline = physics::horowitz(tau, 0.0, context.rowDecoderRampOutput).delay;
        return delay;
    }

    // Voltage sensing, current-in uses the cell resistance directly and voltage-in uses the voltage divider
    bool currentInput = (parameters.readVoltage == Volt(0));
    double resOn = (currentInput ? topology.resMemCellOn() : topology.resEquivalentOn()).value();
    double resOff = (currentInput ? topology.resMemCellOff() : topology.resEquivalentOff()).value();

    double capCellAccess = topology.bitlineCapacitance(0).value();
    double capMux = context.bitlineMuxCapacitance.value();
    double voltagePrecharge = properties.prechargeVoltage.value();
    double voltageMemCellOn = topology.voltageMemCellOn().value();
    double voltageMemCellOff = topology.voltageMemCellOff().value();
    double senseVoltage = context.senseVoltage.value();

    double tau = resOn * (capCellAccess + capBitline + capMux) + resBitline * (capMux + capBitline / 2); // Time constant of LRS
    delay.bitlineOn = Second(tau * std::log((voltagePrecharge - voltageMemCellOn) / (voltagePrecharge - voltageMemCellOn - senseVoltage)));
    tau = resOff * (capCellAccess + capBitline + capMux) + resBitline * (capMux + capBitline / 2); // Time constant of HRS
    delay.bitlineOff = Second(tau * std::log((voltageMemCellOff - voltagePrecharge) / (voltageMemCellOff - voltagePrecharge - senseVoltage)));
    delay.bitline = std::max(delay.bitlineOn, delay.bitlineOff);

    return delay;
}

struct ResistiveWriteLatency {
    unit::Second write;
    unit::Second set;
    unit::Second reset;
};

// Write through a selected cell, where the longer of the set and reset pulses follows bitline charging
inline ResistiveWriteLatency pulsedWriteLatency(const ResistiveCellParameters& parameters, const MemoryCellLatencyContext& context) {
    ResistiveWriteLatency latency;
    latency.write = std::max(context.rowDecoderWriteLatency, context.columnDecoderLatency + context.chargeLatency);
    latency.reset = latency.write + parameters.resetPulse;
    latency.set = latency.write + parameters.setPulse;
    latency.write += std::max(parameters.resetPulse, parameters.setPulse);
    return latency;
}

// Write in a crosspoint array without access transistors, where both set and reset pulses are applied
inline ResistiveWriteLatency crosspointWriteLatency(const ResistiveCellParameters& parameters, const MemoryCellLatencyContext& context) {
    ResistiveWriteLatency latency;
    if (gInputParameter.writeScheme == erase_before_reset || gInputParameter.writeScheme == erase_before_set) {
        latency.write = std::max(context.rowDecoderWriteLatency, context.chargeLatency);
    } else {
        latency.write = std::max(context.rowDecoderWriteLatency, context.columnDecoderLatency + context.chargeLatency);
    }
    latency.write += context.chargeLatency;
    latency.write += parameters.resetPulse + parameters.setPulse;
    return latency;
}

// Write latency of cells selected through an access device, crosspoint arrays apply both pulses
inline ResistiveWriteLatency accessDeviceWriteLatency(const ResistiveCellParameters& parameters, const MemoryCellLatencyContext& context) {
    if (parameters.accessType == diode_access || parameters.accessType == none_access) {
        return crosspointWriteLatency(parameters, context);
    }
    return pulsedWriteLatency(parameters, context);
}

// Bitline energy of reading every column
inline unit::Joule resistiveReadDynamicEnergy(
        const ResistiveCellParameters& parameters,
        const ResistiveCellTopology& topology,
        const MemoryCellProperties& properties,
        const MemoryCellEnergyContext& context) {
    double capCellAccess = topology.bitlineCapacitance(0).value();
    double capBitline = context.bitlineCapacitance.value();
    double capMux = context.bitlineMuxPreviousPowerCapacitance.value();

    if (!parameters.readMode) { // Current sensing
        // Use ICCAD 2009 model
        double resBitlineMux = context.bitlineMuxResistance.value();
        double resBitline = context.bitlineResistance.value();
        double resMemCellOn = topology.resMemCellOn().value();
        double readVoltage = parameters.readVoltage.value();
        double vpreMin = readVoltage * resBitlineMux / (resBitlineMux + resBitline + resMemCellOn);
        double vpreMax = readVoltage * (resBitlineMux + resBitline) / (resBitlineMux + resBitline + resMemCellOn);
        double energy = capCellAccess * vpreMax * vpreMax + capMux * vpreMin * vpreMin
                        + capBitline * (vpreMax * vpreMax + vpreMin * vpreMin + vpreMax * vpreMin) / 3;
        return unit::Joule(energy * context.numColumn);
    }

    double voltagePrecharge = properties.prechargeVoltage.value();
    double voltageMemCellOn = topology.voltageMemCellOn().value();
    return unit::Joule((capCellAccess + capBitline + capMux) * (voltagePrecharge * voltagePrecharge - voltageMemCellOn * voltageMemCellOn) * context.numColumn);
}

// Energy of the cells being read, each read also passes current through a reference cell
inline unit::Joule resistiveCellReadEnergy(const ResistiveCellParameters& parameters, const MemoryCellEnergyContext& context) {
    unit::Joule energy(2 * parameters.readPower.value() * context.senseAmpReadLatency.value());
    return energy * (context.numColumn / context.muxSenseAmp / context.muxOutputLev1 / context.muxOutputLev2);
}

struct ResistiveWriteEnergy {
    unit::Joule write;
    unit::Joule set;
    unit::Joule reset;
};

// Energy to reset or set every accessed cell, the dynamic transition during switching is ignored
// The cell resistance is assumed to stay high for worst-case power estimation
inline ResistiveWriteEnergy resistiveCellWriteEnergy(
        const ResistiveCellParameters& parameters,
        const ResistiveCellTopology& topology,
        const MemoryCellEnergyContext& context) {
    double capLine = topology.bitlineCapacitance(0).value() + context.bitlineCapacitance.value() + context.bitlineMuxPreviousPowerCapacitance.value();
    double resetVoltage = parameters.resetMode ? parameters.resetVoltage.value() : gTech.vdd.value();
    double setVoltage = parameters.setMode ? parameters.setVoltage.value() : gTech.vdd.value();

    unit::Joule resetEnergyPerBit = parameters.resetEnergy + unit::Joule(capLine * resetVoltage * resetVoltage);
    unit::Joule setEnergyPerBit = parameters.setEnergy + unit::Joule(capLine * setVoltage * setVoltage);

    ResistiveWriteEnergy energy;
    energy.reset = resetEnergyPerBit * context.numColumn / context.muxSenseAmp / context.muxOutputLev1 / context.muxOutputLev2;
    energy.set = setEnergyPerBit * context.numColumn / context.muxSenseAmp / context.muxOutputLev1 / context.muxOutputLev2;
    return energy;
}

// Write energy of cells selected through an access device
// Crosspoint arrays that erase before writing apply both the set and reset pulses
inline ResistiveWriteEnergy accessDeviceWriteEnergy(
        const ResistiveCellParameters& parameters,
        const ResistiveCellTopology& topology,
        const MemoryCellEnergyContext& context) {
    ResistiveWriteEnergy energy = resistiveCellWriteEnergy(parameters, topology, context);

    bool crosspoint = parameters.accessType == diode_access || parameters.accessType == none_access;
    bool eraseFirst = gInputParameter.writeScheme == erase_before_reset || gInputParameter.writeScheme == erase_before_set;
    if (crosspoint && eraseFirst) {
        energy.write = energy.reset + energy.set; // TODO write pattern is not considered
    } else {
        energy.write = std::max(energy.reset, energy.set);
    }

    // Due to the shaper inefficiency
    energy.reset /= constant::shaperEfficiencyAggressive;
    energy.set /= constant::shaperEfficiencyAggressive;
    energy.write /= constant::shaperEfficiencyAggressive;

    return energy;
}

} // namespace mse::cell

#endif
