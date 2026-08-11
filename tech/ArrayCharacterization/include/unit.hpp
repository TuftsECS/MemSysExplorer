// This file defines the Unit class which enforces basic rules for physical arithmetic
// Declaring variables as unit types improves readability by informing readers what context variables will be used in
// Variable names then don't need to include their intended unit like "powerOnVoltage"
// Comments at a variable's declaration explaining its unit are not needed since the type explicitly says the unit
// Also, using a unit type includes extra information about variables which the compiler and programmer can use to improve debugging
//
// Basic unit rules are defined in the type system for the Unit class
// Different units can't be summed together (1 farad + 1 meter = ?), but similar units can be
// Any combination of different units can be multiplied together to produce a new compounded unit
// Each individual unit in a compound unit (and single units by themselves) has its own exponent
// Normally units can't morph into others, but exceptions can be defined when it is physically accurate
//   For example, a unit of amperes * ohms is expected to convert to volts, following V = IR, so this exception can be manually defined
// In this file there are also custom literals that convert to units automatically like 5.5_V for 5.5 volts or 10_msec for 10 milliseconds
//
// Most importantly, C++ primitives like float and double will NOT implicitly convert to Unit types
// Unit objects can only be used with other Unit objects, so moving values to/from a unit context is explicit and purposeful

#ifndef MSE_UNIT_HPP
#define MSE_UNIT_HPP

#include <cmath>
#include <string>
#include <format>
#include <iostream>

namespace mse::unit {

//==================================================
// Unique classes of units used by the compiler to distinguish unit types
// Units with different UnitType values wont mix

enum class UnitType {
    Time,
    Distance,
    Feature,
    Voltage,
    Resistance,
    Capacitance,
    Current,
    Conductance,
    Energy,
    Power,
    Temperature
};

//==================================================
// A basic building block of a Unit type
// Compound units are made of multiple UnitAtom types (singular units are represented with a single UnitAtom)
// The UnitAtom captures a UnitType and its associated exponent like meters^3 or volts^2
// The UnitAtomType concept is used to enforce UnitAtom types are passed to templates below

template <UnitType Type, int Exponent>
struct UnitAtom {
    static constexpr UnitType type = Type;
    static constexpr int exponent = Exponent;

    static constexpr const char* unitString() noexcept {
        if constexpr (type == UnitType::Time) {
            return "s";
        } else if constexpr (type == UnitType::Distance) {
            return "m";
        } else if constexpr (type == UnitType::Feature) {
            return "feat";
        } else if constexpr (type == UnitType::Voltage) {
            return "V";
        } else if constexpr (type == UnitType::Resistance) {
            return "Ohm";
        } else if constexpr (type == UnitType::Capacitance) {
            return "F";
        } else if constexpr (type == UnitType::Current) {
            return "A";
        } else if constexpr (type == UnitType::Conductance) {
            return "S";
        } else if constexpr (type == UnitType::Energy) {
            return "J";
        } else if constexpr (type == UnitType::Power) {
            return "W";
        } else if constexpr (type == UnitType::Temperature) {
            return "degC";
        } else {
            static_assert(false);
        }
    }
};

template <typename T>
concept UnitAtomType = requires {
    { T::type } -> std::convertible_to<UnitType>;
    { T::exponent } -> std::convertible_to<int>;
};

//==================================================
// Forward declaration for struct holding conversion rules, more information below

template <typename>
struct UnitConversion;

//==================================================
// The actual Unit class which wraps a single value and enforces the strict arithmetic rules between other Unit objects
// Parameterized by a template parameter pack Atoms... of UnitAtom types

template <UnitAtomType... Atoms>
class Unit {
    
    // Helper structs required by the Unit class are kept private
    // Some functions require accessing other Units' structs,
    // So this friend declaration lets all Unit types share with each other
    template <UnitAtomType...>
    friend class Unit;

public:

    using UnitValueType = double; // Type that each Unit wraps and naturally operates on

    // Explicit constructors prevent implicit conversion from other types
    explicit constexpr Unit() noexcept : _value(0) {}
    explicit constexpr Unit(UnitValueType value) noexcept : _value(value) {}

    // A special implicit constructor just for unitless/Number values
    constexpr Unit(UnitValueType value) noexcept requires(sizeof...(Atoms) == 0) : _value(value) {}

private:

    // The single wrapped value
    UnitValueType _value;

private:

    // Returns true if the atom T is found in this Unit type
    template <UnitAtomType T>
    static consteval bool containsAtom() noexcept {
        return (std::is_same_v<Atoms, T> || ...);
    }

