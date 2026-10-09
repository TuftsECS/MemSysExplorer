// Wrapper for members in components wanting to do cached, lazy initialization
// Uses NTTP to take a pointer to a member function and calls that on an owner object later
// Deduces stored type by the function pointer return type
// Once the lazy value is computed, it is also cached internally so subsequent calls do not have recomputation
// So the operator() is safe and ensures that the value returned is always valid, computed only when needed, and not recomputed accidentally later
//
// !!!!!
// Arguments are only used by the first call, later calls return the stored value until reset()
// The first call through a const owner writes the value, so concurrent first calls are not thread-safe
// !!!!!
//
// Use like:
//   int CustomComponent::computeMyLazyValue(int arg1, ...);
//   Lazy<&computeMyLazyValue> lazyValue;
// Then every call to lazyValue(Owner owner, ...) will use owner as the object for the member function pointer

#ifndef MSE_COMPONENT_LAZY_HPP
#define MSE_COMPONENT_LAZY_HPP

#include <optional>
#include <utility>

namespace mse::component {

// Specialization lets the compiler extract each part of the function
template <auto Function>
class Lazy;

template <typename Owner, typename T, typename... Args, T (Owner::*Function)(Args...) const noexcept>
class Lazy<Function> {
public:

    // Access underlying value and call initialization function if needed
    const T& operator()(const Owner& owner, Args... args) const noexcept {
        if (!_value) {
            _value = (owner.*Function)(std::forward<Args>(args)...);
        }
        return *_value;
    }

    // Values can also be directly assigned if needed
    Lazy& operator=(const T& value) noexcept {
        _value = value;
        return *this;
    }

    // Resets the internal state so that a new value is computed with possibly differnt arguments
    void reset() noexcept {
        _value.reset();
    }

private:

    // The actual storage for the underlying value
    mutable std::optional<T> _value;
};

} // namespace mse::component

#endif
