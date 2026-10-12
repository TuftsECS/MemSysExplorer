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

    virtual Dimensions dimensions() const noexcept = 0;
    virtual unit::Farad drainCapacitance() const noexcept = 0;
    virtual unit::Farad gateCapacitance() const noexcept = 0;
    virtual unit::Ampere leakageCurrent() const noexcept = 0;
};

//==================================================
// Pass-transistor stack model

template <PassType Type, int Inputs>
class PassStack : public PassDevice {
private:

    const Technology* _tech = nullptr;
    unit::Feature _gateWidth;
    unit::Feature _maxRegionHeight;
    int _tempIndex = 0;

public:

    constexpr PassStack() = default;
    PassStack(const Technology& tech, unit::Feature gateWidth, unit::Feature maxRegionHeight);

    constexpr int inputCount() const noexcept override final { return Inputs; }

    Dimensions dimensions() const noexcept override final;
    unit::Farad drainCapacitance() const noexcept override final;
    unit::Farad gateCapacitance() const noexcept override final;
    unit::Ampere leakageCurrent() const noexcept override final;
    unit::Ohm resistance() const noexcept;
};

template <int Inputs>
using PassNmosStack = PassStack<PassType::Nmos, Inputs>;

template <int Inputs>
using PassPmosStack = PassStack<PassType::Pmos, Inputs>;

} // namespace mse

#endif
