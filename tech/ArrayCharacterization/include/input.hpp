// This file establishes a standard way of passing inputs to the program through YAML files
// Parameter types from ConfigurationParameter are wrappers used to differentiate against normal variables
// The functionality of each parameter type elimintes repetitive file parsing logic
// Parameters have field names corresponding to their YAML input field and printed names for use in output
// Parameters can have default values where applicable so that users can exclude defining some fields
// Parameters also track if they have been assigned a value yet
// This way, standard error reporting when parameters are not found in files (while lacking default values) is easy
// Each parameter can wrap a generic variable type
// Overloads for complex types are possible to instruct parameters on how to interpret YAML field values when reading
//
// A YAML file wrapper class YamlFile owns the opened file contents and fills parameters with found field values
// The fields which are accessed are tracked by YamlFile
// When input is done, the file object can check for "strict access" to ensure no fields in the file were not used
//
// For enum values, the declarations in enumInfo.hpp are referenced for string to enum mappings
// Add a mapping declaration in that file to automatically have it be interpretable as a parameter

#ifndef MSE_INPUT_HH
#define MSE_INPUT_HH

#include "Unit.hpp"
#include "enumInfo.hpp"
#include "log.hpp"
#include "utility.hpp"

#include "yaml-cpp/yaml.h"

#include <string>
#include <exception>
#include <optional>
#include <vector>
#include <tuple>
#include <unordered_set>
#include <functional>

namespace mse::input {

//==================================================
// A base class for all other parameter types
// Every parameter will have a field name for YAML file input
// Parameters can optionally specify a printed name instead of reusing the YAML field name
// If the printed name is set to the empty string "", the field name is used as the printed name
//   This is needed for templated constructors below

class ConfigurationParameter {
public:

    std::string fieldName; // Field string to search for in YAML files
    std::string printName; // Name to use when displaying information about the parameter

    virtual ~ConfigurationParameter() = default;

    virtual bool initialized() const noexcept = 0; // Checks if a parameter has been given a value

protected:

    ConfigurationParameter() noexcept
            : fieldName("NullParameter"),
              printName("NullParameter") {}

    ConfigurationParameter(const std::string& fieldName) noexcept
            : fieldName(fieldName),
              printName(fieldName) {}

    ConfigurationParameter(const std::string& fieldName, const std::string& printName) noexcept
            : fieldName(fieldName),
              printName(printName.empty() ? fieldName : printName) {}
};

//==================================================
// For single-valued parameters like a number, boolean, string...
// Written in YAML as
// FieldName: 10

template <typename T>
class SimpleParameter : public ConfigurationParameter {

    friend class YamlFile;

private:

    std::optional<T> _value;

public:

    SimpleParameter() noexcept {}

    SimpleParameter(const std::string& fieldName) noexcept
            : ConfigurationParameter(fieldName) {}

    SimpleParameter(const std::string& fieldName, const std::string& printName) noexcept
            : ConfigurationParameter(fieldName, printName) {}

    SimpleParameter(const std::string& fieldName, const std::string& printName, const T& defaultValue) noexcept
            : ConfigurationParameter(fieldName, printName),
              _value(defaultValue) {}

    bool initialized() const noexcept override final {
        return static_cast<bool>(_value);
    }

    //==================================================
    // Access the underlying value with
    // T v = parameter;
    // T v = parameter.value();

    operator T&() noexcept {
        return _value.value();
    }

    operator const T&() const noexcept {
        return _value.value();
    }

    auto& value(this auto& self) noexcept {
        return self._value.value();
    }
};

//==================================================
// For arrays of values like a list of temperatures or enum flags
// Written in YAML as
// FieldName:
//   - 1
//   - 2
//   - 3

template <typename T, int C = 0>
class ArrayParameter : public ConfigurationParameter {

    friend class YamlFile;

private:

    std::optional<std::vector<T>> _values;

public:

    ArrayParameter() noexcept {}

    ArrayParameter(const std::string& fieldName) noexcept
            : ConfigurationParameter(fieldName) {}

    ArrayParameter(const std::string& fieldName, const std::string& printName) noexcept
            : ConfigurationParameter(fieldName, printName) {}

    template <typename... Us>
    requires (std::same_as<Us, T> && ...)
    ArrayParameter(const std::string& fieldName, const std::string& printName, const Us&... defaultValues) noexcept
            : ConfigurationParameter(fieldName, printName),
              _values(defaultValues...) {}

