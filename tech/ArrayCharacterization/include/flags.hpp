// This file defines the command line flags and positional arguments of the program and the function that parses them
//
// Flags and positionals are listed once in constant arrays in flags.cpp, which are checked at compile time
// A flag has a short form like -s, a long form like --size, or both, and takes no argument, an optional one, or a required one
//   No argument: -a, --alpha, and short flags group together like -abc
//   Required argument: -s arg, -sarg, -s=arg, --size arg, --size=arg
//   Optional argument: -c, -cX, -c=X, --color, --color=X, the argument has to be attached like getopt
// Words that are not flags fill the positionals in the order they are listed, and every word after -- is a positional
//
// Each flag and positional has a function that runs as soon as it is read, once each time it is given
// The functions write into the global settings, so a setting keeps its default when nothing on the command line sets it
// A flag function gets its argument, or nullopt when there is none, and returns false when it can't use the argument
//
// Unknown flags, missing arguments, rejected arguments, and wrong positional counts are fatal
// An argument given to a flag that takes none is only an error, the flag still runs without it

#ifndef MSE_FLAGS_HPP
#define MSE_FLAGS_HPP

#include <optional>
#include <vector>
#include <span>
#include <string_view>

namespace mse::flags {

enum class FlagArgument {
    None,
    Optional,
    Required
};

using FlagFunction = bool (*)(std::optional<std::string_view> argument) noexcept;

struct Flag {
    std::optional<char> shortName;
    std::string_view longName;
    FlagArgument argument;
    std::string_view argumentName;
    std::string_view description;
    FlagFunction function;

    // Short and long flags, short only flags can pass an empty long name
    constexpr Flag(char shortName,
         std::string_view longName,
         FlagArgument argument,
         std::string_view argumentName,
         std::string_view description,
         FlagFunction function) noexcept
            : shortName(shortName),
              longName(longName),
              argument(argument),
              argumentName(argumentName),
              description(description),
              function(function) {}

    // Long only flags
    constexpr Flag(std::string_view longName,
         FlagArgument argument,
         std::string_view argumentName,
         std::string_view description,
         FlagFunction function) noexcept
            : longName(longName),
              argument(argument),
              argumentName(argumentName),
              description(description),
              function(function) {}
};

using PositionalFunction = bool (*)(std::string_view argument) noexcept;

struct Positional {
    std::string_view name;
    bool required;
    std::string_view description;
    PositionalFunction function;

    constexpr Positional(std::string_view name,
         bool required,
         std::string_view description,
         PositionalFunction function) noexcept
            : name(name),
              required(required),
              description(description),
              function(function) {}
};

// Values the flag and positional functions set, read by the rest of the program after parsing
struct Settings {
    std::string_view programName;
    // Empty when no config file is given, so the default is chosen where it is loaded
    std::optional<std::string_view> configFile;
};

inline Settings settings;

// Views of the flag and positional arrays defined in flags.cpp
extern const std::span<const Flag> programFlags;
extern const std::span<const Positional> programPositionals;

//==================================================

// Reads every program argument in order and runs the function of each flag and positional it finds
void parseFlags(int argc, char* argv[]) noexcept;

// Finds a flag by name, fatal when no flag has it
const Flag& findShort(char name) noexcept;
const Flag& findLong(std::string_view name) noexcept;

} // namespace mse::flags

#endif
