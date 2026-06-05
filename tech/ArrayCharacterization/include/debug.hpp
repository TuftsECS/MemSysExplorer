#ifndef MSE_DEBUG_HPP
#define MSE_DEBUG_HPP

#include <string>
#include <source_location>

// Define MSE_DEBUG to enable more detailed warn and fatal messages including source location where called

namespace mse::debug {

[[maybe_unused]] void log(const std::string& msg);

[[maybe_unused]] void warn(const std::string& msg, [[maybe_unused]] std::source_location location = std::source_location::current());
[[maybe_unused]] void warn(bool condition, const std::string& msg, [[maybe_unused]] std::source_location location = std::source_location::current());

[[maybe_unused, noreturn]] void fatal(const std::string& msg, int exitCode = EXIT_FAILURE, [[maybe_unused]] std::source_location location = std::source_location::current());
[[maybe_unused]] void fatal(bool condition, const std::string& msg, int exitCode = EXIT_FAILURE, [[maybe_unused]] std::source_location location = std::source_location::current());

}

#endif
