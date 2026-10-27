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

    ScalarParameter<unit::Feature> accessNmosGateWidth{"Access.Width", "", unit::Feature(0)};
    ScalarParameter<unit::Volt> voltageDropAccessDevice{"Access.VoltageDrop", "", unit::Volt(0)};
    ScalarParameter<unit::Ampere> leakageCurrentAccessDevice{"Access.LeakageCurrent", "", unit::Ampere(0)}; // Reverse current of the access device
    unit::Feature transistorRegionHeight;

    ScalarParameter<unit::Ohm> resistanceOn{"Resistance.On", "", unit::Ohm(0)}; // At the read voltage
    ScalarParameter<unit::Ohm> resistanceOff{"Resistance.Off", "", unit::Ohm(0)}; // At the read voltage
    ScalarParameter<unit::Ohm> resistanceOnAtResetVoltage{"Resistance.OnAtResetVoltage", "", unit::Ohm(0)};
    ScalarParameter<unit::Ohm> resistanceOnAtSetVoltage{"Resistance.OnAtSetVoltage", "", unit::Ohm(0)};
    ScalarParameter<unit::Ohm> resistanceOnAtHalfResetVoltage{"Resistance.OnAtHalfResetVoltage", "", unit::Ohm(0)};
    ScalarParameter<unit::Farad> capacitanceOn{"Capacitance.On", "", unit::Farad(0)};
    ScalarParameter<unit::Farad> capacitanceOff{"Capacitance.Off", "", unit::Farad(0)};

    ScalarParameter<std::string> readModeName{"Read.Mode", "", std::string("voltage")};
    ScalarParameter<unit::Volt> readVoltage{"Read.Voltage", "", unit::Volt(0)}; // 0 means current-in voltage sensing
    ScalarParameter<unit::Ampere> readCurrent{"Read.Current", "", unit::Ampere(0)};
    ScalarParameter<unit::Watt> readPower{"Read.Power", "", unit::Watt(0)}; // Read power per bitline, calculated when not given
    bool readMode; // true = voltage sensing, false = current sensing

    ScalarParameter<std::string> resetModeName{"Reset.Mode", "", std::string("voltage")};
    ScalarParameter<unit::Volt> resetVoltage{"Reset.Voltage", "", unit::Volt(0)};
    ScalarParameter<unit::Ampere> resetCurrent{"Reset.Current", "", unit::Ampere(0)};
    ScalarParameter<unit::Second> resetPulse{"Reset.Pulse", "", unit::Second(0)};
    ScalarParameter<unit::Joule> resetEnergy{"Reset.Energy", "", unit::Joule(0)}; // Calculated when not given
    bool resetMode; // true = voltage mode, false = current mode

    ScalarParameter<std::string> setModeName{"Set.Mode", "", std::string("voltage")};
    ScalarParameter<unit::Volt> setVoltage{"Set.Voltage", "", unit::Volt(0)};
    ScalarParameter<unit::Ampere> setCurrent{"Set.Current", "", unit::Ampere(0)};
    ScalarParameter<unit::Second> setPulse{"Set.Pulse", "", unit::Second(0)};
    ScalarParameter<unit::Joule> setEnergy{"Set.Energy", "", unit::Joule(0)}; // Calculated when not given
    bool setMode; // true = voltage mode, false = current mode

    ResistiveCellParameters(YamlInputFile& file) : MemoryCellParameters(file), tech(gTech) {
        fillParameter(accessNmosGateWidth, voltageDropAccessDevice, leakageCurrentAccessDevice);
        fillParameter(resistanceOn, resistanceOff, resistanceOnAtResetVoltage, resistanceOnAtSetVoltage, resistanceOnAtHalfResetVoltage, capacitanceOn, capacitanceOff);
        fillParameter(readModeName, readVoltage, readCurrent, readPower);
        fillParameter(resetModeName, resetVoltage, resetCurrent, resetPulse, resetEnergy);
        fillParameter(setModeName, setVoltage, setCurrent, setPulse, setEnergy);

        unit::Number featureScale = (tech.get().featureSizeInNano <= 14 ? 2 : 1);
        accessNmosGateWidth.value() *= featureScale;
        transistorRegionHeight = cellWidth;

        readMode = readModeName.value() == "voltage";
        resetMode = resetModeName.value() == "voltage";
        setMode = setModeName.value() == "voltage";

        if (readPower.value() == unit::Watt(0)) {
            readPower.value() = unit::Watt(calculateReadPower());
        }
    }

