#include "debug.hpp"

#include <iostream>

[[maybe_unused]] void mse::debug::log(const std::string& msg) {
    std::cout << msg;
}

[[maybe_unused]] void mse::debug::warn(const std::string& msg, [[maybe_unused]] std::source_location location) {
#ifdef MSE_DEBUG
    std::cerr << "[WARN][" << location.file_name() << ":" << location.line() << " @ " << location.function_name() << "] " << msg << std::flush;
#else
    std::cerr << "[WARN] " << msg << std::flush;
#endif
}

[[maybe_unused]] void mse::debug::warn(bool condition, const std::string& msg, [[maybe_unused]] std::source_location location) {
    if (condition) {
#ifdef MSE_DEBUG
        std::cerr << "[WARN][" << location.file_name() << ":" << location.line() << " @ " << location.function_name() << "] " << msg << std::flush;
#else
        std::cerr << "[WARN] " << msg << std::flush;
#endif
    }
}

[[maybe_unused, noreturn]] void mse::debug::fatal(const std::string& msg, int exitCode, [[maybe_unused]] std::source_location location) {
#ifdef MSE_DEBUG
    std::cerr << "[FATAL][" << location.file_name() << ":" << location.line() << " @ " << location.function_name() << "] " << msg << std::flush;
#else
    std::cerr << "[FATAL] " << msg << std::flush;
#endif
    exit(exitCode);
}

[[maybe_unused]] void mse::debug::fatal(bool condition, const std::string& msg, int exitCode, [[maybe_unused]] std::source_location location) {
    if (condition) {
#ifdef MSE_DEBUG
        std::cerr << "[FATAL][" << location.file_name() << ":" << location.line() << " @ " << location.function_name() << "] " << msg << std::flush;
#else
        std::cerr << "[FATAL] " << msg << std::flush;
#endif
        exit(exitCode);
    }
}
