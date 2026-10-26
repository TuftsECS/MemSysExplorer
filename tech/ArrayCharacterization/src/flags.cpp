#include "flags.hpp"
#include "log.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace mse::flags {

static bool printHelp(std::optional<std::string_view>) noexcept;

// Every flag the program accepts, listed in any order since help sorts them
constexpr std::array programFlagArray{
    Flag('h', "help", FlagArgument::None, "", "Print help message and exit", printHelp),
    Flag('u', "uncompressed", FlagArgument::None, "", "Print every component on its own, including the ones with only zero values",
            [] (std::optional<std::string_view>) noexcept {
        settings.compactComponents = false;

        return true;
    }),
    Flag('f', "flat", FlagArgument::None, "", "Print each component value on one line under its full path, like Bank.Mat.Timing.Read Latency",
            [] (std::optional<std::string_view>) noexcept {
        settings.flatComponents = true;

        return true;
    })
};

// Every positional the program accepts, filled by the words on the command line in this order
constexpr std::array programPositionalArray{
    Positional("config file", false, "Configuration file, nvsim.cfg when not given",
            [] (std::string_view argument) noexcept {
        settings.configFile = argument;

        return true;
    })
};

//==================================================
// Malformed flags fail to compile, so parsing never has to check them

static_assert(std::ranges::none_of(programFlagArray, [] (const Flag& flag) {
    return (!flag.shortName || *flag.shortName == '\0') && flag.longName.empty();
}), "Flag needs a short or long name");

static_assert(std::ranges::none_of(programFlagArray, [] (const Flag& flag) {
    return flag.shortName && (*flag.shortName == '\0' || *flag.shortName == '-' || *flag.shortName == '=');
}), "Flag has invalid short name");

static_assert(std::ranges::none_of(programFlagArray, [] (const Flag& flag) {
    return flag.longName.contains('=');
}), "Flag has '=' in its long name");

static_assert(std::ranges::none_of(programFlagArray, [] (const Flag& flag) {
    return !flag.function;
}), "Flag has no function");

static_assert(std::ranges::all_of(programFlagArray, [] (const Flag& flag) {
    return !flag.shortName || std::ranges::count(programFlagArray, flag.shortName, &Flag::shortName) == 1;
}), "Flag short name already in use");

static_assert(std::ranges::all_of(programFlagArray, [] (const Flag& flag) {
    return flag.longName.empty() || std::ranges::count(programFlagArray, flag.longName, &Flag::longName) == 1;
}), "Flag long name already in use");

//==================================================
// Malformed positionals fail to compile the same way

static_assert(std::ranges::none_of(programPositionalArray, [] (const Positional& positional) {
    return positional.name.empty();
}), "Positional has no name");

static_assert(std::ranges::none_of(programPositionalArray, [] (const Positional& positional) {
    return !positional.function;
}), "Positional has no function");

// Required positionals first, so the words given always fill them before any optional ones
static_assert(std::ranges::is_sorted(programPositionalArray, std::greater(), &Positional::required),
        "Optional positionals must all come after the required ones");

//==================================================

const std::span<const Flag> programFlags = programFlagArray;
const std::span<const Positional> programPositionals = programPositionalArray;

// The help flag's function, which prints the usage, the positionals in order, and the flags, then exits
static bool printHelp(std::optional<std::string_view>) noexcept {
    // Flags are listed alphabetically by the name shown first, ignoring case, with the exact name breaking ties
    std::vector<const Flag*> sortedFlags;
    for (const Flag& flag : programFlags) {
        sortedFlags.push_back(&flag);
    }
    std::ranges::sort(sortedFlags, {}, [] (const Flag* flag) {
        std::string name = flag->shortName ? std::string{*flag->shortName} : std::string(flag->longName);
        std::string lowercase = name;
        std::ranges::transform(lowercase, lowercase.begin(), [] (unsigned char letter) { return std::tolower(letter); });

        return std::pair(lowercase, name);
    });

    std::vector<std::string> forms;
    for (const Flag* flag : sortedFlags) {
        std::string form = flag->shortName ? std::format("-{}", *flag->shortName) : "  ";
        if (!flag->longName.empty()) {
            form += std::format("{}--{}", flag->shortName ? ", " : "  ", flag->longName);
        }

        const bool attached = !flag->longName.empty();
        if (flag->argument == FlagArgument::Required) {
            form += std::format("{}{}", attached ? "=" : " ", flag->argumentName);
        } else if (flag->argument == FlagArgument::Optional) {
            form += std::format("[{}{}]", attached ? "=" : "", flag->argumentName);
        }
        forms.push_back(std::move(form));
    }

    size_t width = 0;
    for (const std::string& form : forms) {
        width = std::max(width, form.size());
    }
    for (const Positional& positional : programPositionals) {
        width = std::max(width, positional.name.size());
    }

    std::string usage = std::format("Usage: {} [options]", settings.programName);
    for (const Positional& positional : programPositionals) {
        usage += positional.required ? std::format(" <{}>", positional.name) : std::format(" [{}]", positional.name);
    }
    std::cout << usage << "\n";

    if (!programPositionals.empty()) {
        std::cout << "\nPositionals:\n";
        for (const Positional& positional : programPositionals) {
            std::cout << std::format("  {:<{}} | {}\n", positional.name, width, positional.description);
        }
    }

    std::cout << "\nOptions:\n";
    for (size_t index = 0; index < forms.size(); ++index) {
        std::cout << std::format("  {:<{}} | {}\n", forms[index], width, sortedFlags[index]->description);
    }
    mse::exit(ExitCode::Success);
}

} // namespace mse::flags

//==================================================

