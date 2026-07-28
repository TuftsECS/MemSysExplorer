#include "log.hpp"

#include <iomanip>
#include <utility>

//==================================================

void mse::Logger::applyDefaultConfiguration() noexcept {
    *_stream << std::boolalpha; // Booleans are display as "true" and "false"
    setFixedFormat();
    setPrecision(6); // Default 6 decimals
}

//==================================================

mse::Logger& mse::Logger::blank() noexcept {
    log("\n");
    return *this;
}

//==================================================

mse::Logger& mse::Logger::flush() noexcept {
    *_stream << std::flush;
    return *this;
}

//==================================================

mse::Logger& mse::Logger::setBase(NumberBase base) noexcept {
    switch (base) {
        case NumberBase::Decimal:
            setDecimalBase();
            break;
        case NumberBase::Hex:
            setHexBase();
            break;
        case NumberBase::Octal:
            setOctalBase();
            break;
    }
    return *this;
}

mse::Logger& mse::Logger::setBase(int base) noexcept {
    switch (base) {
        case 10:
            setDecimalBase();
            break;
        case 16:
            setHexBase();
            break;
        case 8:
            setOctalBase();
            break;
        default:
            warn("Attempted to set number base to unsupported value of {}, defaulting to decimal / base 10\n", base);
            setDecimalBase();
            break;
    }
    return *this;
}

mse::Logger& mse::Logger::setDecimalBase() noexcept {
    *_stream << std::dec;
    return *this;
}

mse::Logger& mse::Logger::setHexBase() noexcept {
    *_stream << std::hex;
    return *this;
}

mse::Logger& mse::Logger::setOctalBase() noexcept {
    *_stream << std::oct;
    return *this;
}

//==================================================

mse::Logger& mse::Logger::setFormat(NumberFormat format) noexcept {
    switch (format) {
        case NumberFormat::Sigfig:
            setSigfigFormat();
            break;
        case NumberFormat::Fixed:
            setFixedFormat();
            break;
        case NumberFormat::Scientific:
            setScientificFormat();
            break;
    }
    return *this;
}

mse::Logger& mse::Logger::setSigfigFormat() noexcept {
    *_stream << std::defaultfloat;
    return *this;
}

mse::Logger& mse::Logger::setFixedFormat() noexcept {
    *_stream << std::fixed;
    return *this;
}

mse::Logger& mse::Logger::setScientificFormat() noexcept {
    *_stream << std::scientific;
    return *this;
}

//==================================================

mse::Logger& mse::Logger::setPrecision(int precision) noexcept {
    *_stream << std::setprecision(precision);
    return *this;
}
