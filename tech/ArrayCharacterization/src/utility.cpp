#include "utility.hpp"

#include <utility>
#include <cstdlib>

[[noreturn]] void mse::exit(ExitCode exitCode) {
    ::exit(std::to_underlying(exitCode));
}
