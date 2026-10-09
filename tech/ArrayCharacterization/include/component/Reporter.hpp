// This file defines the Reporter which components fill with their values when report(Reporter&) is called on them
// Components report each value with a title and the Reporter formats it to a string right away
// Any type that can be written to an ostream can be reported like unit types and Dimensions
// Several values can be passed in one call and are written back to back
//
// Values are sorted into the groups listed in ReportGroup or into an ungrouped list when no group is given
// The printing code later walks reportGroups in order and prints each nonempty group and then the ungrouped values
//
// Formatting copies the flags of a base stream so reported values match the rest of the output

#ifndef MSE_COMPONENT_REPORTER_HPP
#define MSE_COMPONENT_REPORTER_HPP

#include <array>
#include <concepts>
#include <vector>
#include <string>
#include <string_view>
#include <ostream>
#include <sstream>
#include <utility>

namespace mse::component {

// Categories that reported values are sorted into
enum class ReportGroup {
    Dimensions,
    Timing,
    Power
};

// A reported value already formatted into a string
struct ReportValue {
    std::string title;
    std::string value;
};

//==================================================

// Any type that can be written to an output stream with <<
template <typename T>
concept Streamable = requires(std::ostream& stream, const T& value) { stream << value; };

class Reporter {
public:

    // Every group paired with its printed name in printing order
    static constexpr std::array<std::pair<ReportGroup, std::string_view>, 3> reportGroups = {{
        {ReportGroup::Dimensions, "Dimensions"},
        {ReportGroup::Timing, "Timing"},
        {ReportGroup::Power, "Power"}
    }};

private:

    // Reused stream for formatting values holding the base stream's flags
    std::ostringstream _format;

    // One list of values per group
    std::array<std::vector<ReportValue>, reportGroups.size() + 1> _groups;

public:

    // Values are formatted with the same flags as baseFormat
    explicit Reporter(const std::ostream& baseFormat) noexcept {
        _format.copyfmt(baseFormat);
    }

    //==================================================

    // Adds a titled value to a group
    void report(ReportGroup group, std::string title, const Streamable auto&... values) noexcept {
        _groups[std::to_underlying(group)].push_back({std::move(title), format(values...)});
    }

    // Adds a titled value to the ungrouped list
    void report(std::string title, const Streamable auto&... values) noexcept {
        _groups[_groups.size() - 1].push_back({std::move(title), format(values...)});
    }

    // Values reported to a group in the order they were reported
    const std::vector<ReportValue>& getGroup(ReportGroup group) const {
        return _groups[std::to_underlying(group)];
    }

    // Values reported without a group in the order they were reported
    const std::vector<ReportValue>& getGroup() const {
        return _groups[_groups.size() - 1];
    }

    //==================================================

private:

    // Writes one value to the format stream with bools written as True or False
    template <Streamable T>
    void write(const T& value) noexcept {
        if constexpr (std::same_as<T, bool>) {
            _format << (value ? "True" : "False");
        } else {
            _format << value;
        }
    }

    // Clears the format stream and writes every value into it
    std::string format(const Streamable auto&... values) noexcept {
        _format.str(std::string());
        _format.clear();
        (write(values), ...);

        return _format.str();
    }
};

} // namespace mse::component

#endif