    // Returns true if all atoms in Ts are in this Unit type AND all atoms in this type are in Ts (set equality)
    // Special because Unit types with the same UnitAtom types but different ordering compare equal
    template <UnitAtomType... Ts>
    static consteval bool containsEquivalentAtoms() noexcept {
        return (containsAtom<Ts>() && ...) && (Unit<Ts...>::template containsAtom<Atoms>() && ...);
    }

    template <UnitAtomType... Ts>
    static consteval bool containsEquivalentAtoms(Unit<Ts...>) noexcept {
        return (containsAtom<Ts>() && ...) && (Unit<Ts...>::template containsAtom<Atoms>() && ...);
    }

    //==================================================
    //
    // DO NOT TRY TO INTERPRET THESE STRUCT DEFINITIONS!!! JUST READ THEIR COMMENTS!!! SAVE YOURSELF TIME!!!
    // These structs use recursive partial template specialization tricks to let the Unit type be compile-time friendly
    //
    //==================================================

    // Appends a UnitAtom type to the end of a Unit's Atoms... list
    // Use like typename MergeSingleAtom<Unit<*empty*>, Unit<*existing atoms*>, UnitAtomToMerge>::type
    template <typename, typename, typename>
    struct MergeSingleAtom;

    template <UnitAtomType... Ts, UnitAtomType U, UnitAtomType... Us, UnitAtomType V>
    requires (U::type == V::type && U::exponent != -V::exponent)
    struct MergeSingleAtom<Unit<Ts...>, Unit<U, Us...>, V> {
        using type = Unit<Ts..., UnitAtom<U::type, U::exponent + V::exponent>, Us...>;
    };

    template <UnitAtomType... Ts, UnitAtomType U, UnitAtomType... Us, UnitAtomType V>
    requires (U::type == V::type && U::exponent == -V::exponent)
    struct MergeSingleAtom<Unit<Ts...>, Unit<U, Us...>, V> {
        using type = Unit<Ts..., Us...>;
    };

    template <UnitAtomType... Ts, UnitAtomType U, UnitAtomType... Us, UnitAtomType V>
    requires (U::type != V::type)
    struct MergeSingleAtom<Unit<Ts...>, Unit<U, Us...>, V> {
        using type = MergeSingleAtom<Unit<Ts..., U>, Unit<Us...>, V>::type;
    };

    template <UnitAtomType... Ts, UnitAtomType V>
    struct MergeSingleAtom<Unit<Ts...>, Unit<>, V> {
        using type = Unit<Ts..., V>;
    };

    // Appends a list of UnitAtom types to the end of a Unit's Atoms... list
    // Use like typename MergeManyAtoms<Unit<*existing atoms*>, Unit<*atoms to append*>>::type
    template <typename, typename>
    struct MergeManyAtoms;

    template <UnitAtomType... Ts, UnitAtomType U, UnitAtomType... Us>
    struct MergeManyAtoms<Unit<Ts...>, Unit<U, Us...>> {
        using type = MergeManyAtoms<typename MergeSingleAtom<Unit<>, Unit<Ts...>, U>::type, Unit<Us...>>::type;
    };

    template <UnitAtomType... Ts>
    struct MergeManyAtoms<Unit<Ts...>, Unit<>> {
        using type = Unit<Ts...>;
    };

    // Gets a Unit and its Atoms... list but with each exponent negated
    // Use like typename ReciprocalAtoms<Unit<*empty*>, Unit<*existing atoms*>>::type
    template <typename, typename>
    struct ReciprocalAtoms;

    template <UnitAtomType... Ts, UnitAtomType U, UnitAtomType... Us>
    struct ReciprocalAtoms<Unit<Ts...>, Unit<U, Us...>> {
        using type = ReciprocalAtoms<Unit<Ts..., UnitAtom<U::type, -U::exponent>>, Unit<Us...>>::type;
    };

    template <UnitAtomType... Ts>
    struct ReciprocalAtoms<Unit<Ts...>, Unit<>> {
        using type = Unit<Ts...>;
    };

    // Gets all the UnitAtom types in this Unit's Atoms... list that are also in another Unit's list
    // Use like typename CommonAtoms<Unit<*empty*>, Unit<*existing atoms*>>::type
    template <typename, typename>
    struct CommonAtoms;

    template <UnitAtomType... Ts, UnitAtomType U, UnitAtomType... Us>
    requires (containsAtom<U>())
    struct CommonAtoms<Unit<Ts...>, Unit<U, Us...>> {
        using type = CommonAtoms<Unit<Ts..., U>, Unit<Us...>>::type;
    };

