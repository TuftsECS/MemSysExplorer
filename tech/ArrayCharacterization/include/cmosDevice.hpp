#ifndef MSE_CMOS_HPP
#define MSE_CMOS_HPP

#include "unit.hpp"
#include "Dimensions.hpp"

#include <functional>

class Technology;

namespace mse {

enum class GateType {
    Inverter,
    Nand,
    Nor
};

//==================================================
// CMOS device abstract base that unifies an interface for all CMOS devices (transistors and gates)

class CmosDevice {
public:

    virtual ~CmosDevice() = default;

    virtual constexpr int inputCount() const noexcept = 0;

    Dimensions dimensions() const noexcept { return _dimensions; }
    unit::Farad drainCapacitance() const noexcept { return _drainCapacitance; }
    unit::Farad gateCapacitance() const noexcept { return _gateCapacitance; }
    unit::Ampere leakageCurrent() const noexcept { return _leakageCurrent; }

protected:

    Dimensions _dimensions;
    unit::Farad _drainCapacitance;
    unit::Farad _gateCapacitance;
    unit::Ampere _leakageCurrent;
};

//==================================================
// CMOS tranistor model base class that computes all values once during construction

class CmosTransistor : public CmosDevice {
public:

    constexpr int inputCount() const noexcept override final { return 1; }
    unit::Ohm channelResistance() const noexcept { return _channelResistance; }
    unit::Siemens transconductance() const noexcept { return _transconductance; }

protected:

    constexpr CmosTransistor() = default;
    CmosTransistor(
        const Technology& tech,
        unit::Feature gateWidth,
        unit::Feature transistorRegionHeight,
        const double* onCurrent, // TODO change to unit::Ampere
        const double* offCurrent, // TODO change to unit::Ampere
        unit::Volt vdsat,
        unit::Number mobility
    );

private:

    unit::Ohm _channelResistance;
    unit::Siemens _transconductance;
};

//==================================================
// NMOS variant of a CMOS transistor
// Provides NMOS specific values for use in the generic CMOS transistor functions

class NmosTransistor final : public CmosTransistor {
public:

    constexpr NmosTransistor() = default;
    NmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight);
};

//==================================================
// PMOS variant of a CMOS transistor
// Provides PMOS specific values for use in the generic CMOS transistor functions

class PmosTransistor final : public CmosTransistor {
public:

    constexpr PmosTransistor() = default;
    PmosTransistor(const Technology& tech, unit::Feature gateWidth, unit::Feature transistorRegionHeight);
};

//==================================================
// CMOS gate model that computes all values once during construction
// When inputs is > 0, that value is used as a compile-time constant for a number of inputs
// When inputs is = 0, a runtime value is supplied to the constructor for the number of inputs

template <GateType Gate, int Inputs>
class CmosGate : public CmosDevice {
public:

    constexpr CmosGate() = default;
    CmosGate(const Technology& tech, unit::Feature unitNmosGateWidth, unit::Feature unitPmosGateWidth, unit::Feature gateRegionHeight) requires (Inputs > 0);
    CmosGate(const Technology& tech, int inputs, unit::Feature unitNmosGateWidth, unit::Feature unitPmosGateWidth, unit::Feature gateRegionHeight);

    constexpr int inputCount() const noexcept override final { return _inputs; }
    unit::Ohm nmosResistance() const noexcept { return _nmosResistance; }
    unit::Ohm pmosResistance() const noexcept { return _pmosResistance; }
    unit::Number logicalEffort() const noexcept { return _logicalEffort; }
    // Effective widths
    unit::Feature nmosWidth() const noexcept { return _nmosWidth; }
    unit::Feature pmosWidth() const noexcept { return _pmosWidth; }

private:

    int _inputs = 0;
    unit::Feature _nmosWidth;
    unit::Feature _pmosWidth;
    unit::Ohm _nmosResistance;
    unit::Ohm _pmosResistance;
    unit::Number _logicalEffort;
};

//==================================================
// Aliases for GateType selections

using CmosInverter = CmosGate<GateType::Inverter, 1>;

template <int Inputs = 0>
using CmosNand = CmosGate<GateType::Nand, Inputs>;

template <int Inputs = 0>
using CmosNor = CmosGate<GateType::Nor, Inputs>;

} // namespace mse

#endif
