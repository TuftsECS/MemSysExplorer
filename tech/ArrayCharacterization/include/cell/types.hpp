// This file declares general types used for MemoryCell operations
// The goal is to have a single place to add cell types that will propagate through the program
// Helper structs below use the type system to automatically build other useful types

#ifndef MSE_CELL_TYPES_HPP
#define MSE_CELL_TYPES_HPP

#include "input.hpp"
#include "log.hpp"

#include <format>
#include <type_traits>
#include <variant>
#include <string>

// Include custom cells here to use them for the MemoryCellList
#include "cell/Sram6t.hpp"
#include "cell/Dram.hpp"
#include "cell/Edram.hpp"
#include "cell/Edram3t.hpp"
#include "cell/Edram3t333.hpp"
#include "cell/Mram.hpp"
#include "cell/Pcram.hpp"
#include "cell/Memristor.hpp"
#include "cell/Fbram.hpp"
#include "cell/SlcNand.hpp"
#include "cell/Ctt.hpp"
#include "cell/Fefet.hpp"

namespace mse::cell {

// Concept for making sure that types being used as cells have required subtypes
// In order for a type to be a considered a valid cell type it must
//   Have a Parameters type
//   Have a static name member which can be converted to std::string
//   Be derived from MemoryCell
template <typename T>
concept IsCellType = requires {
    // Types required to be within each derived cell:
    typename T::Parameters;

    {T::name} -> std::convertible_to<std::string_view>; // Cells need to have a name string
} && std::derived_from<T, MemoryCell>; // And be derived from MemoryCell

// The CellList type is just a type helper for gathering each cell type in the program
template <IsCellType... Ts>
struct CellList {};

//==================================================
// Add custom cell types derived from mse::cell::MemoryCell here to this list
// Types placed in this CellList pack are used to derive other types below automatically

using MemoryCellList = CellList<
    Sram6tCell,
    DramCell,
    EdramCell,
    Edram3tCell,
    Edram3t333Cell,
    MramCell,
    PcramCell,
    MemristorCell,
    FbramCell,
    SlcNandCell,
    CttCell,
    FefetCell
>;

// MemoryCellInfo creates new types by using each cell type in a CellList
// Specifically, the MemoryCellList type above holding each cell
template <typename>
struct MemoryCellInfo;

template <IsCellType... Ts>
struct MemoryCellInfo<CellList<Ts...>> {
    using Variant = std::variant<std::monostate, Ts...>; // Holds any one cell
};

//==================================================
// Using each cell type in the MemoryCellList with the MemoryCellInfo type to actually extract these new types

using MemoryCellVariant = MemoryCellInfo<MemoryCellList>::Variant;

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
            log::fatal(std::format("No memory cell with name \"{}\" found", cellName.value()));
        }
    }

};

//==================================================
// The memory cell every array in a run is built from, created by MemoryCellFactory before the search
// TODO move into the run once it has its own type

inline MemoryCellVariant runCell;

// The cell in the variant as its MemoryCell base, nullptr when the variant holds no cell
template <typename T>
constexpr T* variantBasePtr(auto& variant) {
    return std::visit(
        [](auto& value) -> T* {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::monostate>) {
                return nullptr;
            } else {
                return &value;
            }
        }, variant);
}

// The run's memory cell, which has to be created before any array is built
inline const MemoryCell& memoryCell() noexcept {
    const MemoryCell* cell = variantBasePtr<const MemoryCell>(runCell);
    if (!cell) {
        log::fatal("No memory cell has been created");
    }

    return *cell;
}

} // namespace mse::cell

#endif