    template <UnitAtomType... Ts, UnitAtomType U, UnitAtomType... Us>
    requires (!containsAtom<U>())
    struct CommonAtoms<Unit<Ts...>, Unit<U, Us...>> {
        using type = CommonAtoms<Unit<Ts...>, Unit<Us...>>::type;
    };

    template <UnitAtomType... Ts>
    struct CommonAtoms<Unit<Ts...>, Unit<>> {
        using type = Unit<Ts...>;
    };

    // Opposite of CommonAtoms, gets all the UnitAtom types in this Unit's Atoms... list which are NOT in another Unit's list
    // Use like DifferentAtomsType<*other atoms*>
    template <UnitAtomType... Ts>
    using DifferentAtomsType = MergeManyAtoms<Unit, typename ReciprocalAtoms<Unit<>, typename CommonAtoms<Unit<>, Unit<Ts...>>::type>::type>::type;

    // Returns the UnitAtom at index I in a Unit's Atoms... list
    // Use like typename AtomAt<Unit<*atoms to index*>, index>::type
    template <typename, int>
    struct AtomAt;

    template <UnitAtomType T, UnitAtomType... Ts, int I>
    struct AtomAt<Unit<T, Ts...>, I> {
        using type = AtomAt<Unit<Ts...>, I - 1>::type;
    };

    template <UnitAtomType T, UnitAtomType... Ts>
    struct AtomAt<Unit<T, Ts...>, 0> {
        using type = T;
    };

    template <int I>
    struct AtomAt<Unit<>, I> {
        using type = void;
    };

    //==================================================
    //
    // End of evil structs
    //
    //==================================================

public:

    //==================================================
    // Unit X Unit operations
    //
    // Unit + Unit, Unit += Unit
    // Unit - Unit, Unit -= Unit
    // Unit * Unit
    // Unit / Unit

    template <UnitAtomType... Ts>
    requires (containsEquivalentAtoms<Ts...>())
    constexpr Unit operator+(Unit<Ts...> u) const noexcept {
        return Unit(_value + u._value);
    }

    template <UnitAtomType... Ts>
    requires (containsEquivalentAtoms<Ts...>())
    constexpr Unit& operator+=(Unit<Ts...> u) noexcept {
        _value += u._value;
        return *this;
    }

    template <UnitAtomType... Ts>
    requires (containsEquivalentAtoms<Ts...>())
    constexpr Unit operator-(Unit<Ts...> u) const noexcept {
        return Unit(_value - u._value);
    }

    template <UnitAtomType... Ts>
    requires (containsEquivalentAtoms<Ts...>())
    constexpr Unit& operator-=(Unit<Ts...> u) noexcept {
        _value -= u._value;
        return *this;
    }

    template <UnitAtomType... Ts>
    constexpr auto operator*(Unit<Ts...> u) const noexcept {
        return typename MergeManyAtoms<Unit, Unit<Ts...>>::type(_value * u._value);
    }

    template <UnitAtomType... Ts>
    constexpr auto operator/(Unit<Ts...> u) const noexcept {
        return typename MergeManyAtoms<Unit, typename ReciprocalAtoms<Unit<>, Unit<Ts...>>::type>::type(_value / u._value);
    }

    //==================================================
    // Unit X primitive operations
    //
    // Unit * primitive, Unit *= primitive
    // Unit / primitive, Unit /= primitive

    constexpr Unit operator*(UnitValueType v) const noexcept {
        return Unit(_value * v);
    }

    constexpr Unit& operator*=(UnitValueType v) noexcept {
        _value *= v;
        return *this;
    }

    constexpr Unit operator/(UnitValueType v) const noexcept {
        return Unit(_value / v);
    }

    constexpr Unit& operator/=(UnitValueType v) noexcept {
        _value /= v;
        return *this;
    }

    //==================================================
    // Primitive X Unit operations
    //
    // Primitive * Unit
    // Primitive / Unit

    friend constexpr Unit operator*(UnitValueType v, Unit u) noexcept {
        return Unit(v * u._value);
    }

    friend constexpr auto operator/(UnitValueType v, Unit u) noexcept {
        return typename ReciprocalAtoms<Unit<>, Unit>::type(v / u._value);
    }

    //==================================================
    // Conversion operators to other Unit types
    // The first one is for Unit types with the same UnitAtom types, but could be ordered differently
    //   Either way, this conversion is clean and the converting type is returned because the units are practically equal
    // The second one is for Unit types with differing UnitAtom types
    //   Common types are removed from the original type and converting type, then a conversion rule is searched on the differing types only
    //   Only works if a conversion rule defined with a UnitConversion struct matches the different types
    //   This indicates a rule from the remainder of the current type exists to the remainder of the converting type
    //   The result is the full converting type

