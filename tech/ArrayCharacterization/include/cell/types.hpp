// This file declares general types used for MemoryCell operations
// The goal is to have a single place to add cell types that will propagate through the program
// Helper structs below use the type system to automatically build other useful types

#ifndef MSE_CELL_TYPES_HPP
#define MSE_CELL_TYPES_HPP

#include "input.hpp"
#include "log.hpp"

#include <variant>
#include <string>

// Include custom cells here to use them for the MemoryCellList
#include "cell/Sram6t.hpp"
#include "cell/Dram.hpp"
#include "cell/Edram.hpp"
#include "cell/Edram3t.hpp"
#include "cell/Edram3t333.hpp"

namespace mse::cell {

// Concept for making sure that types being used as cells have required subtypes
template <typename T>
concept IsCellType = requires {
    // Types required to be within each derived cell:
    typename T::Latency;
    typename T::Energy;
    typename T::Parameters;

    {T::name} -> std::convertible_to<std::string>; // Cells need to have a name string
} && std::derived_from<T, MemoryCell>;

// The CellList type is just a type helper for gathering each cell type in the program
template <IsCellType... Ts>
struct CellList {};

//==================================================
// Add custom cell types derived from mse::cell::MemoryCell here to this list
// Types placed in this CellList pack are used to derive other types below automatically

using MemoryCellList = CellList<Sram6tCell, DramCell, EdramCell, Edram3tCell, Edram3t333Cell>;

// MemoryCellInfo creates new types by using each cell type in a CellList
// Specifically, the MemoryCellList type above holding each cell
template <typename>
struct MemoryCellInfo;

template <IsCellType... Ts>
struct MemoryCellInfo<CellList<Ts...>> {
    using Variant = std::variant<std::monostate, Ts...>; // Holds any one cell
    using Latency = std::variant<typename Ts::Latency...>; // Holds any one Latency subtype for latency results
    using Energy = std::variant<typename Ts::Energy...>; // Holds any one Energy subtype for energy results
};

//==================================================
// Using each cell type in the MemoryCellList with the MemoryCellInfo type to actually extract these new types

using MemoryCellVariant = MemoryCellInfo<MemoryCellList>::Variant;
using MemoryCellLatency = MemoryCellInfo<MemoryCellList>::Latency;
using MemoryCellEnergy = MemoryCellInfo<MemoryCellList>::Energy;

//==================================================
// A helper factory type to emplace cells into an allocated MemoryCellVariant
// Reads from a cell configuration file and uses cellParameterField as the field where the cell name can be found

template <typename = MemoryCellList>
struct MemoryCellFactory;

template <typename... Ts>
struct MemoryCellFactory<CellList<Ts...>> {

    static void create(YamlInputFile& cellFile, const std::string& cellParameterField, MemoryCellVariant& cell) {
        cell.emplace<std::monostate>();

        ScalarParameter<std::string> cellName{cellParameterField};
        cellFile.fillParameter(cellName);

        ([&]() {
            if (std::holds_alternative<std::monostate>(cell) && Ts::name == cellName.value()) {
                cell.emplace<Ts>(typename Ts::Parameters(cellFile));
            }
        }(), ...);

        if (std::holds_alternative<std::monostate>(cell)) {
            outputLog.fatal("No memory cell with name \"{}\" found\n", cellName.value());
            mse::exit(mse::ExitCode::Failure);
        }
    }

};

} // namespace mse::cell

#endif
