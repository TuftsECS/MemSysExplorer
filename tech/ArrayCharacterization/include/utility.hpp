#ifndef MSE_UTILITY_HPP
#define MSE_UTILITY_HPP

#include "exitCodes.hpp"

namespace mse {

//==================================================
// Shorthand exit function

[[noreturn]] void exit(ExitCode exitCode);

}

#endif
