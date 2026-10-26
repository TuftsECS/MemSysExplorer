// A Component implementation that requires a ramp argument to get timing values
//
// This type of component differs from a BasicComponent in that timing() requires an input ramp to be set
// Before getting timing values, set the input ramp by calling setInputRamp(ramp)
// Each call to setInputRamp resets the timing values too
// If no input ramp was previously set, the timing() call will fatal out
//
// Contains Dimensions, BasicTiming, and BasicPower values at least
// Requires them to be implemented with the calculate* virtual functions
// Also automatically reports these values in a report() override
// When deriving from this type, more values may be appropriate to add to the timing and power values
//   Derive custom timing and power structs from BasicTiming and BasicPower, then feed those types to the template arguments
//   Components on a cell's access path use PathTiming and PathPower instead
//   This will change the base function return types and set up the interface for the more custom types appropriately
//   To add the extra values to reported output, override report() and simply call BasicComponent<..., ...>::report(reporter)
//   Then all the base values are reported in the same style, and the custom ones can be added after

#ifndef MSE_COMPONENT_RAMPEDCOMPONENT_HPP
#define MSE_COMPONENT_RAMPEDCOMPONENT_HPP

#include "component/Component.hpp"
#include "component/BasicValues.hpp"
#include "Dimensions.hpp"
#include "physics/Horowitz.hpp"

#include <concepts>
#include <optional>

namespace mse::component {

struct RampedTiming : BasicTiming {
    physics::HorowitzRamp outputRamp;
};

template <typename Timing = RampedTiming, typename Power = BasicPower>
class RampedComponent : public Component {
public:

    //==================================================

    const Dimensions& dimensions() const noexcept { return getValue(_dimensions); }
    const Power& power() const noexcept { return getValue(_power); }

    // This timing checks for a set input ramp first before accessing the value
    const Timing& timing() const noexcept {
        if (!_inputRamp) {
            fatal("Input ramp not set before timing");
        }
        return getValue(_timing, *_inputRamp);
    }

    // Used to set input ramp value before reading timing()
    void setInputRamp(physics::HorowitzRamp inputRamp) const noexcept {
        _inputRamp = inputRamp;
        _timing.reset();
    }

    //==================================================
    // Standard base reporting function for the dimensions, timing, and power values

    void report(Reporter& reporter) const noexcept override {
        const Dimensions& d = dimensions();
        reporter.report(ReportGroup::Dimensions, "Width", d.width);
        reporter.report(ReportGroup::Dimensions, "Height", d.height);
        reporter.report(ReportGroup::Dimensions, "Area", d.area());

        // Components on a cell access path report their operations themselves
        if constexpr (std::derived_from<Timing, RampedTiming>) {
            const Timing& t = timing();
            reporter.report(ReportGroup::Timing, "Read Latency", t.readLatency);
            reporter.report(ReportGroup::Timing, "Write Latency", t.writeLatency);
            // An infinite ramp means no ramp was worked out
            if (t.outputRamp < physics::infiniteRamp) {
                reporter.report(ReportGroup::Timing, "Output Ramp", t.outputRamp);
            } else {
                reporter.report(ReportGroup::Timing, "Output Ramp", "infinite");
            }
        }

        if constexpr (std::derived_from<Power, BasicPower>) {
            const Power& p = power();
            reporter.report(ReportGroup::Power, "Dynamic Read Energy", p.dynamicReadEnergy);
            reporter.report(ReportGroup::Power, "Dynamic Write Energy", p.dynamicWriteEnergy);
            reporter.report(ReportGroup::Power, "Leakage", p.leakage);
        }
    }

    //==================================================
    // Resets cached lazy values so other arguments may be used for recomputation

    void reset() noexcept override {
        _dimensions.reset();
        _timing.reset();
        _power.reset();
        _inputRamp.reset();
    }

protected:

    virtual Dimensions calculateDimensions() const noexcept = 0;
    virtual Timing calculateTiming(physics::HorowitzRamp inputRamp) const noexcept = 0;
    virtual Power calculatePower() const noexcept = 0;

private:

    Lazy<&RampedComponent::calculateDimensions> _dimensions;
    // Mutable for logical constness since setInputRamp resets the timing through a const owner
    mutable Lazy<&RampedComponent::calculateTiming> _timing;
    Lazy<&RampedComponent::calculatePower> _power;

    mutable std::optional<physics::HorowitzRamp> _inputRamp;
};

} // namespace mse::component

#endif