    bool initialized() const noexcept override final {
        return static_cast<bool>(_values);
    }

    //==================================================
    // Access the underlying values with
    // T v = parameter[idx];
    // std::vector<T> v = parameter;
    // std::vector<T> v = parameter.values();
    //
    // And get the number of elements in this parameter with
    // size_t l = parameter.length;

    auto& operator[](this auto& self, size_t index) noexcept {
        return self._values.value().at(index);
    }

    operator std::vector<T>&() noexcept {
        return _values.value();
    }

    operator const std::vector<T>&() const noexcept {
        return _values.value();
    }

    auto& values(this auto& self) noexcept {
        return self._values.value();
    }

    size_t length() const noexcept {
        return _values.value().length();
    }

private:

    //==================================================
    // Functions used by the YamlFile class when filling this parameter

    void construct() noexcept {
        _values = std::vector<T>();
    }

    void append(const T& value) noexcept {
        _values.value().push_back(value);
    }

    bool hasExactCount() const noexcept {
        if constexpr (C == 0) {
            return true;
        } else {
            return _values.value().size() == C;
        }
    }
};

//==================================================
// For more complicated parameters requiring a map at the top level
// Could be used for describing a type requiring a string, and int, and a bool
// Written in YAML as
// FieldName:
//   name: Simon Jarrett
//   age: 26
//
// CompoundParameters are built around SimpleParameters and ArrayParameters
// They are meant for building other types and must be provided with a constructor functor
// For example, to capture the above structure in a Person type, do this:
//
// using PersonParameter = CompoundParameter<
//     Person,
//     SimpleParameter<std::string>,
//     SimpleParameter<int>>;
//
// PersonParameter param("FieldName", "",
//     Person(22, "Connor"), <<< optional defualt value after "" printName
//     [](const auto& p) { return Person(p.get<0>(), p.get<1>()); }, <<< lambda for construction functor
//     SimpleParameter<std::string>("name"),
//     SimpleParameter<int>("age")
// );
//
// If every parameter passed has a default value in their constructors,
// Then the CompoundParameter will also be defaultable to all of those values
//
// Use a void type to simplify the CompoundParameter to just an aggregate of parameters
// Then no constructor is requried

template <typename T = void, typename... Us>
requires (std::derived_from<Us, ConfigurationParameter> && ...)
class CompoundParameter : public ConfigurationParameter {

    friend class YamlFile;

private:

    bool _defaulted;
    std::function<T(const CompoundParameter&)> _constructor;
    std::tuple<Us...> _parameters;
    std::optional<T> _value;

public:

    CompoundParameter() noexcept {}

    CompoundParameter(const std::string& fieldName, std::function<T(const CompoundParameter&)> constructor, const Us&... args) noexcept
            : ConfigurationParameter(fieldName),
              _constructor(constructor),
              _parameters(args...) {
        // Funky logic for defaulting if each passed parameter is also defaulted
        _defaulted = std::apply([](const auto&... parameter) {
            return (parameter.initialized() && ...);
        }, _parameters);
    }

    CompoundParameter(const std::string& fieldName, const std::string& printName, std::function<T(const CompoundParameter&)> constructor, const Us&... args) noexcept
            : ConfigurationParameter(fieldName, printName),
              _constructor(constructor),
              _parameters(args...) {
        _defaulted = std::apply([](const auto&... parameter) {
            return (parameter.initialized() && ...);
        }, _parameters);
    }

    CompoundParameter(const std::string& fieldName, const std::string& printName, const T& defaultValue, std::function<T(const CompoundParameter&)> constructor, const Us&... args) noexcept
            : ConfigurationParameter(fieldName, printName),
              _defaulted(true),
              _constructor(constructor),
              _parameters(args...),
              _value(defaultValue) {}

    //==================================================
    // Access the underlying parameters with
    // ParamType v = parameter.get<idx>()
    // Don't forget that idx must be constexpr
    //
    // Also access the underlying object with
    // T v = parameter;
    // T v = parameter.value();

    template <int I>
    auto& get(this auto& self) noexcept {
        return std::get<I>(self._parameters);
    };

    operator T&() noexcept {
        return _value.value();
    }

    operator const T&() const noexcept {
        return _value.value();
    }

