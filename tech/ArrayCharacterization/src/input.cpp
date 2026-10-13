#include "input.hpp"

#include "log.hpp"
#include "unit.hpp"

#include <charconv>
#include <format>

//==================================================
// Terminates the program if not all keys in the input file were accessed

bool mse::YamlInputFile::verifyStrictAccess() const noexcept {
    bool result = strictlyAccessed();
    if (!result) {
        log::warn(std::format("Error parsing YAML file \"{}\": strict access check failed, ensure there are no unused fields in the file", _filePath));
    }
    return result;
}

//==================================================
// Functions for checking strict access

bool mse::YamlInputFile::checkForKey(const YAML::Node& node, const InputFileParameter& parameter) {
    // If the key is not found / node is invalid
    if (!node) {
        // But the parameter is initialized (with a default value)
        if (parameter.initialized()) {
            // Return false to signal no further action should be taken by the getting caller
            return false;
        }
        // Otherwise the key is completely missing and no default is available
        // So throw
        throw std::runtime_error("Key \"" + parameter.key + "\" not found");
    }
    // The key exists, so the getter function should keep going and extract its value
    return true;
}

void mse::YamlInputFile::touchKey(std::string key) {
    _touchedKeys.insert(std::move(key));
}

bool mse::YamlInputFile::strictlyAccessed() const {
    // Return false if ANY node's key in the top level does NOT exist in the touched keys list
    for (const auto& kv : _fileRoot) {
        std::string key = kv.first.as<std::string>();
        if (_touchedKeys.find(key) == _touchedKeys.cend()) {
            log::warn(std::format("Key \"{}\" not accessed in YAML input file \"{}\"", key, _filePath));
            return false;
        }
    }
    // Otherwise all keys in the top level were accessed / touched
    return true;
}

//==================================================
// Functions for reading values

YAML::Node mse::YamlInputFile::findNode(const YAML::Node& root, std::string_view key) {
    const size_t dot = key.find('.');
    const YAML::Node node = root[std::string(key.substr(0, dot))];
    if (dot == std::string_view::npos || !node) {
        return node;
    }

    return findNode(node, key.substr(dot + 1));
}

double mse::YamlInputFile::unitValue(const std::string& text, std::string_view unitString) {
    std::string_view rest = text;
    rest.remove_prefix(std::min(rest.find_first_not_of(" +"), rest.size()));

    double value = 0;
    const auto [end, error] = std::from_chars(rest.data(), rest.data() + rest.size(), value);
    if (error != std::errc()) {
        throw std::runtime_error(std::format("Value \"{}\" is not a number", text));
    }
    rest.remove_prefix(end - rest.data());
    rest.remove_prefix(std::min(rest.find_first_not_of(' '), rest.size()));
    rest.remove_suffix(rest.size() - std::min(rest.find_last_not_of(' ') + 1, rest.size()));

    if (rest.empty()) {
        return value;
    }
    if (unitString.empty() || !rest.ends_with(unitString)) {
        throw std::runtime_error(std::format("Value \"{}\" should be a number with unit \"{}\"", text, unitString));
    }

    // Fractional scales divide so values match the legacy parsing of keys like Pulse_ns
    const std::string_view scale = rest.substr(0, rest.size() - unitString.size());
    if (scale.empty()) {
        return value;
    } else if (scale == "a") {
        return value / 1e18;
    } else if (scale == "f") {
        return value / 1e15;
    } else if (scale == "p") {
        return value / 1e12;
    } else if (scale == "n") {
        return value / 1e9;
    } else if (scale == "u") {
        return value / 1e6;
    } else if (scale == "m") {
        return value / 1e3;
    } else if (scale == "K" || scale == "k") {
        return value * 1e3;
    } else if (scale == "M") {
        return value * 1e6;
    } else if (scale == "G") {
        return value * 1e9;
    } else if (scale == "T") {
        return value * 1e12;
    }

    throw std::runtime_error(std::format("Value \"{}\" has an unknown unit scale \"{}\"", text, scale));
}
