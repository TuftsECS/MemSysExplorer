#include "output.hpp"

#include <iomanip>
#include <utility>

//==================================================

void mse::OutputFile::applyDefaultConfiguration() noexcept {
    *_stream << std::boolalpha; // Booleans are displayed as "true" and "false"
    setFixedFormat();
    setPrecision(6); // 6 decimals
    setSeparator(""); // No argument separator
}

//==================================================

mse::OutputFile& mse::OutputFile::flush() noexcept {
    *_stream << std::flush;
    return *this;
}

//==================================================

mse::OutputFile& mse::OutputFile::setBase(NumberBase base) noexcept {
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

mse::OutputFile& mse::OutputFile::setBase(int base) noexcept {
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
            operator()(PrintStyle::Warn, "Attempted to set number base to unsupported value of {}, defaulting to decimal / base 10\n", base);
            setDecimalBase();
            break;
    }
    return *this;
}

mse::OutputFile& mse::OutputFile::setDecimalBase() noexcept {
    *_stream << std::dec;
    return *this;
}

mse::OutputFile& mse::OutputFile::setHexBase() noexcept {
    *_stream << std::hex;
    return *this;
}

mse::OutputFile& mse::OutputFile::setOctalBase() noexcept {
    *_stream << std::oct;
    return *this;
}

//==================================================

mse::OutputFile& mse::OutputFile::setFormat(NumberFormat format) noexcept {
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

mse::OutputFile& mse::OutputFile::setSigfigFormat() noexcept {
    *_stream << std::defaultfloat;
    return *this;
}

mse::OutputFile& mse::OutputFile::setFixedFormat() noexcept {
    *_stream << std::fixed;
    return *this;
}

mse::OutputFile& mse::OutputFile::setScientificFormat() noexcept {
    *_stream << std::scientific;
    return *this;
}

//==================================================

mse::OutputFile& mse::OutputFile::setPrecision(int precision) noexcept {
    *_stream << std::setprecision(precision);
    return *this;
}

//==================================================

mse::OutputFile& mse::OutputFile::setSeparator(const std::string& separator) {
    printSeparator = separator;
    return *this;
}
