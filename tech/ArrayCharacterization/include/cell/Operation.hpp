// This file defines the cell operations that values along a cell access path are computed for
// A cell declares the operations it supports in its properties
// The components on the cell's access path then compute their latency and energy for each supported operation
//
// Read and Write are the basic accesses
// Set and Reset are the two writes of cells that switch between states, where a write is one or the other
//   SLC NAND programs with Set and erases with Reset
// Refresh reads and writes back every row, for cells that lose their data over time

#ifndef MSE_CELL_OPERATION_HPP
#define MSE_CELL_OPERATION_HPP

#include <array>
#include <initializer_list>
#include <string_view>
#include <utility>

namespace mse::cell {

enum class Operation {
    Read,
    Write,
    Set,
    Reset,
    Refresh
};

// Every operation in order, for looping over operations
inline constexpr std::array allOperations{Operation::Read, Operation::Write, Operation::Set, Operation::Reset, Operation::Refresh};

// The name of an operation for output
constexpr std::string_view operationName(Operation operation) noexcept {
    switch (operation) {
        case Operation::Read:
            return "Read";
        case Operation::Write:
            return "Write";
        case Operation::Set:
            return "Set";
        case Operation::Reset:
            return "Reset";
        case Operation::Refresh:
            return "Refresh";
    }

    return "";
}

//==================================================
// One value for each operation, indexed by the operation
// Every value starts as its type's default, which is zero for unit types

template <typename T>
class OperationValues {
private:

    std::array<T, allOperations.size()> _values;

public:

    constexpr OperationValues() noexcept {
        _values.fill(T());
    }

    constexpr T& operator[](Operation operation) noexcept { return _values[std::to_underlying(operation)]; }
    constexpr const T& operator[](Operation operation) const noexcept { return _values[std::to_underlying(operation)]; }
};

//==================================================
// A set of operations, like the operations a cell supports

class OperationSet {
private:

    unsigned _bits = 0;

public:

    constexpr OperationSet() noexcept = default;

    constexpr OperationSet(std::initializer_list<Operation> supported) noexcept {
        for (Operation operation : supported) {
            _bits |= 1u << std::to_underlying(operation);
        }
    }

    constexpr bool contains(Operation operation) const noexcept {
        return _bits & (1u << std::to_underlying(operation));
    }
};

} // namespace mse::cell

#endif