    auto& value(this auto& self) noexcept {
        return self._value.value();
    }

    bool initialized() const noexcept override final {
        return static_cast<bool>(_value) || _defaulted;
    }

    void construct() noexcept {
        _value = _constructor(*this);
    }
};

//==================================================
// void specialization
//==================================================

template <typename... Us>
requires (std::derived_from<Us, ConfigurationParameter> && ...)
class CompoundParameter<void, Us...> : public ConfigurationParameter {

    friend class YamlFile;

private:

    bool _defaulted;
    std::tuple<Us...> _parameters;

public:

    CompoundParameter() noexcept {}

    CompoundParameter(const std::string& fieldName, const Us&... args) noexcept
            : ConfigurationParameter(fieldName),
              _parameters(args...) {
        // Funky logic for defaulting if each passed parameter is also defaulted
        _defaulted = std::apply([](const auto&... parameter) {
            return (parameter.initialized() && ...);
        }, _parameters);
    }

    CompoundParameter(const std::string& fieldName, const std::string& printName, const Us&... args) noexcept
            : ConfigurationParameter(fieldName, printName),
              _parameters(args...) {
        _defaulted = std::apply([](const auto&... parameter) {
            return (parameter.initialized() && ...);
        }, _parameters);
    }

    //==================================================
    // Access the underlying parameters with
    // ParamType v = parameter.get<idx>()
    // Don't forget that idx must be constexpr
    //
    // Also access the underlying object with
    // T v = parameter;
    // T v = parameter.value();

    template <int I>
    auto& get(this auto& self) noexcept {
        return std::get<I>(self._parameters);
    };

    bool initialized() const noexcept override final {
        return _defaulted;
    }

    void construct() noexcept {}
};

template <typename... Us>
requires (std::derived_from<Us, ConfigurationParameter> && ...)
CompoundParameter(const std::string& fieldName, const Us&... args) -> CompoundParameter<void, Us...>;

template <typename... Us>
requires (std::derived_from<Us, ConfigurationParameter> && ...)
CompoundParameter(const std::string& fieldName, const std::string& printName, const Us&... args) -> CompoundParameter<void, Us...>;

//==================================================
// The YAML file wrapper which fills parameters using the YAML-CPP library
// After supplying a file path to the constructor, the file can be read from to fill parameters
// All parameter types are filled with one function fillParameter
// If a parameter can't be found, fillParameter will call mse::debug::fatal to terminate the program
// Once all consumers have finished filling their parameters, call verifyStrictAccess if desired
//   If there is a field in the file which wasn't ever accessed, mse::debug::fatal is called
//   This is useful for ensuring users input only correct files without excess information
//   It is likely that input files for different contexts will not use the same exact fields
//   But in case of a subset, calling this will notify users instead of letting it slide

class YamlFile {
private:

    const std::string _filePath;
    const YAML::Node _fileRoot;
    std::unordered_set<std::string> _touchedFields;

public:

    YamlFile(std::string fileName) : _filePath(fileName), _fileRoot(YAML::LoadFile(fileName)) {}

    // Strict ownership
    // No copying or moving for simplicity
    YamlFile(const YamlFile&) = delete;
    YamlFile(YamlFile&&) = delete;
    YamlFile& operator=(const YamlFile&) = delete;
    YamlFile& operator=(YamlFile&&) = delete;

    //==================================================
    // This fillParameter is called to fill all parameter types
    // It overloads to the other fillParameter static functions to take different action
    // This needed because each parameter interacts with YAML in different ways
    // All fillParameter overloads return true if the parameter was filled by this call
    //   Meaning it was found in the input file and not given a default value
    // Returning false indicates a default value was given to the parameter
    // No return state indicates not found in the file with no default since that is a throwing condition

    template <typename... Ts>
    requires (std::derived_from<Ts, ConfigurationParameter> && ...)
    bool fillParameter(Ts&... parameters) noexcept {
        try {
            bool result = (fillParameter(parameters, _fileRoot) && ...); // Overload to certain parameter type
            (touchField(parameters.fieldName), ...); // Mark field as seen in this input file
            return result;
        } catch (const YAML::Exception& e) {
            outputLog.fatal("Error parsing YAML file \"{}\": {}\n", _filePath, e.what());
            mse::exit(mse::ExitCode::Failure);
        } catch (const std::exception& e) {
            outputLog.fatal("Error reading file \"{}\": {}\n", _filePath, e.what());
            mse::exit(mse::ExitCode::Failure);
        }
    }

