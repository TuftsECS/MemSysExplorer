// This file contains template structures for converting YAML field values / strings to enum values and enum values to strings
// It is meant to reduce boilerplate when working with enums
// The enumToString function takes any specialized enum value and returns its string representation
// The enumToYaml function takes any specialized enum value and returns its YAML name
// input.hpp functions also use these maps in parameter filling logic
// All mappings created in this file will automatically apply to that logic
//
// To add mappings for a new enum, use the following specialization pattern:
//
// template<>
// struct EnumMap<EnumType> {
//     static inline const EnumFromYamlMap<EnumType> fromYaml = {
//         {"FieldValue1", EnumValue1},
//         {"FieldValue2", EnumValue2},
//         ...
//     };
// 
//     *OPTIONAL MAPPING - fromYaml USED WHEN THIS MAPPING NOT PRESENT*
//     static inline const EnumToStringMap<EnumType> toString = {
//         {EnumValue1, "String1"},
//         {EnumValue2, "String2"},
//         ...
//     };
// };
//
// The toString mapping can be defined when other strings beside the YAML field values should be used for printing
// If it is not defined, a backwards search through fromYaml is used for printing enum values

#ifndef MSE_ENUMINFO_HPP
#define MSE_ENUMINFO_HPP

#include "typedef.hpp"
#include "debug.hpp"

#include "yaml-cpp/yaml.h"

#include <unordered_map>
#include <iostream>

// Templated struct which will be specialized for each enum mapping
template<typename T>
//requires std::is_scoped_enum_v<T> TODO
struct EnumMap;

// Concept for determining if enum is actually mapped
template<typename T>
//requires std::is_scoped_enum_v<T> TODO
concept EnumIsMapped = requires {
    typename EnumMap<T>;
    EnumMap<T>::fromYaml;
};

// Types to use in EnumMap structs for YAML to enum mappings and enum to string mappings
template<typename T>
//requires std::is_scoped_enum_v<T> TODO
using EnumFromYamlMap = std::unordered_map<std::string, T>;

template<typename T>
//requires std::is_scoped_enum_v<T> TODO
using EnumToStringMap = std::unordered_map<T, std::string>;

//==================================================
// Search the enum's fromYaml map backwards to convert back into the YAML string

template<typename T>
requires EnumIsMapped<T>
std::string enumToYaml(T v) noexcept {
    const auto& map = EnumMap<T>::fromYaml;
    // For loop instead of std:: algo because backward search is nonstandard but simple :)
    for(auto it = map.cbegin(); it != map.cend(); ++it) {
        if (it->second == v) {
            return it->first;
        }
    }
    
    // No mapping rule is bad and not really recoverable, so just terminate
    mse::debug::fatal("Missing YAML to enum conversion rule\n");
}

//==================================================
// Turn an enum value into a string from its mappings
// If the optional toString map exists, use that string
// Otherwise, just use the fromYaml map in enumToYaml

template<typename T>
requires EnumIsMapped<T>
std::string enumToString(T v) noexcept {
    // Check if the optional toString mapping exists
    if constexpr (requires { EnumMap<T>::toString; }) {
        const auto& map = EnumMap<T>::toString;
        const auto it = map.find(v);

        // If no mapping was found, not a fatal error
        // Warn the user and just use enumToYaml as a fallback
        if (it == map.cend()) {
            mse::debug::warn(it == map.cend(), "Missing enum to string conversion rule\n");
            return enumToYaml(v);
        }
        // Otherwise, success and return the string mapping
        return it->second;
    } else {
        return enumToYaml(v);
    }
}

//==================================================
// Custom stream insertion operator to print enums naturally

template<typename T>
requires EnumIsMapped<T>
std::ostream& operator<<(std::ostream& stream, T v) {
    stream << enumToString(v);
    return stream;
}

//==================================================

template<>
struct EnumMap<MemCellType> {
    static inline const EnumFromYamlMap<MemCellType> fromYaml = {
        {"SRAM", SRAM},
        {"DRAM", DRAM},
        {"eDRAM", eDRAM},
        {"eDRAM3T", eDRAM3T},
        {"eDRAM3T333", eDRAM3T333},
        {"MRAM", MRAM},
        {"PCRAM", PCRAM},
        {"memristor", memristor},
        {"FBRAM", FBRAM},
        {"SLCNAND", SLCNAND},
        {"MLCNAND", MLCNAND},
        {"CTT", CTT},
        {"MLCCTT", MLCCTT},
        {"FeFET", FeFET},
        {"MLCFeFET", MLCFeFET},
        {"MLCRRAM", MLCRRAM}
    };

