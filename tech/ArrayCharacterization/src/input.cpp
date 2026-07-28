#include "input.hpp"

#include "log.hpp"
#include "Unit.hpp"

//==================================================
// Terminates the program if not all fields in the input file were accessed

void mse::input::YamlFile::verifyStrictAccess() const noexcept {
    if (!strictlyAccessed()) {
        outputLog.error("Error parsing YAML file \"{}\": strict access check failed, ensure there are no unused fields in the file\n", _filePath);
    }
}

//==================================================
// Functions for checking strict access

bool mse::input::YamlFile::checkForField(const YAML::Node& node, const ConfigurationParameter& parameter) {
    // If the field is not found / node is invalid
    if (!node) {
        // But the parameter is initialized (with a default value)
        if (parameter.initialized()) {
            // Return false to signal no further action should be taken by the getting caller
            return false;
        }
        // Otherwise the field is completely missing and no default is available
        // So throw
        throw std::runtime_error(std::string("Field name \"") + parameter.fieldName + "\" not found");
    }
    // The field exists, so the getter function should keep going and extract its value
    return true;
}

void mse::input::YamlFile::touchField(const std::string& field) {
    _touchedFields.insert(field);
}

bool mse::input::YamlFile::strictlyAccessed() const {
    // Return false if ANY node's field name in the top level does NOT exist in the touched fields list
    for (const auto& kv : _fileRoot) {
        std::string key = kv.first.as<std::string>();
        if (_touchedFields.find(key) == _touchedFields.cend()) {
            return false;
        }
    }
    // Otherwise all all fields in the top level were accessed / touched
    return true;
}
