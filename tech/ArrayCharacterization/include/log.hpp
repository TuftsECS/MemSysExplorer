// This file defines the Logger class that wraps an output stream
// The class provides a simple interface that serves as a standard way to log information
// Logger objects can call log, info, warn, error, and fatal methods to log different priority messages
// This way, the style of decorated messages is consistent and controlled in one place in the program
// The ...Line variants each log a quick one-liner and automatically append newlines to their string arguments
// Logger objects can also be treated like a C++ stream with an overloaded, forwarding << operator
// In addition to the decorated logging, debugging information can also be printed to logs
//
//==================================================
// When compiled with the macro
//         MSE_DEBUG
// defined above this header, source locations of log messages are output
// This allows users to easily pin down exactly where a log message was printed and debug further
//==================================================
//
// Some stream controls have also been turned into Logger methods
// A flush method forces the stream buffer to update the output target
//   Warnings and errors automatically flush after writing
// When outputting certain values, a base of 16 or 8 may more easily represent a value
// Use either the setBase method or one of setDecimalBase, setHexBase, or setOctalBase to change the output integer base
// Floating point precision can be set with the setPrecision method
// For outputting different representations of floating point numbers, the number format can also be changed
// Use either the setFormat method or one of setSigfigFormat, setFixedFormat, or setScientificFormat
// A default state is set upon Logger construction and can be reapplied with applyDefaultConfiguration
//
// At the bottom of this file, two default Logger objects are globally created inline
// outputLog connects to std::cout and should be used whenever standard information needs to be printed
// errorLog connects to std::cerr and should be used if urgent information needs to be displayed in an appropriate channel
// outputLog can of course log fatal messages, and errorLog can output info messages
// But you should be able to tell which log to use hopefully

#ifndef MSE_LOG_HH
#define MSE_LOG_HH

#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <memory>
#include <source_location>
#include <format>

namespace mse {

class Logger {
private:

    // A smart pointer to manage an ofstream object or an ostream object like std::cout
    // The second template type is normally omitted, but to write this code cleanly, we need a custom destructor
    // Whenever a created ofstream object is wrapped, it will be default destroyed
    // But whenever a reference to an existing ostream like std::cout is wrapped, it will not be destroyed at all
    // This just lets us creatively use one smart pointer member for all the code
    std::unique_ptr<std::ostream, std::function<void(std::ostream*)>> _stream;

    // A type workaround for format logging functions below
    template <typename... Ts>
    struct LocationFormat {
        std::format_string<Ts...> format;
        std::source_location loc;

        consteval LocationFormat(const auto& format, std::source_location loc = std::source_location::current()) noexcept
                : format(format),
                  loc(loc) {}
    };

public:

    //==================================================
    // A Logger can wrap some existing ostream (std::cout (default), std::cerr, ...)
    // Or it can create a new output stream to a file by taking a path to it
    //   An existing file at the path will be overwritten

    Logger(std::ostream& stream = std::cout) noexcept
            : _stream(&stream, [] (auto) {} ) {} // This lambda takes any pointer and does nothing

    Logger(const std::string& filename) noexcept
            : _stream(new std::ofstream(filename), std::default_delete<std::ostream>()) { // New, owned ofstream, use default deleter
        applyDefaultConfiguration(); // Apply the default configuration, not done for reference constructor
    }

    //==================================================
    // Resets the Logger output to the default state

    void applyDefaultConfiguration() noexcept;

    //==================================================
    // A custom << operator overload lets Logger objects be used just like std::cout or other streams

    template <typename T>
    Logger& operator<<(T&& value) noexcept {
        *_stream << std::forward<T>(value);
        return *this;
    }

    //==================================================
    // Logging methods to have standard ouput style across the program
    // Each function uses format strings and variadic arguments for the std::format function
    // log("message") - outputs "str"
    // info("message") - outputs "[INFO] message"
    // warn("message") - outputs "[WARN] message"
    // error("message") - outputs "[ERROR] message"
    // fatal("message") - outputs "[FATAL] message"
    //==================================================

