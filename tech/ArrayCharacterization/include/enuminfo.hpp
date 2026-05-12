#ifndef MSE_ENUMINFO_H
#define MSE_ENUMINFO_H

#include "typedef.hpp"

#include "yaml-cpp/yaml.h"

#include <unordered_map>
#include <iostream>

/*
 * This file contains template structures for automatically converting YAML strings to enum values and enum values to strings
 * It is meant to reduce boilerplate when working with enums
 * The enumToString function takes any specialized enum value and returns its string representation
 * The enumToYaml function takes any specialized enum value and returns its YAML name
 * The yamlValueFromNode function assigns some type (specialized enum or other) using a YAML node object and field string
 *
 * To add mappings for a new enum, use the following specialization pattern:
 *
 * template<>
 * struct EnumInfo<ENUM_TYPE> {
 *      static constexpr auto name = "ENUM_TYPE"; (or something else appropriate)
 *      static inline const EnumFromYamlMap<ENUM_TYPE> yamlMap = {...};
 *      static inline const EnumToStringMap<ENUM_TYPE> stringMap = {...};
 *
 *      (these are for YAML things - if your enum should have a default value,
 *          define defaultConfigValue, otherwise define configError)
 *      static constexpr auto defaultConfigValue = ...;
 *      static constexpr auto configError = "...";
 *  };
 *
 *  defaultConfigValue is the value assigned to a YAML parameter when no value matches in yamlMap, INCLUDING CASES WHERE A YAML FIELD IS GIVEN BUT INVALID FOR THE TYPE
 *  configError is a string used in a runtime exception if a YAML parameter must be explicitly given a value, but either nothing was given or the given value was not in yamlMap
 */

// Templated struct which will be specialized for each support enum type
template<typename T>
struct EnumInfo;

// Helper trait struct which is specialized below to have std::true_type whenever a user defines default value
template<typename T, typename = void>
struct EnumHasDefault : std::false_type {};

template<typename T>
struct EnumHasDefault<T, std::void_t<decltype(EnumInfo<T>::defaultConfigValue)>> : std::true_type {};

// Helper trait struct which is specialized below to have std::true_type for each custom enum info struct defined below (checking stringMap)
template<typename T, typename = void>
struct EnumIsMapped : std::false_type {};

template<typename T>
struct EnumIsMapped<T, std::void_t<decltype(EnumInfo<T>::stringMap)>> : std::true_type {};

// Types to use in EnumInfo structs for YAML to enum mappings and enum to string mappings
template<typename T>
using EnumFromYamlMap = std::unordered_map<std::string, T>;
template<typename T>
using EnumToStringMap = std::unordered_map<T, std::string>;

//==================================================

template<>
struct EnumInfo<MemCellType> {
    static constexpr auto name = "MemCellType";