protected:

    // Fills in reset and set energy per cell for any not given in the cell file
    // In voltage mode the cell conducts through the given switching resistance for the whole pulse
    void fillWriteEnergy(unit::Ohm resetSwitchingResistance, unit::Ohm setSwitchingResistance) {
        if (resetEnergy.value() == unit::Joule(0)) {
            resetEnergy.value() = resetMode
                ? voltagePulseEnergy(resetVoltage, resetSwitchingResistance, resetPulse)
                : currentPulseEnergy(resetVoltage.value() == unit::Volt(0), resetVoltage, resetCurrent, resetPulse);
        }
        if (setEnergy.value() == unit::Joule(0)) {
            // Current mode checks the reset voltage to decide whether the set pulse is driven from vdd, kept from NVSim
            setEnergy.value() = setMode
                ? voltagePulseEnergy(setVoltage, setSwitchingResistance, setPulse)
                : currentPulseEnergy(resetVoltage.value() == unit::Volt(0), setVoltage, setCurrent, setPulse);
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
        return unit::Joule(v * (v - voltageDropAccessDevice.value().value()) / switchingResistance.value() * pulse.value());
    }

    // Read power per bitline when the cell file doesn't give one
    double calculateReadPower() const {
        const double vdd = tech.get().vdd.value();
        const double on = resistanceOn.value().value();
        const double off = resistanceOff.value().value();
        const double voltage = readVoltage.value().value();
        const double current = readCurrent.value().value();
        const double drop = voltageDropAccessDevice.value().value();

        if (readMode) {
            if (voltage == 0) { // Current-in voltage sensing
                return vdd * current;
            }
            if (current == 0) { // Voltage-divider sensing
                double resInSerialForSenseAmp = std::sqrt(on * off);
                double maxBitlineCurrent = (voltage - drop) / (on + resInSerialForSenseAmp);

                return vdd * maxBitlineCurrent;
            }

            return -1.0;
        }
        double maxBitlineCurrent = (voltage - drop) / on;

        return vdd * maxBitlineCurrent;
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
              cellCapacitanceAdapter(std::max(parameters.capacitanceOn.value(), parameters.capacitanceOff.value())) {
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
                break; // TODO no BJT access device model, so the cell adds nothing to its wordline and bitline loads
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
    ResistiveCellProperties(const ResistiveCellParameters& parameters, const ResistiveCellTopology& topology) : MemoryCellProperties(parameters) {
        using namespace mse::unit;
        operations = {Operation::Read, Operation::Write, Operation::Set, Operation::Reset};

        sensingMethod = parameters.readMode ? SenseMethod::Voltage : SenseMethod::Current;
        if (parameters.readMode) {
            prechargeVoltage = std::min(gTech.vdd, (topology.voltageMemCellOff() + topology.voltageMemCellOn()) / 2.0);
        }

        sharedContact = (parameters.accessType == CMOS_access);
        columnLeak = false;

        maxMuxResistance = Ohm(parameters.resistanceOn.value().value() * constant::irDropTolerance);
        cellEnergyOperations = {Operation::Read, Operation::Set, Operation::Reset};

        globalBitline.model = GlobalBitlineModel::Resistive;
        globalBitline.readVoltage = parameters.readVoltage;
        globalBitline.writeVoltage = Volt(std::max(std::fabs(parameters.resetVoltage.value().value()), std::fabs(parameters.setVoltage.value().value())));
        globalBitline.resistanceOn = parameters.resistanceOn;
        globalBitline.resistanceOff = parameters.resistanceOff;
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
    bool currentInput = (parameters.readVoltage.value() == Volt(0));
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
    latency.reset = latency.write + parameters.resetPulse.value();
    latency.set = latency.write + parameters.setPulse.value();
    latency.write += std::max(parameters.resetPulse.value(), parameters.setPulse.value());
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
    latency.write += parameters.resetPulse.value() + parameters.setPulse.value();
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
        double readVoltage = parameters.readVoltage.value().value();
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
    unit::Joule energy(2 * parameters.readPower.value().value() * context.senseAmpReadLatency.value());
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
    double resetVoltage = parameters.resetMode ? parameters.resetVoltage.value().value() : gTech.vdd.value();
    double setVoltage = parameters.setMode ? parameters.setVoltage.value().value() : gTech.vdd.value();

    unit::Joule resetEnergyPerBit = parameters.resetEnergy.value() + unit::Joule(capLine * resetVoltage * resetVoltage);
    unit::Joule setEnergyPerBit = parameters.setEnergy.value() + unit::Joule(capLine * setVoltage * setVoltage);

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

//==================================================
// Base for resistive cell types, holding the cell's parameters, topology and properties
// Reads and writes go through the access device models above
// Cells with their own write models override timing and power

template <typename ParametersType, typename TopologyType, typename PropertiesType>
class ResistiveMemoryCell : public MemoryCell {
protected:

    ParametersType _parameters;
    TopologyType _topology;
    PropertiesType _properties;

public:

    ResistiveMemoryCell(const ParametersType& parameters) : _parameters(parameters), _topology(parameters), _properties(parameters, _topology) {}

    const MemoryCellTopology& topology() const noexcept override { return _topology; }
    const MemoryCellProperties& properties() const noexcept override { return _properties; }
    const MemoryCellParameters& parameters() const noexcept override { return _parameters; }

    CellTiming timing(const MemoryCellLatencyContext& context) const noexcept override {
        ResistiveBitlineDelay bitlineDelay = resistiveBitlineDelay(_parameters, _topology, _properties, context);
        ResistiveWriteLatency writeLatency = accessDeviceWriteLatency(_parameters, context);

        CellTiming timing;

        timing.bitline = bitlineDelay.bitline;
        timing.bitlineOn = bitlineDelay.bitlineOn;
        timing.bitlineOff = bitlineDelay.bitlineOff;
        timing.latency[Operation::Read] = bitlineDelay.bitline + context.bitlineMuxReadLatency;
        timing.latency[Operation::Write] = writeLatency.write;
        timing.latency[Operation::Set] = writeLatency.set;
        timing.latency[Operation::Reset] = writeLatency.reset;

        return timing;
    }

    CellPower power(const MemoryCellEnergyContext& context) const noexcept override {
        ResistiveWriteEnergy writeEnergy = accessDeviceWriteEnergy(_parameters, _topology, context);

        CellPower power;

        power.dynamicEnergy[Operation::Read] = resistiveReadDynamicEnergy(_parameters, _topology, _properties, context);
        power.cellEnergy[Operation::Read] = resistiveCellReadEnergy(_parameters, context);
        power.dynamicEnergy[Operation::Write] = writeEnergy.write;
        power.cellEnergy[Operation::Set] = writeEnergy.set;
        power.cellEnergy[Operation::Reset] = writeEnergy.reset;

        return power;
    }

    std::optional<ArrayCurrents> arrayCurrents(long long rows, long long columns, int selectedColumns) const noexcept override {
        using namespace mse::unit;

        const int temperatureIndex = gInputParameter.temperature - 300;
        const double resetCurrent = _parameters.resetCurrent.value().value();
        const double setCurrent = _parameters.setCurrent.value().value();
        const double leakageCurrent = _parameters.leakageCurrentAccessDevice.value().value();
        const double resistanceOnAtResetVoltage = _parameters.resistanceOnAtResetVoltage.value().value();
        const double resistanceOnAtHalfResetVoltage = _parameters.resistanceOnAtHalfResetVoltage.value().value();

        ArrayCurrents currents;

        if (_parameters.accessType == CMOS_access) {
            // Bitline too long
            if ((gTech.currentOnNmos[temperatureIndex] / gTech.currentOffNmos[temperatureIndex]).value() < rows / constant::bitlineLeakageTolerance) {
                return std::nullopt;
            }
            currents.maxBitlineCurrent = Ampere(std::max(resetCurrent, setCurrent) + leakageCurrent * (rows - 1));

            return currents;
        }

        // Write half select limits the array size
        Ampere switchingCurrent(resetCurrent);
        if (resetCurrent == 0) {
            switchingCurrent = Ampere((std::fabs(_parameters.resetVoltage.value().value()) - _parameters.voltageDropAccessDevice.value().value())
                    / resistanceOnAtResetVoltage);
        }

        if (_parameters.accessType == none_access) {
            currents.maxWordlineCurrent = switchingCurrent * selectedColumns + switchingCurrent * resistanceOnAtResetVoltage
                    / 2 / resistanceOnAtHalfResetVoltage * (columns - selectedColumns);
            currents.maxBitlineCurrent = switchingCurrent + switchingCurrent * resistanceOnAtResetVoltage / 2
                    / resistanceOnAtHalfResetVoltage * (rows - 1);
        } else { // Diode or BJT
            currents.maxWordlineCurrent = switchingCurrent * selectedColumns + Ampere(leakageCurrent) * (columns - selectedColumns);
            currents.maxBitlineCurrent = switchingCurrent + Ampere(leakageCurrent) * (rows - 1);
        }

        return currents;
    }

    void printSummary(std::ostream& stream) const override {
        using namespace mse::unit;

        const double resistanceOn = _parameters.resistanceOn.value().value();
        const double resistanceOff = _parameters.resistanceOff.value().value();
        const double readCurrent = _parameters.readCurrent.value().value();
        const double readVoltage = _parameters.readVoltage.value().value();

        if (resistanceOn < 1e3) {
            stream << "Cell Turned-On Resistance : " << resistanceOn << "ohm\n";
        } else if (resistanceOn < 1e6) {
            stream << "Cell Turned-On Resistance : " << resistanceOn / 1e3 << "Kohm\n";
        } else {
            stream << "Cell Turned-On Resistance : " << resistanceOn / 1e6 << "Mohm\n";
        }
        if (resistanceOff < 1e3) {
            stream << "Cell Turned-Off Resistance: " << resistanceOff << "ohm\n";
        } else if (resistanceOff < 1e6) {
            stream << "Cell Turned-Off Resistance: " << resistanceOff / 1e3 << "Kohm\n";
        } else {
            stream << "Cell Turned-Off Resistance: " << resistanceOff / 1e6 << "Mohm\n";
        }

        stream << (_parameters.readMode ? "Read Mode: Voltage-Sensing\n" : "Read Mode: Current-Sensing\n");
        if (readCurrent > 0) {
            stream << "  - Read Current: " << readCurrent * 1e6 << "uA\n";
        }
        if (readVoltage > 0) {
            stream << "  - Read Voltage: " << readVoltage << "V\n";
        }

        if (_parameters.resetMode) {
            stream << "Reset Mode: Voltage\n";
            stream << "  - Reset Voltage: " << _parameters.resetVoltage.value().value() << "V\n";
        } else {
            stream << "Reset Mode: Current\n";
            stream << "  - Reset Current: " << _parameters.resetCurrent.value().value() * 1e6 << "uA\n";
        }
        stream << "  - Reset Pulse: " << _parameters.resetPulse.value() << "\n";

        if (_parameters.setMode) {
            stream << "Set Mode: Voltage\n";
            stream << "  - Set Voltage: " << _parameters.setVoltage.value().value() << "V\n";
        } else {
            stream << "Set Mode: Current\n";
            stream << "  - Set Current: " << _parameters.setCurrent.value().value() * 1e6 << "uA\n";
        }
        stream << "  - Set Pulse: " << _parameters.setPulse.value() << "\n";

        stream << "Access Type: " << _parameters.accessType.value() << "\n";
    }

    void writeSummary(YAML::Node node, [[maybe_unused]] bool cacheResult) const override {
        const double resistanceOn = _parameters.resistanceOn.value().value();
        const double resistanceOff = _parameters.resistanceOff.value().value();
        const double readCurrent = _parameters.readCurrent.value().value();
        const double readVoltage = _parameters.readVoltage.value().value();

        if (resistanceOn < 1e3) {
            node["R_on_Ohm"] = resistanceOn;
        } else if (resistanceOn < 1e6) {
            node["R_on_KOhm"] = resistanceOn / 1e3;
        } else {
            node["R_on_MOhm"] = resistanceOn / 1e6;
        }
        if (resistanceOff < 1e3) {
            node["R_off_Ohm"] = resistanceOff;
        } else if (resistanceOff < 1e6) {
            node["R_off_KOhm"] = resistanceOff / 1e3;
        } else {
            node["R_off_MOhm"] = resistanceOff / 1e6;
        }

        node["ReadMode"] = _parameters.readMode ? "Voltage-Sensing" : "Current-Sensing";
        if (readCurrent > 0) {
            node["ReadCurrent_uA"] = readCurrent * 1e6;
        }
        if (readVoltage > 0) {
            node["ReadVoltage_V"] = readVoltage;
        }

        node["ResetMode"] = _parameters.resetMode ? "Voltage" : "Current";
        node["ResetVoltage_V"] = _parameters.resetVoltage.value().value();
        node["ResetCurrent_uA"] = _parameters.resetCurrent.value().value() * 1e6;
        node["ResetPulse_s"] = _parameters.resetPulse.value().value() / 1e9;

        node["SetMode"] = _parameters.setMode ? "Voltage" : "Current";
        node["SetVoltage_V"] = _parameters.setVoltage.value().value();
        node["SetCurrent_uA"] = _parameters.setCurrent.value().value() * 1e6;
        node["SetPulse_s"] = _parameters.setPulse.value().value() / 1e9;

        node["AccessType"] = enumToString(_parameters.accessType.value());
    }
};

} // namespace mse::cell

#endif