    template <typename... Ts>
    Logger& log(const LocationFormat<std::type_identity_t<Ts>...>& message, Ts&&... args) noexcept {
        printSourceLocation(message.loc); // Only print debug source information with the MSE_DEBUG macro
        *_stream << std::format(message.format, std::forward<Ts>(args)...);
        return *this;
    }

    //==================================================

    template <typename... Ts>
    Logger& info(const LocationFormat<std::type_identity_t<Ts>...>& message, Ts&&... args) noexcept {
        printSourceLocation(message.loc); // Only print debug source information with the MSE_DEBUG macro
        *_stream << "[INFO] " << std::format(message.format, std::forward<Ts>(args)...);
        return *this;
    }

    //==================================================

    template <typename... Ts>
    Logger& warn(const LocationFormat<std::type_identity_t<Ts>...>& message, Ts&&... args) noexcept {
        printSourceLocation(message.loc); // Only print debug source information with the MSE_DEBUG macro
        *_stream << "[WARN] " << std::format(message.format, std::forward<Ts>(args)...);
        return *this;
    }

    //==================================================

    template <typename... Ts>
    Logger& error(const LocationFormat<std::type_identity_t<Ts>...>& message, Ts&&... args) noexcept {
        printSourceLocation(message.loc); // Only print debug source information with the MSE_DEBUG macro
        *_stream << "[ERROR] " << std::format(message.format, std::forward<Ts>(args)...);
        return *this;
    }

    //==================================================

    template <typename... Ts>
    Logger& fatal(const LocationFormat<std::type_identity_t<Ts>...>& message, Ts&&... args) noexcept {
        printSourceLocation(message.loc); // Only print debug source information with the MSE_DEBUG macro
        *_stream << "[FATAL] " << std::format(message.format, std::forward<Ts>(args)...);
        return *this;
    }

    //==================================================
    // Output a blank line

    Logger& blank() noexcept;

    //==================================================
    // Force the stream to write buffered data to its target

    Logger& flush() noexcept;

    //==================================================
    // Change the integer output base with one of NumberBase in setBase
    //   Or provide a valid integer for a base to use (8, 16, 10)
    //   Or use setDecimalBase, setHexBase, or setOctalBase

    enum class NumberBase {
        Decimal,
        Hex,
        Octal
    };

    Logger& setBase(NumberBase base) noexcept;
    Logger& setBase(int base) noexcept;

    Logger& setDecimalBase() noexcept;
    Logger& setHexBase() noexcept;
    Logger& setOctalBase() noexcept;

    //==================================================
    // Change the format of floating point numbers with one of NumberFormat in setFormat
    //   Or use setSigfigFormat, setFixedFormat, or setScientificFormat

    enum class NumberFormat {
        Sigfig,
        Fixed,
        Scientific
    };

    Logger& setFormat(NumberFormat format) noexcept;

    Logger& setSigfigFormat() noexcept;
    Logger& setFixedFormat() noexcept;
    Logger& setScientificFormat() noexcept;

    //==================================================
    // Adjust the precision of floating point numbers
    //   In Sigfig format, this controls the number of significant figures, not digits after the decimal point
    //   In Fixed format, this controls the number of digits after the decimal point

    Logger& setPrecision(int precision) noexcept;

private:

    //==================================================
    // Outputs debug source locations
    // Optionally compiled with the MSE_DEBUG macro defined

    void printSourceLocation( [[maybe_unused]] const std::source_location& loc) noexcept {
        // Outputs as "{file:## @ function}"
        // Example from a fatal(message) method call: "[FATAL] {code.cpp:21 @ totallySafeFunction} message..."
#ifdef MSE_DEBUG
        *_stream << "{" << loc.file_name() << ":" << loc.line() << " @ " << loc.function_name() << "} ";
#endif
    }
};

//==================================================
// Standard, global logs to use throughout the program

inline Logger outputLog{std::cout};
inline Logger errorLog{std::cerr};

} // namespace mse

#endif
