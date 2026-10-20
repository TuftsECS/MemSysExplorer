// This file defines global functions for sending messages to a logging file
// For use with messages requiring attention that are not part of normal output
// Like error or warning messages about anyhting related to the program
// Or general debug information
// Any extraneous information users need to be made aware of
//
// By default, the log stream is set to stderr
//
// The Level enum is used to filter which messages are enabled and give messages tags
//   At the Debug level, all debug, warn, and error messages are reported
//   At Warn, all warn and error messages are reported
//   At Error, only error messages are reported
// The Quiet level, the final enum value, supresses all log message output
// The level can be changed by writing to the atomic level variable
//
// Writing to the log is protected with a mutex for thread safety
// All messages are appended with a newline and flush the log stream

#ifndef MSE_LOG_HPP
#define MSE_LOG_HPP

#include "utility.hpp"

#include <ostream>
#include <string>
#include <string_view>
#include <atomic>

namespace mse::log {

enum class Level {
    Debug,
    Warn,
    Error,
    Quiet
};

inline std::atomic<Level> level = Level::Warn;

//==================================================
// These operations require a lock

void setLogStream(std::ostream& stream) noexcept; // Change the log output stream
void write(std::string_view tag, std::string_view message) noexcept; // Write to the log output stream

//==================================================
// Functions to call for logging

inline void debug(std::string_view message) noexcept {
    if (level <= Level::Debug) {
        write("", message);
    }
}

inline void warn(std::string_view message) noexcept {
    if (level <= Level::Warn) {
        write("[WARN] ", message);
    }
}

inline void error(std::string_view message) noexcept {
    if (level <= Level::Error) {
        write("[ERROR] ", message);
    }
}

[[noreturn]] inline void fatal(std::string_view message) noexcept {
    write("[FATAL] ", message);
    mse::exit(ExitCode::Failure);
}

} // namespace mse::log

#endif