    template <UnitAtomType... Ts>
    requires (containsEquivalentAtoms<Ts...>())
    constexpr operator Unit<Ts...>() const noexcept {
        return Unit<Ts...>(_value);
    }

    template <UnitAtomType... Ts>
    requires (!containsEquivalentAtoms<Ts...>()) &&
             (UnitConversion<DifferentAtomsType<Ts...>>::type::containsEquivalentAtoms(typename Unit<Ts...>::template DifferentAtomsType<Atoms...>{}))
    constexpr operator Unit<Ts...>() const noexcept {
        return Unit<Ts...>(_value);
    }

    //==================================================
    // Conversion operators and a function to decay into the underlying value
    // Casting to the underlying type is allowed only in explicit contexts like static_cast<UnitValueType>(unit)
    // But implicit casting is allowed whenever the Unit type is "unitless" with an empty UnitAtom list (Unit<>)
    //   This makes it easy to write things like ratios as double R = metersA / metersB if metersA and metersB are of the same Unit type
    // Implicit casting is also allowed whenever a conversion rule exists and results in an "unitless" type
    // A basic value() function returns a copy of the wrapped value for read-only access

    explicit constexpr operator UnitValueType() const noexcept {
        return _value;
    }

    constexpr operator UnitValueType() const noexcept requires (sizeof...(Atoms) == 0) {
        return _value;
    }

    constexpr operator UnitValueType() const noexcept
    requires std::same_as<typename UnitConversion<Unit<Atoms...>>::type, Unit<>> {
        return _value;
    }

    constexpr UnitValueType value() const noexcept {
        return _value;
    }

    //==================================================
    // The spaceship operator automatically defines all comparison operators so Unit types can be compared naturally

    constexpr auto operator<=>(const Unit&) const noexcept = default;

    //==================================================
    // For feature sizes, it is common to convert them to meters for an absolute physical size
    // Unit types with just a single UnitAtom holding a UnitType::Feature type are given the below function
    // This makes it simple and clear that a feature is being transformed to meters with another Unit type holding the feature size
    // Use like featureLength.toMeters(featureSize)
    // This function also accounts for exponents in the feature Unit type (so square features go to square meters)

    constexpr auto toMeters(Unit<UnitAtom<UnitType::Distance, 1>> featureSizeInMeters) const noexcept
    requires (sizeof...(Atoms) == 1 && AtomAt<Unit, 0>::type::type == UnitType::Feature) {
        // No constexpr std::pow yet so manually do this with a loop
        UnitValueType factor = 1.0;
        for (int i = 0; i < std::abs(AtomAt<Unit, 0>::type::exponent); ++i) {
            factor *= featureSizeInMeters._value;
        }
        if constexpr (AtomAt<Unit, 0>::type::exponent >= 0) {
            return Unit<UnitAtom<UnitType::Distance, AtomAt<Unit, 0>::type::exponent>>(_value * factor);
        } else {
            return Unit<UnitAtom<UnitType::Distance, AtomAt<Unit, 0>::type::exponent>>(_value / factor);
        }
    }

    //==================================================
    // A handy string function that automatically scales and adds units to the unit value for printing
    // If scaleUnit == true and the value is less than 1, the number is adjusted with a scale factor
    //   For example 0.010 volts -> "10.000mV"
    //   And 0.0005 farads -> "500.000uF"
    // Otherwise the true value at a scale of 1 is used
    //   For example 0.0005 volts -> "0.0005V"
    // If the unit is simple and only contains one type
    //   The unit is placed directly aginst the number
    //   No exponent is shown for exponents of 1
    //   For example 10 square meters -> "10m^2"
    // Otherwise if the unit is compound and contains multiple types
    //   The all units are spaced out from each other and the number
    //   An exponent is given to all units even if they have an exponent of 1
    //   For example 0.08 meters per second squared -> 0.08 m^1 s^-2
    //   To keep it simple, no scaling happens for compound units or units with negative exponents

