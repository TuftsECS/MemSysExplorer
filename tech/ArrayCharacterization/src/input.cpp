#include "input.hpp"

#include "log.hpp"
#include "unit.hpp"

//==================================================
// Terminates the program if not all keys in the input file were accessed

bool mse::YamlInputFile::verifyStrictAccess() const noexcept {
    bool result = strictlyAccessed();
    if (!result) {
        outputLog.warn("Error parsing YAML file \"{}\": strict access check failed, ensure there are no unused fields in the file\n", _filePath);
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

void mse::YamlInputFile::touchKey(const std::string& key) {
    _touchedKeys.insert(key);
}

bool mse::YamlInputFile::strictlyAccessed() const {
    // Return false if ANY node's key in the top level does NOT exist in the touched keys list
    for (const auto& kv : _fileRoot) {
        std::string key = kv.first.as<std::string>();
        if (_touchedKeys.find(key) == _touchedKeys.cend()) {
            mse::outputLog.warn("Key \"{}\" not accessed in YAML input file \"{}\"\n", key, _filePath);
            return false;
        }
    }
    // Otherwise all keys in the top level were accessed / touched
    return true;
}
