// M. Horowitz, "Timing Models for MOS Circuits", Stanford PhD thesis, 1984

#ifndef MSE_PHYSICS_HOROWITZ_HPP
#define MSE_PHYSICS_HOROWITZ_HPP

#include "unit.hpp"

#include <cmath>

namespace mse::physics {

using HorowitzRamp = unit::SecondExp<-1>;

struct HorowitzResult {
    unit::Second delay;
    HorowitzRamp outputRamp;
};

inline constexpr HorowitzRamp stepRamp{1e20};
inline constexpr HorowitzRamp infiniteRamp{1e41};

//==================================================

// tau is the stage RC time constant
// beta is 1 / (R * gm) of the driving transistor, 0 for a pure RC stage like a wire or bitline
// inputRamp is the input slope 1 / rise time

inline HorowitzResult horowitz(unit::Second tau, unit::Number beta, HorowitzRamp inputRamp = infiniteRamp) noexcept {
    static constexpr unit::Number vs = 0.5; // Normalized switching voltage
    unit::Number alpha = unit::Number(1) / (inputRamp * tau);
    unit::Second delay = tau * std::sqrt(std::log(vs) * std::log(vs) + 2 * alpha * beta * (1 - vs));
    return {
        delay,
        (1 - vs) / delay
    };
}

} // namespace mse::physics

#endif