    std::string toString(int precision = 3, bool scaleUnit = true) const {
        std::string num = std::format("{:.{}f}", _value, precision);

        std::string scale;
        // No compound units, no unitless scaling, no scaling for negative exponents
        // TODO add negative exponent scaling for non-compound units
        if constexpr (sizeof...(Atoms) == 1 && AtomAt<Unit, 0>::type::exponent >= 1) {
            if (scaleUnit) {
                constexpr int exponent = AtomAt<Unit, 0>::type::exponent;
                if (_value < std::pow(1e-15, exponent)) {
                    num = std::format("{:.{}f}", _value * std::pow(1e18, exponent), precision);
                    scale = "a";
                } else if (_value < std::pow(1e-12, exponent)) {
                    num = std::format("{:.{}f}", _value * std::pow(1e15, exponent), precision);
                    scale = "f";
                } else if (_value < std::pow(1e-9, exponent)) {
                    num = std::format("{:.{}f}", _value * std::pow(1e12, exponent), precision);
                    scale = "p";
                } else if (_value < std::pow(1e-6, exponent)) {
                    num = std::format("{:.{}f}", _value * std::pow(1e9, exponent), precision);
                    scale = "n";
                } else if (_value < std::pow(1e-3, exponent)) {
                    num = std::format("{:.{}f}", _value * std::pow(1e6, exponent), precision);
                    scale = "u";
                } else if (_value < 1) {
                    num = std::format("{:.{}f}", _value * std::pow(1e3, exponent), precision);
                    scale = "m";
                }
            }
        }

        constexpr std::array<const char*, sizeof...(Atoms)> units{ Atoms::unitString()... };
        constexpr std::array<int, sizeof...(Atoms)> exponents{ Atoms::exponent... };

        std::string allUnits;
        for (size_t i = 0; i < sizeof...(Atoms); ++i) {
            if constexpr (sizeof...(Atoms) > 1) {
                allUnits += " ";
            }
            allUnits += units[i];
            if constexpr (sizeof...(Atoms) != 1 || exponents[0] != 1) {
                allUnits += "^" + std::to_string(exponents[i]);
            }
        }

        return num + scale + allUnits;
    }