    static inline const EnumToStringMap<MemCellType> toString = {
        {SRAM, "SRAM"},
        {DRAM, "DRAM"},
        {eDRAM, "Embedded DRAM"},
        {eDRAM3T, "3T Embedded DRAM"},
        {eDRAM3T333, "333 Embedded DRAM"},
        {MRAM, "MRAM (Magnetoresistive)"},
        {PCRAM, "PCRAM (Phase-Change)"},
        {memristor, "RRAM (Memristor)"},
        {FBRAM, "FBRAM (Floating Body)"},
        {SLCNAND, "Single-Level Cell NAND Flash"},
        {MLCNAND, "Multi-Level Cell NAND Flash"},
        {CTT, "Single-Level Cell CTT"},
        {MLCCTT, "Multi-Level Cell CTT"},
        {FeFET, "Single-Level Cell FeFET"},
        {MLCFeFET, "Multi-Level Cell FeFET"},
        {MLCRRAM, "Multi-Level Cell RRAM (Memristor)"}
    };
};

//==================================================

template<>
struct EnumMap<CellAccessType> {
    static inline const EnumFromYamlMap<CellAccessType> fromYaml = {
        {"CMOS", CMOS_access},
        {"BJT", BJT_access},
        {"diode", diode_access},
        {"None", none_access}
    };

    static inline const EnumToStringMap<CellAccessType> toString = {
        {CMOS_access, "CMOS"},
        {BJT_access, "BJT"},
        {diode_access, "Diode"},
        {none_access, "None Access Device"}
    };
};

//==================================================

template<>
struct EnumMap<DeviceRoadmap> {
    static inline const EnumFromYamlMap<DeviceRoadmap> fromYaml = {
        {"HP", HP},
        //{"LSTP", LSTP},
        {"LOP", LOP},
        //{"EDRAM", EDRAM},
        {"IGZO", IGZO},
        {"CNT", CNT}
    };

    static inline const EnumToStringMap<DeviceRoadmap> toString = {
        {HP, "HP"},
        {LSTP, "LSTP"},
        {LOP, "LOP"},
        {EDRAM, "EDRAM"},
        {IGZO, "IGZO"},
        {CNT, "CNT"}
    };

    static constexpr auto configError = "Invalid DeviceRoadmap (choose HP/LOP/CNT/IGZO)";
};

//==================================================

template<>
struct EnumMap<WireType> {
    static inline const EnumFromYamlMap<WireType> fromYaml = {
        {"LocalAggressive", local_aggressive},
        {"LocalConservative", local_conservative},
        {"SemiAggressive", semi_aggressive},
        {"SemiConservative", semi_conservative},
        {"GlobalAggressive", global_aggressive},
        {"GlobalConservative", global_conservative},
        {"DRAMWordline", dram_wordline}
    };

    static inline const EnumToStringMap<WireType> toString = {
        {local_aggressive, "Local Aggressive"},
        {local_conservative, "Local Conservative"},
        {semi_aggressive, "Semi-Global Aggressive"},
        {semi_conservative, "Semi-Global Conservative"},
        {global_aggressive, "Global Aggressive"},
        {global_conservative, "Global Conservative"},
        {dram_wordline, "DRAM Wire"}
    };
};

//==================================================

template<>
struct EnumMap<WireRepeaterType> {
    static inline const EnumFromYamlMap<WireRepeaterType> fromYaml = {
        {"RepeatedNone", repeated_none},
        {"RepeatedOpt", repeated_opt},
        {"Repeated5%Penalty", repeated_5},
        {"Repeated10%Penalty", repeated_10},
        {"Repeated20%Penalty", repeated_20},
        {"Repeated30%Penalty", repeated_30},
        {"Repeated40%Penalty", repeated_40},
        {"Repeated50%Penalty", repeated_50}
    };

    static inline const EnumToStringMap<WireRepeaterType> toString = {
        {repeated_none, "No Repeaters"},
        {repeated_opt, "Fully-Optimized Repeaters"},
        {repeated_5, "Repeaters with 5% Overhead"},
        {repeated_10, "Repeaters with 10% Overhead"},
        {repeated_20, "Repeaters with 20% Overhead"},
        {repeated_30, "Repeaters with 30% Overhead"},
        {repeated_40, "Repeaters with 40% Overhead"},
        {repeated_50, "Repeaters with 50% Overhead"},
    };
};

//==================================================

template<>
struct EnumMap<BufferDesignTarget> {
    static inline const EnumFromYamlMap<BufferDesignTarget> fromYaml = {
        {"latency", latency_first},
        {"trade", latency_area_trade_off},
        {"area", area_first}
    };

