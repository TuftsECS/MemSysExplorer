// A simple Component implementation with properties that require no dependencies (arguments)
// Contains Dimensions, BasicTiming, and BasicPower values at least
// Requires them to be implemented with the calculate* virtual functions
// Also automatically reports these values in a report() override
// When deriving from this type, more values may be appropriate to add to the timing and power values
//   Derive custom timing and power structs from BasicTiming and BasicPower, then feed those types to the template arguments
//   This will change the base function return types and set up the interface for the more custom types appropriately
//   To add the extra values to reported output, override report() and simply call BasicComponent<..., ...>::report(reporter)
//   Then all the base values are reported in the same style, and the custom ones can be added after

#ifndef MSE_COMPONENT_BASICCOMPONENT_HPP
#define MSE_COMPONENT_BASICCOMPONENT_HPP

#include "component/Component.hpp"
#include "component/BasicValues.hpp"
#include "Dimensions.hpp"

#include <optional>

namespace mse::component {

template <std::derived_from<BasicTiming> Timing = BasicTiming,
          std::derived_from<BasicPower> Power = BasicPower>
class BasicComponent : public Component {
public:

    //==================================================

    const Dimensions& dimensions() const noexcept { return getValue(_dimensions); }
    const Timing& timing() const noexcept { return getValue(_timing); }
    const Power& power() const noexcept { return getValue(_power); }

    //==================================================
    // Standard base reporting function for the dimensions, timing, and power values

    void report(Reporter& reporter) const noexcept override {
        const Dimensions& d = dimensions();
        reporter.report(ReportGroup::Dimensions, "Width", d.width);
        reporter.report(ReportGroup::Dimensions, "Height", d.height);
        reporter.report(ReportGroup::Dimensions, "Area", d.area());

        const Timing& t = timing();
        reporter.report(ReportGroup::Timing, "Read Latency", t.readLatency);
        reporter.report(ReportGroup::Timing, "Write Latency", t.writeLatency);

        const Power& p = power();
        reporter.report(ReportGroup::Power, "Dynamic Read Energy", p.dynamicReadEnergy);
        reporter.report(ReportGroup::Power, "Dynamic Write Energy", p.dynamicWriteEnergy);
        reporter.report(ReportGroup::Power, "Leakage", p.leakage);
    }

    //==================================================
    // Resets cached lazy values so other arguments may be used for recomputation

    void reset() noexcept override {
        _dimensions.reset();
        _timing.reset();
        _power.reset();
    }

protected:

    virtual Dimensions calculateDimensions() const noexcept = 0;
    virtual Timing calculateTiming() const noexcept = 0;
    virtual Power calculatePower() const noexcept = 0;

private:

    Lazy<&BasicComponent::calculateDimensions> _dimensions;
    Lazy<&BasicComponent::calculateTiming> _timing;
    Lazy<&BasicComponent::calculatePower> _power;
};

} // namespace mse::component

#endif
