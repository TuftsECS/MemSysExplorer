// This file defines the Reporter interface which components fill with their values when report(Reporter&) is called on them
// Components report each value with a title and the Reporter formats it to a string right away
// Any type that can be written to an ostream can be reported like unit types and Dimensions
// Several values can be passed in one call and are written back to back
//
// Values are sorted into the groups listed in ReportGroup or into an ungrouped list when no group is given
// Each formatted value goes to the subtype, which decides what to keep:
//   ListReporter keeps every value in order for printing
//   CombiningReporter also combines values with the same title, so several components can report into one
//   ZeroReporter only keeps whether every value was zero
//
// Formatting copies the flags of a base stream so reported values match the rest of the output

#ifndef MSE_COMPONENT_REPORTER_HPP
#define MSE_COMPONENT_REPORTER_HPP

#include <algorithm>
#include <array>
#include <map>
#include <optional>
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

// Unit types that wrap a number and can be rebuilt from it
template <typename T>
concept NumberUnit = requires(const T& value) {
    { value.value() } -> std::same_as<double>;
    T(value.value());
};

class Reporter {
public:

    // Every group paired with its printed name in printing order
    static constexpr std::array<std::pair<ReportGroup, std::string_view>, 3> reportGroups = {{
        {ReportGroup::Dimensions, "Dimensions"},
        {ReportGroup::Timing, "Timing"},
        {ReportGroup::Power, "Power"}
    }};

protected:

    // Index of the ungrouped list, after the groups
    static constexpr std::size_t ungrouped = reportGroups.size();

private:

    // Reused stream for formatting values holding the base stream's flags
    std::ostringstream _format;

public:

    virtual ~Reporter() = default;

    //==================================================

    // Adds a titled value to a group
    void report(ReportGroup group, std::string title, const Streamable auto&... values) noexcept {
        add(std::to_underlying(group), std::move(title), values...);
    }

    // Adds a titled value to the ungrouped list
    void report(std::string title, const Streamable auto&... values) noexcept {
        add(ungrouped, std::move(title), values...);
    }

    //==================================================

protected:

    // Values are formatted with the same flags as baseFormat
    explicit Reporter(const std::ostream& baseFormat) noexcept {
        _format.copyfmt(baseFormat);
    }

    // The number formatted for a single unit value, so a subtype can combine it with an earlier value of the same title
    virtual double combine([[maybe_unused]] std::size_t group, [[maybe_unused]] const std::string& title, double number) noexcept {
        return number;
    }

    // Takes a formatted value, with its number when it is a single unit value
    virtual void take(std::size_t group, std::string title, std::string value, std::optional<double> number) noexcept = 0;

private:

    // A single unit value is formatted from its number after the subtype combines it
    template <Streamable T>
    void add(std::size_t group, std::string title, const T& value) noexcept {
        if constexpr (NumberUnit<T>) {
            const double number = combine(group, title, value.value());
            take(group, std::move(title), format(T(number)), number);
        } else {
            take(group, std::move(title), format(value), std::nullopt);
        }
    }

    template <Streamable... Ts>
    requires (sizeof...(Ts) != 1)
    void add(std::size_t group, std::string title, const Ts&... values) noexcept {
        take(group, std::move(title), format(values...), std::nullopt);
    }

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

//==================================================

// Keeps every reported value in the order it was reported
class ListReporter : public Reporter {
private:

    // One list of values per group, then the ungrouped list
    std::array<std::vector<ReportValue>, ungrouped + 1> _groups;

public:

    explicit ListReporter(const std::ostream& baseFormat) noexcept : Reporter(baseFormat) {}

    // Values reported to a group in the order they were reported
    const std::vector<ReportValue>& getGroup(ReportGroup group) const {
        return _groups[std::to_underlying(group)];
    }

    // Values reported without a group in the order they were reported
    const std::vector<ReportValue>& getGroup() const {
        return _groups[ungrouped];
    }

protected:

    // Values reported to a group, by index
    std::vector<ReportValue>& values(std::size_t group) noexcept {
        return _groups[group];
    }

    void take(std::size_t group, std::string title, std::string value, [[maybe_unused]] std::optional<double> number) noexcept override {
        _groups[group].push_back({std::move(title), std::move(value)});
    }
};

// Combines a unit value with an earlier unit value of the same title, so several components can report into one reporter
// Timing values take the larger one since those parts work at the same time, and other values add up
// Other repeated titles keep the first value
class CombiningReporter final : public ListReporter {
private:

    // The number of each unit value per group by title
    std::array<std::map<std::string, double, std::less<>>, ungrouped + 1> _numbers;

public:

    explicit CombiningReporter(const std::ostream& baseFormat) noexcept : ListReporter(baseFormat) {}

protected:

    double combine(std::size_t group, const std::string& title, double number) noexcept override {
        auto earlier = _numbers[group].find(title);
        if (earlier == _numbers[group].end()) {
            return number;
        }
        if (group == std::to_underlying(ReportGroup::Timing)) {
            return std::max(earlier->second, number);
        }

        return earlier->second + number;
    }

    void take(std::size_t group, std::string title, std::string value, std::optional<double> number) noexcept override {
        auto earlier = std::ranges::find(values(group), title, &ReportValue::title);
        if (earlier == values(group).end()) {
            if (number) {
                _numbers[group][title] = *number;
            }
            ListReporter::take(group, std::move(title), std::move(value), number);
        } else if (number && _numbers[group].contains(title)) {
            _numbers[group][title] = *number;
            earlier->value = std::move(value);
        }
    }
};

// Only keeps whether every reported value is a unit value of zero, which is also true when nothing was reported
class ZeroReporter final : public Reporter {
private:

    bool _allZero = true;

public:

    explicit ZeroReporter(const std::ostream& baseFormat) noexcept : Reporter(baseFormat) {}

    bool allZero() const noexcept {
        return _allZero;
    }

protected:

    void take([[maybe_unused]] std::size_t group, [[maybe_unused]] std::string title, [[maybe_unused]] std::string value, std::optional<double> number) noexcept override {
        _allZero = _allZero && number == 0.0;
    }
};

} // namespace mse::component

#endif
