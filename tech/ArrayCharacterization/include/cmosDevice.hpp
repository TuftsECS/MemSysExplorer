#ifndef MSE_CMOS_HPP
#define MSE_CMOS_HPP

#include "unit.hpp"
#include "Dimensions.hpp"
#include "Technology.hpp"

#include <functional>


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

    virtual Dimensions dimensions() const noexcept = 0;
    virtual unit::Farad drainCapacitance() const noexcept = 0;
    virtual unit::Farad gateCapacitance() const noexcept = 0;
    virtual unit::Ampere leakageCurrent() const noexcept = 0;
};

//==================================================
// CMOS tranistor model base class

class CmosTransistor : public CmosDevice {
private:

    const Technology* _tech = nullptr;
    unit::Feature _gateWidth;
    unit::Feature _regionHeight;
    const Technology::TemperatureTable* _onCurrent = nullptr;
    const Technology::TemperatureTable* _offCurrent = nullptr;
    unit::Volt _vdsat;
    Technology::Mobility _mobility;
    int _tempIndex = 0;

public:

    constexpr int inputCount() const noexcept override final { return 1; }

    Dimensions dimensions() const noexcept override final;
    unit::Farad drainCapacitance() const noexcept override final;
    unit::Farad gateCapacitance() const noexcept override final;
    unit::Ampere leakageCurrent() const noexcept override final;
    unit::Ohm channelResistance() const noexcept;
    unit::Siemens transconductance() const noexcept;

protected:

    constexpr CmosTransistor() = default;
    CmosTransistor(
        const Technology& tech,
        unit::Feature gateWidth,
        unit::Feature transistorRegionHeight,
        const Technology::TemperatureTable& onCurrent,
        const Technology::TemperatureTable& offCurrent,
        unit::Volt vdsat,
        Technology::Mobility mobility
    );
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
// CMOS gate model
// When inputs is > 0, that value is used as a compile-time constant for a number of inputs
// When inputs is = 0, a runtime value is supplied to the constructor for the number of inputs

template <GateType Gate, int Inputs>
class CmosGate : public CmosDevice {
private:

    static constexpr bool sequentialNmos = Gate != GateType::Nor;

    const Technology* _tech = nullptr;
    int _inputs = 0;
    unit::Feature _unitNmosWidth;
    unit::Feature _unitPmosWidth;
    unit::Feature _nmosWidth;
    unit::Feature _pmosWidth;
    unit::Feature _nmosMaxWidth;
    unit::Feature _pmosMaxWidth;
    unit::Number _logicalEffort;
    int _tempIndex = 0;

public:

    constexpr CmosGate() = default;
    CmosGate(const Technology& tech, unit::Feature unitNmosGateWidth, unit::Feature unitPmosGateWidth, unit::Feature gateRegionHeight) requires (Inputs > 0);
    CmosGate(const Technology& tech, int inputs, unit::Feature unitNmosGateWidth, unit::Feature unitPmosGateWidth, unit::Feature gateRegionHeight);

    constexpr int inputCount() const noexcept override final { return _inputs; }

    Dimensions dimensions() const noexcept override final;
    unit::Farad drainCapacitance() const noexcept override final;
    unit::Farad gateCapacitance() const noexcept override final;
    unit::Ampere leakageCurrent() const noexcept override final;
    unit::Ohm nmosResistance() const noexcept;
    unit::Ohm pmosResistance() const noexcept;
    unit::Number logicalEffort() const noexcept { return _logicalEffort; }
    // Effective widths
    unit::Feature nmosWidth() const noexcept { return _nmosWidth; }
    unit::Feature pmosWidth() const noexcept { return _pmosWidth; }

private:

    struct Layout {
        int nmosFoldDegree = 0;
        int pmosFoldDegree = 0;
        unit::Feature nmosRegionHeight;
        unit::Feature pmosRegionHeight;
    };

    Layout layout() const noexcept;
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
