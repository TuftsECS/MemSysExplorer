#ifndef MSE_PASS_HPP
#define MSE_PASS_HPP

#include "unit.hpp"
#include "Dimensions.hpp"

class Technology;

namespace mse {

enum class PassType {
    Nmos,
    Pmos
};

//==================================================
// Pass device abstract base that unifies an interface for series-connected transistor stacks

class PassDevice {
public:

    virtual ~PassDevice() = default;

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
// Pass-transistor stack model that computes all values once during construction

template <PassType Type, int Inputs>
class PassStack : public PassDevice {
public:

    constexpr PassStack() = default;
    PassStack(const Technology& tech, unit::Feature gateWidth, unit::Feature maxRegionHeight);

    constexpr int inputCount() const noexcept override final { return Inputs; }
    unit::Ohm resistance() const noexcept { return _resistance; }

private:

    unit::Ohm _resistance;
};

template <int Inputs>
using PassNmosStack = PassStack<PassType::Nmos, Inputs>;

template <int Inputs>
using PassPmosStack = PassStack<PassType::Pmos, Inputs>;

} // namespace mse

#endif
