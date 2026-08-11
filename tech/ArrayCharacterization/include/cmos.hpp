#ifndef MSE_CMOS_HPP
#define MSE_CMOS_HPP

#include "unit.hpp"

#include <functional>

class Technology;

namespace mse {

//==================================================
// CMOS device abstract base that unifies an interface for all CMOS devices (transistors and gates)

class CmosDevice {
public:

    virtual ~CmosDevice() = default;

    virtual int inputCount() const noexcept = 0;

    virtual unit::Farad drainCapacitance() const noexcept = 0;
    virtual unit::Farad gateCapacitance() const noexcept = 0;
    unit::Farad totalGateCapacitance() const noexcept {
        return gateCapacitance() * inputCount();
    }

    virtual unit::Ampere leakageCurrent() const noexcept = 0;

    virtual unit::Meter width() const noexcept = 0;
    virtual unit::Meter height() const noexcept = 0;
};

//==================================================
// CMOS tranistor model base class that computes all values once during construction

class CmosTransistor : public CmosDevice {
public:

    int inputCount() const noexcept override final { return 1; }
    unit::Farad drainCapacitance() const noexcept override { return _drainCapacitance; }
    unit::Farad gateCapacitance() const noexcept override { return _gateCapacitance; }
    unit::Ohm channelResistance() const noexcept { return _channelResistance; }
    unit::Siemens transconductance() const noexcept { return _transconductance; }
    unit::Ampere leakageCurrent() const noexcept override { return _leakageCurrent; }
    unit::Meter width() const noexcept override { return _width; }
    unit::Meter height() const noexcept override { return _height; }

protected:

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

    unit::Farad _drainCapacitance;
    unit::Farad _gateCapacitance;
    unit::Ohm _channelResistance;
    unit::Siemens _transconductance;
    unit::Ampere _leakageCurrent;
    unit::Meter _width;
    unit::Meter _height;
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
// CMOS gate model base class that computes all values once during construction
// Templated to do CRTP and take a certain number of gate inputs so logic may be optimized at compile time

template <typename Derived, int Inputs>
class CmosGate : public CmosDevice {
public:

    int inputCount() const noexcept override final { return Inputs; }
    unit::Farad drainCapacitance() const noexcept override { return _drainCapacitance; }
    unit::Farad gateCapacitance() const noexcept override { return _gateCapacitance; }
    unit::Ohm nmosResistance() const noexcept { return _nmosResistance; }
    unit::Ohm pmosResistance() const noexcept { return _pmosResistance; }
    unit::Ampere leakageCurrent() const noexcept override { return _leakageCurrent; }
    unit::Meter width() const noexcept override { return _width; }
    unit::Meter height() const noexcept override { return _height; }

protected:

    CmosGate(const Technology& tech, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight);

    unit::Farad _drainCapacitance;
    unit::Farad _gateCapacitance;
    unit::Ohm _nmosResistance;
    unit::Ohm _pmosResistance;
    unit::Ampere _leakageCurrent;
    unit::Meter _width;
    unit::Meter _height;
};

// Specialization for CMOS gates where the number of gate inputs must be given at runtime
// Choose to specialize to when Inputs = 0 because that would not be a practical value to use for a real gate
// This specialization changes the constructor to take an extra int for the number of inputs

template <typename Derived>
class CmosGate<Derived, 0> : public CmosDevice {
public:

    int inputCount() const noexcept override final { return _inputs; }
    unit::Farad drainCapacitance() const noexcept override { return _drainCapacitance; }
    unit::Farad gateCapacitance() const noexcept override { return _gateCapacitance; }
    unit::Ohm nmosResistance() const noexcept { return _nmosResistance; }
    unit::Ohm pmosResistance() const noexcept { return _pmosResistance; }
    unit::Ampere leakageCurrent() const noexcept override { return _leakageCurrent; }
    unit::Meter width() const noexcept override { return _width; }
    unit::Meter height() const noexcept override { return _height; }

protected:

    constexpr CmosGate() = default;
    CmosGate(const Technology& tech, int inputs, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight);

    const int _inputs;
    unit::Farad _drainCapacitance;
    unit::Farad _gateCapacitance;
    unit::Ohm _nmosResistance;
    unit::Ohm _pmosResistance;
    unit::Ampere _leakageCurrent;
    unit::Meter _width;
    unit::Meter _height;
};

//==================================================
// INV variant of a CMOS gate

class CmosInverter final : public CmosGate<CmosInverter, 1> {

    friend class CmosGate<CmosInverter, 1>;

public:

    constexpr CmosInverter() = default;
    CmosInverter(const Technology& tech, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight);

private:

    static constexpr bool sequentialNmos = true;
};

//==================================================
// NAND variant of a CMOS gate

template <int Inputs = 0>
class CmosNand final : public CmosGate<CmosNand<Inputs>, Inputs> {

    friend class CmosGate<CmosNand<Inputs>, Inputs>;

public:

    constexpr CmosNand() = default;
    CmosNand(const Technology& tech, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight);

private:

    static constexpr bool sequentialNmos = true;
};

template <>
class CmosNand<0> final : public CmosGate<CmosNand<0>, 0> {

    friend class CmosGate<CmosNand<0>, 0>;

public:

    constexpr CmosNand() = default;
    CmosNand(const Technology& tech, int inputs, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight);

private:

    static constexpr bool sequentialNmos = true;
};

//==================================================
// NOR variant of a CMOS gate

template <int Inputs = 0>
class CmosNor final : public CmosGate<CmosNor<Inputs>, Inputs> {

    friend class CmosGate<CmosNor<Inputs>, Inputs>;

public:

    constexpr CmosNor() = default;
    CmosNor(const Technology& tech, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight);

private:

    static constexpr bool sequentialNmos = false;
};

template <>
class CmosNor<0> final : public CmosGate<CmosNor<0>, 0> {

    friend class CmosGate<CmosNor<0>, 0>;

public:

    constexpr CmosNor() = default;
    CmosNor(const Technology& tech, int inputs, unit::Feature nmosGateWidth, unit::Feature pmosGateWidth, unit::Feature gateRegionHeight);

private:

    static constexpr bool sequentialNmos = false;
};

} // namespace mse

#endif
