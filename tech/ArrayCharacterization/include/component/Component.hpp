// The base Component interface that all components derive from and implement
// Designed to be used like a tree of components

#ifndef MSE_COMPONENT_COMPONENT_HPP
#define MSE_COMPONENT_COMPONENT_HPP

#include "unit.hpp"
#include "constant.hpp"
#include "Dimensions.hpp"
#include "log.hpp"
#include "utility.hpp"
#include "component/Lazy.hpp"
#include "component/Reporter.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <concepts>
#include <optional>
#include <functional>
#include <string_view>
#include <type_traits>

namespace mse::component {

//==================================================
// The base Component interface
// Every Component can have an instance name, component type name, subcomponents, and standard setup procedure
// Shadowing, logging functions for components automatically report names for debugging

class Component {
public:

    // Every component can have a unique name
    std::string instanceName;

    virtual ~Component() = default;

    // Every type of component has a static name reported through this function
    virtual constexpr std::string_view componentName() const noexcept = 0;

    // For generalized access and function application to all of a component's subcomponents
    // Without needing knowledge of how many or which subcomponents exist, visit(subcomponent) will be applied to all subcomponents
    using SubcomponentVisitor = std::function<void(Component&)>;
    virtual void forEachSubcomponent(const SubcomponentVisitor& visit) noexcept { (void)visit; }

    // Checks the component's inputs and builds its subcomponents with build() in dependency order, returning if the component is valid
    // Subcomponents are only built here and never in constructors, so an invalid component fails before building any of them
    virtual bool setup() noexcept { return true; }

    // Reporter visiting interface to print component values to output
    virtual void report(Reporter& reporter) const noexcept = 0;

    // Resets any cached values in a component for recomputation of values with potentially different arguments
    virtual void reset() noexcept = 0;

    //==================================================
    // Logging functions for use inside component code
    // Name shadowing lets these calls look like normal log functions, but they prepend the component name to messages

    void debug(std::string_view message) const noexcept {
        log::debug(printName().append(message));
    }

    void warn(std::string_view message) const noexcept {
        log::warn(printName().append(message));
    }

    void error(std::string_view message) const noexcept {
        log::error(printName().append(message));
    }

    [[noreturn]] void fatal(std::string_view message) const noexcept {
        log::fatal(printName().append(message));
    }

    //==================================================

protected:

    Component() = default;
    Component(const Component&) = default;
    Component& operator=(const Component&) = default;
    Component(Component&&) = default;
    Component& operator=(Component&&) = default;

    //==================================================
    // Helper method for derived components using Lazy types to automatically pass their self reference
    // Removes repeated *this arguments in components
    // Use in public access methods like T value() { return getValue(_value); }

    template <typename Self, auto F, typename... Args>
    const auto& getValue(this const Self& self, const Lazy<F>& lazy, Args&&... args) noexcept {
        return lazy(self, std::forward<Args>(args)...);
    }

    // Constructs a subcomponent from args and sets it up, returning if it is valid
    // Args are evaluated before the call, so only read from an earlier subcomponent after its build returned true
    template <std::derived_from<Component> T, typename... Args>
    [[nodiscard]] bool build(T& subcomponent, Args&&... args) noexcept {
        subcomponent = T(std::forward<Args>(args)...);

        return subcomponent.setup();
    }

    // Overload for conditional subcomponents stored in std::optional
    template <std::derived_from<Component> T, typename... Args>
    [[nodiscard]] bool build(std::optional<T>& subcomponent, Args&&... args) noexcept {
        subcomponent.emplace(std::forward<Args>(args)...);

        return subcomponent->setup();
    }

    // Overloads that also give the subcomponent an instance name before it is set up
    template <std::derived_from<Component> T, typename... Args>
    [[nodiscard]] bool build(std::string instanceName, T& subcomponent, Args&&... args) noexcept {
        subcomponent = T(std::forward<Args>(args)...);
        subcomponent.instanceName = std::move(instanceName);

        return subcomponent.setup();
    }

    template <std::derived_from<Component> T, typename... Args>
    [[nodiscard]] bool build(std::string instanceName, std::optional<T>& subcomponent, Args&&... args) noexcept {
        subcomponent.emplace(std::forward<Args>(args)...);
        subcomponent->instanceName = std::move(instanceName);

        return subcomponent->setup();
    }

private:

    // The prepended string with component name info used in the logging functions
    std::string printName() const noexcept {
        if (instanceName != "") {
            return std::format("({} \"{}\") ", componentName(), instanceName);
        } else {
            return std::format("({}) ", componentName());
        }
    }
};

} // namespace mse::component

#endif
