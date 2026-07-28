// This file defines the OutputFile class that wraps an output stream
// The class provides a simple interface that serves as a standard way to output information
// The style of decorated messages is consistent and controlled in one place in the program
// A global object mse::output is connected to std::cout at the bottom of this file and is the default way to print
//
// Some stream controls have also been turned into OutputFile methods
// A flush method forces the stream buffer to update the output target
// When outputting certain values, a base of 16 or 8 may more easily represent a value
// Use either the setBase method or one of setDecimalBase, setHexBase, or setOctalBase to change the output integer base
// Floating point precision can be set with the setPrecision method
// For outputting different representations of floating point numbers, the number format can also be changed
// Use either the setFormat method or one of setSigfigFormat, setFixedFormat, or setScientificFormat
// A default state is set upon OutputFile construction and can be reapplied with applyDefaultConfiguration

#ifndef MSE_OUTPUT_HH
#define MSE_OUTPUT_HH

#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <memory>

namespace mse {

// Enums for choosing which style of message to use
// For standard message decoration
enum class PrintStyle {
    Info,
    Warn,
    Error,
    Fatal
};

class OutputFile {
private:

    // A smart pointer to manage an ofstream object or an ostream object like std::cout
    // The second template type is normally omitted, but to write this code cleanly, we need a custom destructor
    // Whenever a created ofstream object is wrapped, it will be default destroyed
    // But whenever a reference to an existing ostream like std::cout is wrapped, it will not be destroyed at all
    // This just lets us creatively use one smart pointer member for all the code
    std::unique_ptr<std::ostream, std::function<void(std::ostream*)>> _stream;

public:

    // A string that is automatically insterted between print() arguments
    std::string printSeparator;

    //==================================================
    // An OutputFile can wrap some existing ostream (std::cout (default), std::cerr, ...)
    // Or it can create a new output stream to a file by taking a path to it
    //   An existing file at the path will be overwritten

    OutputFile(std::ostream& stream) noexcept
            : _stream(&stream, [] (auto) {} ) {} // This lambda takes any pointer and does nothing

    OutputFile(const std::string& filename) noexcept
            : _stream(new std::ofstream(filename), std::default_delete<std::ostream>()) { // New, owned ofstream, use default deleter
        applyDefaultConfiguration(); // Apply the default configuration, not done for reference constructor
    }

    //==================================================
    // Resets the OutputFile output to the default state

    void applyDefaultConfiguration() noexcept;

    //==================================================
    // A custom () operator overload lets OutputFile objects be used like functions
    // This is the one way to print from an OutputFile object
    // OutputFile() prints a newline
    // OutputFile(arg1, arg2, ...) prints arguments in order
    // OutputFile(style, arg1, arg2, ...) uses a custom style to print arguments in order

    OutputFile& operator()() noexcept {
        *_stream << '\n';
        return *this;
    }

    template <typename... Ts>
    OutputFile& operator()(Ts&&... args) noexcept {
        std::string_view sep = "";
        ((*_stream << sep << std::forward<Ts>(args), sep = printSeparator), ...);
        return *this;
    }

    template <typename... Ts>
    OutputFile& operator()(PrintStyle style, Ts&&... args) noexcept {
        switch (style) {
            case PrintStyle::Info:
                *_stream << "[INFO] ";
                break;
            case PrintStyle::Warn:
                *_stream << "[WARN] ";
                break;
            case PrintStyle::Error:
                *_stream << "[ERROR] ";
                break;
            case PrintStyle::Fatal:
                *_stream << "[FATAL] ";
                break;
        }

        std::string_view sep = "";
        ((*_stream << sep << std::forward<Ts>(args), sep = printSeparator), ...);

        // Fatal messages are mirrored to cerr
        if (style == PrintStyle::Fatal) {
            sep = "";
            ((std::cerr << sep << std::forward<Ts>(args), sep = printSeparator), ...);
        }

        return *this;
    }

    //==================================================
    // Force the stream to write buffered data to its target

    OutputFile& flush() noexcept;

    //==================================================
    // Change the integer output base with one of NumberBase in setBase
    //   Or provide a valid integer for a base to use (8, 16, 10)
    //   Or use setDecimalBase, setHexBase, or setOctalBase

    enum class NumberBase {
        Decimal,
        Hex,
        Octal
    };

    OutputFile& setBase(NumberBase base) noexcept;
    OutputFile& setBase(int base) noexcept;

    OutputFile& setDecimalBase() noexcept;
    OutputFile& setHexBase() noexcept;
    OutputFile& setOctalBase() noexcept;

    //==================================================
    // Change the format of floating point numbers with one of NumberFormat in setFormat
    //   Or use setSigfigFormat, setFixedFormat, or setScientificFormat

    enum class NumberFormat {
        Sigfig,
        Fixed,
        Scientific
    };

    OutputFile& setFormat(NumberFormat format) noexcept;

    OutputFile& setSigfigFormat() noexcept;
    OutputFile& setFixedFormat() noexcept;
    OutputFile& setScientificFormat() noexcept;

    //==================================================
    // Adjust the precision of floating point numbers
    //   In Sigfig format, this controls the number of significant figures, not digits after the decimal point
    //   In Fixed format, this controls the number of digits after the decimal point

    OutputFile& setPrecision(int precision) noexcept;

    //==================================================
    // Set the print argument separator string
    // Can also be modified directly through the member

    OutputFile& setSeparator(const std::string& separator);
};

//==================================================
// Standard, global output to use throughout the program

inline OutputFile output{std::cout};

} // namespace mse

#endif