    static inline const EnumToStringMap<BufferDesignTarget> toString = {
        {latency_first, "Latency-Optimized"},
        {latency_area_trade_off, "Balanced"},
        {area_first, "Area-Optimized"}
    };
};

//==================================================

template<>
struct EnumMap<MemoryType> {
    static inline const EnumFromYamlMap<MemoryType> fromYaml = {};

    static inline const EnumToStringMap<MemoryType> toString = {
        {dataT, "DataT"},
        {tag, "Tag"},
        {CAM, "CAM"}
    };
};

//==================================================

template<>
struct EnumMap<RoutingMode> {
    static inline const EnumFromYamlMap<RoutingMode> fromYaml = {
        {"H-tree", h_tree}
    };

    static inline const EnumToStringMap<RoutingMode> toString = {
        {h_tree, "H-tree"},
        {non_h_tree, "Non-H-tree"}
    };
};

//==================================================

template<>
struct EnumMap<WriteScheme> {
    static inline const EnumFromYamlMap<WriteScheme> fromYaml = {
        {"SetBeforeReset", set_before_reset},
        {"ResetBeforeSet", reset_before_set},
        {"EraseBeforeSet", erase_before_set},
        {"EraseBeforeReset", erase_before_reset},
        {"WriteAndVerify", write_and_verify},
        {"NormalWrite", normal_write}
    };

    static inline const EnumToStringMap<WriteScheme> toString = {
        {set_before_reset, "Set Before Reset"},
        {reset_before_set, "Reset Before Set"},
        {erase_before_set, "Erase Before Set"},
        {erase_before_reset, "Erase Before Reset"},
        {write_and_verify, "Write And Verify"},
        {normal_write, "Normal Write"}
    };
};

//==================================================

template<>
struct EnumMap<DesignTarget> {
    static inline const EnumFromYamlMap<DesignTarget> fromYaml = {
        {"cache", cache},
        {"RAM", RAM_chip},
        {"CAM", CAM_chip}
    };

    static inline const EnumToStringMap<DesignTarget> toString = {
        {cache, "Cache"},
        {RAM_chip, "Random Access Memory"},
        {CAM_chip, "Content Addressable Memory"}
    };
};

//==================================================

template<>
struct EnumMap<OptimizationTarget> {
    static inline const EnumFromYamlMap<OptimizationTarget> fromYaml = {
        {"ReadLatency", read_latency_optimized},
        {"WriteLatency", write_latency_optimized},
        {"ReadDynamicEnergy", read_energy_optimized},
        {"WriteDynamicEnergy", write_energy_optimized},
        {"ReadEDP", read_edp_optimized},
        {"WriteEDP", write_edp_optimized},
        {"LeakagePower", leakage_optimized},
        {"Area", area_optimized},
        {"Full", full_exploration}
    };

    static inline const EnumToStringMap<OptimizationTarget> toString = {
        {read_latency_optimized, "read latency"},
        {write_latency_optimized, "write latency"},
        {read_energy_optimized, "read energy"},
        {write_energy_optimized, "write energy"},
        {read_edp_optimized, "read energy-delay-product"},
        {write_edp_optimized, "write energy-delay-product"},
        {leakage_optimized, "leakage power"},
        {area_optimized, "area"},
        {full_exploration, "full exploration"}
    };
};

//==================================================

template<>
struct EnumMap<CacheAccessMode> {
    static inline const EnumFromYamlMap<CacheAccessMode> fromYaml = {
        {"Normal", normal_access_mode},
        {"Sequential", sequential_access_mode},
        {"Fast", fast_access_mode}
    };

    static inline const EnumToStringMap<CacheAccessMode> toString = {
        {normal_access_mode, "Normal"},
        {sequential_access_mode, "Sequential"},
        {fast_access_mode, "Fast"}
    };
};

//==================================================
// TODO REMOVE THIS FUNCTION
// This still exists because the new input interface is not fully added yet
// Once ready, delete this function and fix all references to it to use the new input interface

template<typename T>
bool yamlValueFromNode(T& var, const YAML::Node& node, const std::string& key) {
    if constexpr (EnumIsMapped<T>) {
        // one of the mapped enum types, first get the string and then do enum mapping magic
        std::string enumString;
        if (node[key]) {
            enumString = node[key].as<std::string>();
            const auto it = EnumMap<T>::fromYaml.find(enumString);
            if (it == EnumMap<T>::fromYaml.cend()) {
                throw std::runtime_error(std::string("Unconvertable enum string: ") + enumString);
            }
            var = it->second;
            return true;
        }
    } else {
        // other type, attempt to cast in .as<T>()
        if (node[key]) {
            var = node[key].as<T>();
            return true;
        }
    }
    return false;
}

#endif