void mse::flags::parseFlags(int argc, char* argv[]) noexcept {
    // Kept for the help usage line, argv[0] is the name the program was run with
    settings.programName = argc > 0 ? argv[0] : "";

    // Index of the word being read, starting after the program name
    int index = 1;
    // Takes the word after the current one as the argument of a flag that requires one
    // It moves the index past that word too, so the loop doesn't read it again as a flag or positional
    auto nextWord = [&index, argc, argv] () noexcept -> std::optional<std::string_view> {
        if (index + 1 >= argc) {
            return std::nullopt;
        }

        return argv[++index];
    };

    // Runs a flag's function, fatal when the function rejects the argument it was given
    // A function can also reject having no argument, which only optional argument flags can be given
    auto runFlag = [] (const Flag& flag, std::optional<std::string_view> argument) noexcept {
        if (flag.function(argument)) {
            return;
        }

        const std::string name = flag.longName.empty() ? std::format("-{}", *flag.shortName) : std::format("--{}", flag.longName);
        if (argument) {
            log::fatal(std::format("Flag {} can't use argument \"{}\"", name, *argument));
        }
        log::fatal(std::format("Flag {} needs an argument", name));
    };

    // Each word that is not a flag fills the next positional in the array
    // More words than positionals is fatal, as is a positional function rejecting its word
    size_t positionalsGiven = 0;
    auto runPositional = [&positionalsGiven] (std::string_view word) noexcept {
        if (positionalsGiven >= programPositionals.size()) {
            log::fatal(std::format("Unexpected positional \"{}\"", word));
        }

        const Positional& positional = programPositionals[positionalsGiven++];
        if (!positional.function(word)) {
            log::fatal(std::format("Positional {} can't use \"{}\"", positional.name, word));
        }
    };

    // Set by --, after which every word is a positional even when it starts with -
    bool flagsDone = false;
    for (; index < argc; ++index) {
        const std::string_view word = argv[index];

        // Every word after --, a lone -, and any word not starting with - is a positional
        // A lone - is a positional because it usually stands for stdin or stdout
        if (flagsDone || word == "-" || !word.starts_with('-')) {
            runPositional(word);
            continue;
        }

        if (word == "--") {
            flagsDone = true;
            continue;
        }

        // A long flag, with its argument after = or in the next word when it requires one
        if (word.starts_with("--")) {
            // The name runs up to the first =, and anything after it is the attached argument
            // Only the first = splits, so --size=a=b gives the argument a=b and --size= gives an empty one
            std::string_view body = word.substr(2);
            size_t equals = body.find('=');
            std::string_view name = body.substr(0, equals);

            const Flag& flag = findLong(name);

            std::optional<std::string_view> argument;
            if (equals != std::string_view::npos) {
                argument = body.substr(equals + 1);
            }

            // Not fatal, the flag was clearly meant to be given so it still runs without the argument
            if (flag.argument == FlagArgument::None && argument) {
                log::error(std::format("Flag --{} does not take an argument, ignoring \"{}\"", name, *argument));
                argument = std::nullopt;
            }

            // A required argument that isn't attached is the next word, even one starting with - like -5
            // An optional argument is never taken from the next word, so --color config.yaml leaves config.yaml a positional
            if (flag.argument == FlagArgument::Required && !argument) {
                argument = nextWord();
                if (!argument) {
                    log::fatal(std::format("Flag --{} requires an argument", name));
                }
            }

            runFlag(flag, argument);
            continue;
        }

        // A group of short flags, where the first one taking an argument uses the rest of the word
        for (size_t position = 1; position < word.size(); ++position) {
            const char name = word[position];

            const Flag& flag = findShort(name);

            // The characters after this flag's letter, which are more flags or this flag's argument
            const std::string_view rest = word.substr(position + 1);

            // A flag without an argument runs right away and the group continues with the next letter
            // An = after it can't be another flag, so the rest of the word is reported and skipped
            if (flag.argument == FlagArgument::None) {
                runFlag(flag, std::nullopt);
                if (rest.starts_with('=')) {
                    log::error(std::format("Flag -{} does not take an argument, ignoring \"{}\"", name, rest.substr(1)));
                    break;
                }
                continue;
            }

            // A flag taking an argument uses the whole rest of the word, with one leading = dropped
            // So -sarg and -s=arg both give arg, and -s== gives =
            std::optional<std::string_view> argument;
            if (rest.starts_with('=')) {
                argument = rest.substr(1);
            } else if (!rest.empty()) {
                argument = rest;
            }

            // Like long flags, only a required argument can come from the next word
            if (flag.argument == FlagArgument::Required && !argument) {
                argument = nextWord();
                if (!argument) {
                    log::fatal(std::format("Flag -{} requires an argument", name));
                }
            }

            // The rest of the word was this flag's argument, so the group ends here
            runFlag(flag, argument);
            break;
        }
    }

    // Required positionals come first, so only the next unfilled one needs checking
    if (positionalsGiven < programPositionals.size() && programPositionals[positionalsGiven].required) {
        log::fatal(std::format("Missing positional {}", programPositionals[positionalsGiven].name));
    }
}

//==================================================

const mse::flags::Flag& mse::flags::findShort(char name) noexcept {
    auto flag = std::ranges::find(programFlags, std::optional<char>(name), &Flag::shortName);
    if (flag == programFlags.end()) {
        log::fatal(std::format("Unknown flag -{}", name));
    }

    return *flag;
}

//==================================================

const mse::flags::Flag& mse::flags::findLong(std::string_view name) noexcept {
    // Short only flags have an empty long name, so an empty name never matches
    auto flag = name.empty() ? programFlags.end() : std::ranges::find(programFlags, name, &Flag::longName);
    if (flag == programFlags.end()) {
        log::fatal(std::format("Unknown flag --{}", name));
    }

    return *flag;
}