    const std::string& name() const noexcept { return _filePath; }
    void verifyStrictAccess() const noexcept; // Terminates program if unused fields still exist

private:

    //==================================================
    // Helper overloads for each parameter type
    // Necessary to get around virtual template incompatibility

    template <typename T>
    static bool fillParameter(SimpleParameter<T>& parameter, const YAML::Node& root) {
        YAML::Node node = root[parameter.fieldName];
        if (!checkForField(node, parameter)) {
            return false;
        }
        if (node.Type() != YAML::NodeType::Scalar) {
            throw std::runtime_error(std::string("Simple parameter of field \"") + parameter.fieldName + "\" not interpreted as a scalar node");
        }

        // Enum values are looked for in their map if one exists
        // Otherwise treat the type as a normal one
        if constexpr (EnumIsMapped<T>) {
            std::string value;
            nodeValue(node, value);
            const auto it = EnumMap<T>::fromYaml.find(value);
            if (it == EnumMap<T>::fromYaml.cend()) {
                throw std::runtime_error(std::string("Field \"") + parameter.fieldName + "\" value \"" + value + "\" has no enum mapping");
            }
            parameter._value = it->second;
        } else {
            T v;
            nodeValue(node, v);
            parameter._value = std::move(v);
        }

        return true;
    }

    template <typename T, int C>
    static bool fillParameter(ArrayParameter<T, C>& parameter, const YAML::Node& root) {
        YAML::Node node = root[parameter.fieldName];
        if (!checkForField(node, parameter)) {
            return false;
        }
        if (node.Type() != YAML::NodeType::Sequence) {
            throw std::runtime_error(std::string("Array parameter of field \"") + parameter.fieldName + "\" not interpreted as a sequence node");
        }

        parameter.construct(); // Default constructs internal container

        // Enum values are looked for in their map if one exists
        // Otherwise treat the type as a normal one
        if constexpr (EnumIsMapped<T>) {
            for (const auto& item : node) {
                std::string value;
                nodeValue(item, value);
                const auto it = EnumMap<T>::fromYaml.find(value);
                if (it == EnumMap<T>::fromYaml.cend()) {
                    throw std::runtime_error(std::string("Field \"") + parameter.fieldName + "\" value \"" + value + "\" has no enum mapping");
                }
                parameter.append(it->second);
            }
        } else {
            for (const auto& item : node) {
                T v;
                nodeValue(item, v);
                parameter.append(std::move(v));
            }
        }

        // Check to see if the correct number of elements was read
        if (!parameter.hasExactCount()) {
            throw std::runtime_error(std::string("Array parameter expected ") + std::to_string(C) + " items");
        }

        return true;
    }

    template <typename... Ts>
    static bool fillParameter(CompoundParameter<Ts...>& parameter, const YAML::Node& root) {
        YAML::Node node = root[parameter.fieldName];
        if (!checkForField(node, parameter)) {
            return false;
        }
        if (node.Type() != YAML::NodeType::Map) {
            throw std::runtime_error(std::string("Compound parameter of field \"") + parameter.fieldName + "\" not interpreted as a map node");
        }

        // Get the values of sub parameter
        std::apply([&node](auto&... p) {
            (fillParameter(p, node), ...);
        }, parameter._parameters);

        // Use construction rule to create the compound type
        parameter.construct();

        return true;
    }

    //==================================================
    // nodeValue is a specialized function used whenever the value of a field is needed
    // Overload custom parameter types to let them be naturally assigned from files

    template <typename T>
    static void nodeValue(const YAML::Node& node, T& value) {
        value = node.as<T>();
    }

    template <typename... Ts>
    static void nodeValue(const YAML::Node& node, unit::Unit<Ts...>& value) {
        value = mse::unit::Unit<Ts...>(node.as<typename mse::unit::Unit<Ts...>::UnitValueType>());
    }

    //==================================================
    // Functions for checking strict access

    static bool checkForField(const YAML::Node& node, const ConfigurationParameter& parameter);

    void touchField(const std::string& field);
    bool strictlyAccessed() const; // Checks if all the top level fields have been accessed at least once
};

} // namespace mse::input

#endif