    // Simple operator overload to let units implicitly convert to strings
    operator std::string() const { return toString(); }
};

// Custom stream insertion operator to print units naturally
template<typename... Ts>
std::ostream& operator<<(std::ostream& stream, Unit<Ts...> u) {
    stream << u.toString();
    return stream;
}

//==================================================
// Shorthand types for writing each UnitType in a UnitAtom to make compound units

template <int Exponent = 1>
using TimeAtom = UnitAtom<UnitType::Time, Exponent>;
template <int Exponent = 1>
using SecondAtom = TimeAtom<Exponent>;

template <int Exponent = 1>
using DistanceAtom = UnitAtom<UnitType::Distance, Exponent>;
template <int Exponent = 1>
using MeterAtom = DistanceAtom<Exponent>;

template <int Exponent = 1>
using FeatureAtom = UnitAtom<UnitType::Feature, Exponent>;

template <int Exponent = 1>
using VoltageAtom = UnitAtom<UnitType::Voltage, Exponent>;
template <int Exponent = 1>
using VoltAtom = VoltageAtom<Exponent>;

template <int Exponent = 1>
using ResistanceAtom = UnitAtom<UnitType::Resistance, Exponent>;
template <int Exponent = 1>
using OhmAtom = ResistanceAtom<Exponent>;

template <int Exponent = 1>
using CapacitanceAtom = UnitAtom<UnitType::Capacitance, Exponent>;
template <int Exponent = 1>
using FaradAtom = CapacitanceAtom<Exponent>;

template <int Exponent = 1>
using CurrentAtom = UnitAtom<UnitType::Current, Exponent>;
template <int Exponent = 1>
using AmpereAtom = CurrentAtom<Exponent>;

template <int Exponent = 1>
using ConductanceAtom = UnitAtom<UnitType::Conductance, Exponent>;
template <int Exponent = 1>
using SiemensAtom = ConductanceAtom<Exponent>;

template <int Exponent = 1>
using EnergyAtom = UnitAtom<UnitType::Energy, Exponent>;
template <int Exponent = 1>
using JouleAtom = EnergyAtom<Exponent>;

template <int Exponent = 1>
using PowerAtom = UnitAtom<UnitType::Power, Exponent>;
template <int Exponent = 1>
using WattAtom = PowerAtom<Exponent>;

template <int Exponent = 1>
using TemperatureAtom = UnitAtom<UnitType::Temperature, Exponent>;
template <int Exponent = 1>
using CelsiusAtom = TemperatureAtom<Exponent>;

//==================================================
// Shorthand types for common units to use instead of writing out Unit<...>

using Number = Unit<>::UnitValueType;

using Second = Unit<TimeAtom<>>;
template <int Exponent>
using SecondExp = Unit<TimeAtom<Exponent>>;

using Meter = Unit<DistanceAtom<>>;
template <int Exponent>
using MeterExp = Unit<DistanceAtom<Exponent>>;

using Feature = Unit<FeatureAtom<>>;
template <int Exponent>
using FeatureExp = Unit<FeatureAtom<Exponent>>;

using Volt = Unit<VoltageAtom<>>;
template <int Exponent>
using VoltExp = Unit<VoltageAtom<Exponent>>;

using Ohm = Unit<ResistanceAtom<>>;
template <int Exponent>
using OhmExp = Unit<ResistanceAtom<Exponent>>;

using Farad = Unit<CapacitanceAtom<>>;
template <int Exponent>
using FaradExp = Unit<CapacitanceAtom<Exponent>>;

using Ampere = Unit<CurrentAtom<>>;
template <int Exponent>
using AmpereExp = Unit<CurrentAtom<Exponent>>;

using Siemens = Unit<ConductanceAtom<>>;
template <int Exponent>
using SiemensExp = Unit<ConductanceAtom<Exponent>>;

using Joule = Unit<EnergyAtom<>>;
template <int Exponent>
using JouleExp = Unit<EnergyAtom<Exponent>>;

using Watt = Unit<PowerAtom<>>;
template <int Exponent>
using WattExp = Unit<PowerAtom<Exponent>>;

using Celsius = Unit<TemperatureAtom<>>;
template <int Exponent>
using CelsiusExp = Unit<TemperatureAtom<Exponent>>;

//==================================================
// Conversion rules for units to allow basic physical equivalencies to exist in Unit operations
// So you can naturally write conversions like Volt v = Ampere(...) * Ohm(...) without needing .value() to arbitrate
// Exponents must match exactly because multistep conversion of type and exponent at the same time is not possible
//   Defining resistance^E * conductance^E = none will not allow resistance^2 * conductance^1 = resistance^1
//   Conversions only happen on the final assignment to a new unit
//   So the remainder types would be resistance^2, conductance^1 into resistance^1 which has no match
// To add a new conversion rule, define a partial or full specialization of struct UnitConversion
//   The first and only template parameter is the Unit being converted
//   Inside the struct, declare a type named "type" that is the equivalent Unit type to convert to
//   Declaring the type inside the struct instead of as a second template parameter allows for conversion to Unit types with different UnitAtom orders
// Unfortunately, because of how the type system works, one specialization must be provided for each possible ordering of atoms
// Also, conversions are one-way, so reciprocal definitions need two specializations for A -> B and B -> A
// TODO see if there is a better, more compact way to do this without using helper macros

// For Atom1 = 1 / Atom2 relationships
// Unit<Atom1<E>> -> Unit<Atom2<-E>>
// Unit<Atom2<E>> -> Unit<Atom1<-E>>
// Unit<Atom1<E>, Atom2<E>> -> Unit<>
// Unit<Atom2<E>, Atom1<E>> -> Unit<>
#define UnitConversionReciprocalAtoms(Atom1, Atom2) \
template <int Exponent> struct UnitConversion<Unit<Atom1<Exponent>>> { \
    using type = Unit<Atom2<-Exponent>>; \
}; \
template <int Exponent> struct UnitConversion<Unit<Atom2<Exponent>>> { \
    using type = Unit<Atom1<-Exponent>>; \
}; \
template <int Exponent> struct UnitConversion<Unit<Atom1<Exponent>, Atom2<Exponent>>> { \
    using type = Unit<>; \
}; \
template <int Exponent> struct UnitConversion<Unit<Atom2<Exponent>, Atom1<Exponent>>> { \
    using type = Unit<>; \
}; \

// For Atom1 * Atom2 = ResultAtom relationships
// Unit<Atom1<E>, Atom2<E>> -> Unit<ResultAtom<E>>
// Unit<Atom2<E>, Atom1<E>> -> Unit<ResultAtom<E>>
#define UnitConversion2Atoms(Atom1, Atom2, ResultAtom) \
template <int Exponent> struct UnitConversion<Unit<Atom1<Exponent>, Atom2<Exponent>>> { \
    using type = Unit<ResultAtom<Exponent>>; \
}; \
template <int Exponent> struct UnitConversion<Unit<Atom2<Exponent>, Atom1<Exponent>>> { \
    using type = Unit<ResultAtom<Exponent>>; \
};

UnitConversionReciprocalAtoms(ResistanceAtom, ConductanceAtom)

UnitConversion2Atoms(ResistanceAtom, CapacitanceAtom, TimeAtom)
UnitConversion2Atoms(CurrentAtom, ResistanceAtom, VoltageAtom)

#undef UnitConversionReciprocalAtoms
#undef UnitConversion2Atoms

//==================================================
// Literal value definitions for easy ways to write out constant unit values
// Instead of writing Volt(1.5), these literals allow 1.5_V instead for better readability
//
// Seconds:
//   _s
//   _ms
//   _us
//   _ps
// Meters:
//   _m
//   _mm
//   _um
//   _nm
// Features:
//   _feat
// Volts:
//   _V
//   _mV
//   _uV
//   _nV
// Ohms:
//   _Ohm
//   _mOhm
//   _uOhm
//   _nOhm
// Farads:
//   _F
//   _mF
//   _uF
//   _nF
//   _pF
//   _fF
// Amperes:
//   _A
//   _mA
//   _uA
//   _nA
// Siemens:
//   _S
//   _mS
//   _uS
//   _nS
// Joules:
//   _J
//   _mJ
//   _uJ
//   _nJ
// Watts:
//   _W
//   _mW
//   _uW
//   _nW
// Celsius:
//   _degC

constexpr Second operator""_s(long double value) noexcept {
    return Second(value);
}
constexpr Second operator""_s(unsigned long long int value) noexcept {
    return Second(value);
}
constexpr Second operator""_ms(long double value) noexcept {
    return Second(value * 1e-3);
}
constexpr Second operator""_ms(unsigned long long int value) noexcept {
    return Second(value * 1e-3);
}
constexpr Second operator""_us(long double value) noexcept {
    return Second(value * 1e-6);
}
constexpr Second operator""_us(unsigned long long int value) noexcept {
    return Second(value * 1e-6);
}
constexpr Second operator""_ns(long double value) noexcept {
    return Second(value * 1e-9);
}
constexpr Second operator""_ns(unsigned long long int value) noexcept {
    return Second(value * 1e-9);
}
constexpr Second operator""_ps(long double value) noexcept {
    return Second(value * 1e-12);
}
constexpr Second operator""_ps(unsigned long long int value) noexcept {
    return Second(value * 1e-12);
}

constexpr Meter operator""_m(long double value) noexcept {
    return Meter(value);
}
constexpr Meter operator""_m(unsigned long long int value) noexcept {
    return Meter(value);
}
constexpr Meter operator""_mm(long double value) noexcept {
    return Meter(value * 1e-3);
}
constexpr Meter operator""_mm(unsigned long long int value) noexcept {
    return Meter(value * 1e-3);
}
constexpr Meter operator""_um(long double value) noexcept {
    return Meter(value * 1e-6);
}
constexpr Meter operator""_um(unsigned long long int value) noexcept {
    return Meter(value * 1e-6);
}
constexpr Meter operator""_nm(long double value) noexcept {
    return Meter(value * 1e-9);
}
constexpr Meter operator""_nm(unsigned long long int value) noexcept {
    return Meter(value * 1e-9);
}

constexpr Feature operator""_feat(long double value) noexcept {
    return Feature(value);
}
constexpr Feature operator""_feat(unsigned long long int value) noexcept {
    return Feature(value);
}

constexpr Volt operator""_V(long double value) noexcept {
    return Volt(value);
}
constexpr Volt operator""_V(unsigned long long int value) noexcept {
    return Volt(value);
}
constexpr Volt operator""_mV(long double value) noexcept {
    return Volt(value * 1e-3);
}
constexpr Volt operator""_mV(unsigned long long int value) noexcept {
    return Volt(value * 1e-3);
}
constexpr Volt operator""_uV(long double value) noexcept {
    return Volt(value * 1e-6);
}
constexpr Volt operator""_uV(unsigned long long int value) noexcept {
    return Volt(value * 1e-6);
}
constexpr Volt operator""_nV(long double value) noexcept {
    return Volt(value * 1e-9);
}
constexpr Volt operator""_nV(unsigned long long int value) noexcept {
    return Volt(value * 1e-9);
}

constexpr Ohm operator""_Ohm(long double value) noexcept {
    return Ohm(value);
}
constexpr Ohm operator""_Ohm(unsigned long long int value) noexcept {
    return Ohm(value);
}
constexpr Ohm operator""_mOhm(long double value) noexcept {
    return Ohm(value * 1e-3);
}
constexpr Ohm operator""_mOhm(unsigned long long int value) noexcept {
    return Ohm(value * 1e-3);
}
constexpr Ohm operator""_uOhm(long double value) noexcept {
    return Ohm(value * 1e-6);
}
constexpr Ohm operator""_uOhm(unsigned long long int value) noexcept {
    return Ohm(value * 1e-6);
}
constexpr Ohm operator""_nOhm(long double value) noexcept {
    return Ohm(value * 1e-9);
}
constexpr Ohm operator""_nOhm(unsigned long long int value) noexcept {
    return Ohm(value * 1e-9);
}

constexpr Farad operator""_F(long double value) noexcept {
    return Farad(value);
}
constexpr Farad operator""_F(unsigned long long int value) noexcept {
    return Farad(value);
}
constexpr Farad operator""_mF(long double value) noexcept {
    return Farad(value * 1e-3);
}
constexpr Farad operator""_mF(unsigned long long int value) noexcept {
    return Farad(value * 1e-3);
}
constexpr Farad operator""_uF(long double value) noexcept {
    return Farad(value * 1e-6);
}
constexpr Farad operator""_uF(unsigned long long int value) noexcept {
    return Farad(value * 1e-6);
}
constexpr Farad operator""_nF(long double value) noexcept {
    return Farad(value * 1e-9);
}
constexpr Farad operator""_nF(unsigned long long int value) noexcept {
    return Farad(value * 1e-9);
}
constexpr Farad operator""_pF(long double value) noexcept {
    return Farad(value * 1e-12);
}
constexpr Farad operator""_pF(unsigned long long int value) noexcept {
    return Farad(value * 1e-12);
}
constexpr Farad operator""_fF(long double value) noexcept {
    return Farad(value * 1e-15);
}
constexpr Farad operator""_fF(unsigned long long int value) noexcept {
    return Farad(value * 1e-15);
}

constexpr Ampere operator""_A(long double value) noexcept {
    return Ampere(value);
}
constexpr Ampere operator""_A(unsigned long long int value) noexcept {
    return Ampere(value);
}
constexpr Ampere operator""_mA(long double value) noexcept {
    return Ampere(value * 1e-3);
}
constexpr Ampere operator""_mA(unsigned long long int value) noexcept {
    return Ampere(value * 1e-3);
}
constexpr Ampere operator""_uA(long double value) noexcept {
    return Ampere(value * 1e-6);
}
constexpr Ampere operator""_uA(unsigned long long int value) noexcept {
    return Ampere(value * 1e-6);
}
constexpr Ampere operator""_nA(long double value) noexcept {
    return Ampere(value * 1e-9);
}
constexpr Ampere operator""_nA(unsigned long long int value) noexcept {
    return Ampere(value * 1e-9);
}

constexpr Siemens operator""_S(long double value) noexcept {
    return Siemens(value);
}
constexpr Siemens operator""_S(unsigned long long int value) noexcept {
    return Siemens(value);
}
constexpr Siemens operator""_mS(long double value) noexcept {
    return Siemens(value * 1e-3);
}
constexpr Siemens operator""_mS(unsigned long long int value) noexcept {
    return Siemens(value * 1e-3);
}
constexpr Siemens operator""_uS(long double value) noexcept {
    return Siemens(value * 1e-6);
}
constexpr Siemens operator""_uS(unsigned long long int value) noexcept {
    return Siemens(value * 1e-6);
}
constexpr Siemens operator""_nS(long double value) noexcept {
    return Siemens(value * 1e-9);
}
constexpr Siemens operator""_nS(unsigned long long int value) noexcept {
    return Siemens(value * 1e-9);
}

constexpr Joule operator""_J(long double value) noexcept {
    return Joule(value);
}
constexpr Joule operator""_J(unsigned long long int value) noexcept {
    return Joule(value);
}
constexpr Joule operator""_mJ(long double value) noexcept {
    return Joule(value * 1e-3);
}
constexpr Joule operator""_mJ(unsigned long long int value) noexcept {
    return Joule(value * 1e-3);
}
constexpr Joule operator""_uJ(long double value) noexcept {
    return Joule(value * 1e-6);
}
constexpr Joule operator""_uJ(unsigned long long int value) noexcept {
    return Joule(value * 1e-6);
}
constexpr Joule operator""_nJ(long double value) noexcept {
    return Joule(value * 1e-9);
}
constexpr Joule operator""_nJ(unsigned long long int value) noexcept {
    return Joule(value * 1e-9);
}

constexpr Watt operator""_W(long double value) noexcept {
    return Watt(value);
}
constexpr Watt operator""_W(unsigned long long int value) noexcept {
    return Watt(value);
}
constexpr Watt operator""_mW(long double value) noexcept {
    return Watt(value * 1e-3);
}
constexpr Watt operator""_mW(unsigned long long int value) noexcept {
    return Watt(value * 1e-3);
}
constexpr Watt operator""_uW(long double value) noexcept {
    return Watt(value * 1e-6);
}
constexpr Watt operator""_uW(unsigned long long int value) noexcept {
    return Watt(value * 1e-6);
}
constexpr Watt operator""_nW(long double value) noexcept {
    return Watt(value * 1e-9);
}
constexpr Watt operator""_nW(unsigned long long int value) noexcept {
    return Watt(value * 1e-9);
}

constexpr Celsius operator""_degC(long double value) noexcept {
    return Celsius(value);
}
constexpr Celsius operator""_degC(unsigned long long int value) noexcept {
    return Celsius(value);
}

} // namespace mse::unit

#endif