    static inline const EnumFromYamlMap<MemCellType> yamlMap = {
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

    static inline const EnumToStringMap<MemCellType> stringMap = {
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

    static constexpr auto defaultConfigValue = MLCNAND;
};

//==================================================

template<>
struct EnumInfo<CellAccessType> {
    static constexpr auto name = "CellAccessType";

    static inline const EnumFromYamlMap<CellAccessType> yamlMap = {
        {"CMOS", CMOS_access},
        {"BJT", BJT_access},
        {"diode", diode_access},
        {"none", none_access}
    };

    static inline const EnumToStringMap<CellAccessType> stringMap = {
        {CMOS_access, "CMOS"},
        {BJT_access, "BJT"},
        {diode_access, "Diode"},
        {none_access, "None Access Device"}
    };

    static constexpr auto defaultConfigValue = none_access;
};

//==================================================

template<>
struct EnumInfo<DeviceRoadmap> {
    static constexpr auto name = "DeviceRoadmap";

    static inline const EnumFromYamlMap<DeviceRoadmap> yamlMap = {
        {"HP", HP},
        //{"LSTP", LSTP},
        {"LOP", LOP},
        //{"EDRAM", EDRAM},
        {"IGZO", IGZO},
        {"CNT", CNT}
    };

    static inline const EnumToStringMap<DeviceRoadmap> stringMap = {
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
struct EnumInfo<WireType> {
    static constexpr auto name = "WireType";

    static inline const EnumFromYamlMap<WireType> yamlMap = {
        {"LocalAggressive", local_aggressive},
        {"LocalConservative", local_conservative},
        {"SemiAggressive", semi_aggressive},
        {"SemiConservative", semi_conservative},
        {"GlobalAggressive", global_aggressive},
        {"GlobalConservative", global_conservative},
        {"DRAMWordline", dram_wordline}
    };

    static inline const EnumToStringMap<WireType> stringMap = {
        {local_aggressive, "Local Aggressive"},
        {local_conservative, "Local Conservative"},
        {semi_aggressive, "Semi-Global Aggressive"},
        {semi_conservative, "Semi-Global Conservative"},
        {global_aggressive, "Global Aggressive"},
        {global_conservative, "Global Conservative"},
        {dram_wordline, "DRAM Wire"}
    };

    static constexpr auto defaultConfigValue = dram_wordline;
};

//==================================================

template<>
struct EnumInfo<WireRepeaterType> {
    static constexpr auto name = "WireRepeaterType";

    static inline const EnumFromYamlMap<WireRepeaterType> yamlMap = {
        {"RepeatedNone", repeated_none},
        {"RepeatedOpt", repeated_opt},
        {"Repeated5%Penalty", repeated_5},
        {"Repeated10%Penalty", repeated_10},
        {"Repeated20%Penalty", repeated_20},
        {"Repeated30%Penalty", repeated_30},
        {"Repeated40%Penalty", repeated_40},
        {"Repeated50%Penalty", repeated_50}
    };

    static inline const EnumToStringMap<WireRepeaterType> stringMap = {
        {repeated_none, "No Repeaters"},
        {repeated_opt, "Fully-Optimized Repeaters"},
        {repeated_5, "Repeaters with 5% Overhead"},
        {repeated_10, "Repeaters with 10% Overhead"},
        {repeated_20, "Repeaters with 20% Overhead"},
        {repeated_30, "Repeaters with 30% Overhead"},
        {repeated_40, "Repeaters with 40% Overhead"},
        {repeated_50, "Repeaters with 50% Overhead"},
    };

    static constexpr auto defaultConfigValue = repeated_none;
};

//==================================================

template<>
struct EnumInfo<BufferDesignTarget> {
    static constexpr auto name = "BufferDesignTarget";

    static inline const EnumFromYamlMap<BufferDesignTarget> yamlMap = {
        {"latency", latency_first},
        {"trade", latency_area_trade_off},
        {"area", area_first}
    };

    static inline const EnumToStringMap<BufferDesignTarget> stringMap = {
        {latency_first, "Latency-Optimized"},
        {latency_area_trade_off, "Balanced"},
        {area_first, "Area-Optimized"}
    };

    static constexpr auto defaultConfigValue = latency_area_trade_off;
};

//==================================================

template<>
struct EnumInfo<MemoryType> {
    static constexpr auto name = "MemoryType";

    static inline const EnumFromYamlMap<MemoryType> yamlMap = {};

    static inline const EnumToStringMap<MemoryType> stringMap = {
        {dataT, "DataT"},
        {tag, "Tag"},
        {CAM, "CAM"}
    };

    static constexpr auto defaultConfigValue = tag;
};

//==================================================

template<>
struct EnumInfo<RoutingMode> {
    static constexpr auto name = "RoutingMode";

    static inline const EnumFromYamlMap<RoutingMode> yamlMap = {
        {"H-tree", h_tree}
    };

    static inline const EnumToStringMap<RoutingMode> stringMap = {
        {h_tree, "H-tree"},
        {non_h_tree, "Non-H-tree"}
    };

    static constexpr auto defaultConfigValue = non_h_tree;
};

//==================================================

template<>
struct EnumInfo<WriteScheme> {
    static constexpr auto name = "WriteScheme";

    static inline const EnumFromYamlMap<WriteScheme> yamlMap = {
        {"SetBeforeReset", set_before_reset},
        {"ResetBeforeSet", reset_before_set},
        {"EraseBeforeSet", erase_before_set},
        {"EraseBeforeReset", erase_before_reset},
        {"WriteAndVerify", write_and_verify},
        {"NormalWrite", normal_write}
    };

    static inline const EnumToStringMap<WriteScheme> stringMap = {
        {set_before_reset, "Set Before Reset"},
        {reset_before_set, "Reset Before Set"},
        {erase_before_set, "Erase Before Set"},
        {erase_before_reset, "Erase Before Reset"},
        {write_and_verify, "Write And Verify"},
        {normal_write, "Normal Write"}
    };

    static constexpr auto defaultConfigValue = normal_write;
};

//==================================================

template<>
struct EnumInfo<DesignTarget> {
    static constexpr auto name = "DesignTarget";

    static inline const EnumFromYamlMap<DesignTarget> yamlMap = {
        {"cache", cache},
        {"RAM", RAM_chip},
        {"CAM", CAM_chip}
    };

    static inline const EnumToStringMap<DesignTarget> stringMap = {
        {cache, "Cache"},
        {RAM_chip, "RAM"},
        {CAM_chip, "CAM"}
    };

    static constexpr auto defaultConfigValue = CAM_chip;
};

//==================================================

template<>
struct EnumInfo<OptimizationTarget> {
    static constexpr auto name = "OptimizationTarget";

    static inline const EnumFromYamlMap<OptimizationTarget> yamlMap = {
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

    static inline const EnumToStringMap<OptimizationTarget> stringMap = {
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

    static constexpr auto defaultConfigValue = full_exploration;
};

//==================================================

template<>
struct EnumInfo<CacheAccessMode> {
    static constexpr auto name = "CacheAccessMode";

    static inline const EnumFromYamlMap<CacheAccessMode> yamlMap = {
        {"Normal", normal_access_mode},
        {"Sequential", sequential_access_mode},
        {"Fast", fast_access_mode}
    };

    static inline const EnumToStringMap<CacheAccessMode> stringMap = {
        {normal_access_mode, "Normal"},
        {sequential_access_mode, "Sequential"},
        {fast_access_mode, "Fast"}
    };

    static constexpr auto defaultConfigValue = normal_access_mode;
};

//==================================================

template<typename T>
std::string enumToString(const T& v) {
    const auto& map = EnumInfo<T>::stringMap;
    const auto it = map.find(v);
    if (it == map.cend()) {
        throw std::runtime_error("Missing enum to string conversion rule");
    }
    return it->second;
}

template<typename T>
T enumToYaml(const T& v) {
    const auto& map = EnumInfo<T>::yamlMap;
    for(const auto it = map.cbegin(); it != map.cend(); ++it) {
        if (it->second == v) {
            return it->first;
        }
    }
    throw std::runtime_error("Missing enum to YAML string conversion rule");
}

template<typename T>
typename std::enable_if<EnumIsMapped<T>::value, std::ostream&>::type operator<<(std::ostream& stream, const T& v) {
    stream << enumToString<T>(v);
    return stream;
}

template<typename T>
bool yamlValueFromNode(T& var, const YAML::Node& node, const std::string& key) {
    if constexpr (EnumIsMapped<T>::value) {
        // one of the mapped enum types, first get the string and then do enum mapping magic
        std::string enumString;
        if (node[key]) {
            enumString = node[key].as<std::string>();
            const auto it = EnumInfo<T>::yamlMap.find(enumString);
            if (it == EnumInfo<T>::yamlMap.cend()) {
                if constexpr (EnumHasDefault<T>::value) {
                    var = EnumInfo<T>::defaultConfigValue;
                    return true;
                } else {
                    throw std::runtime_error(EnumInfo<T>::configError);
                }
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
