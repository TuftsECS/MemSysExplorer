// This file contains visitor types for using on each cell type
// Visitors (within the visitor pattern) have overloaded functions for each type they operate on
// Each visitor is responsible for doing one type of computation, and they all must return the same type

#ifndef MSE_CELL_VISITORS_HPP
#define MSE_CELL_VISITORS_HPP

#include "unit.hpp"
#include "cell/types.hpp"

#include <functional>
#include <stdexcept>
#include <variant>

namespace mse {

// This struct adds a throwing overload for monostate types to use in visitors
// Inherit from this whenever a variant can hold a monostate type but should not ever encounter it
template <typename T>
struct MonostateThrow {
    [[noreturn]] T operator()(const std::monostate&) const {
        throw std::runtime_error("std::monostate value found in visitor");
    }
};

// Simple visitor type for small use
template <typename... Ts>
struct GenericVisitor : Ts... {
    using Ts::operator()...;
};

// Helper type for some return type T and overload target type U
// Useful with a pack expansion to create pure virtual visitor overloads on demand
template <typename T, typename U>
struct SingleVisitor {
    virtual T operator()(const U& target) = 0;
};

// TODO move somewhere else
template <typename T>
static inline constexpr T* variantBasePtr(auto& variant) {
    return std::visit(
        [](auto& value) -> T* {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::monostate>) {
                return nullptr;
            } else {
                return &value;
            }
        }, variant);
}

} // namespace mse

namespace mse::cell {

// Helper type for creating visitors from the MemoryCellList in include/cell/types.hpp
template <typename, typename>
struct MemoryCellVisitor;

// This specialization allows for simple visitor construction from a list of cell types
// It assumes monostate types should not be allowed
// Pure virtual functions for type overloads are automatically added with the SingleVisitor expansion
template <typename T, typename... Us>
struct MemoryCellVisitor<T, CellList<Us...>> : MonostateThrow<T>, SingleVisitor<T, Us>... {
    using MonostateThrow<T>::operator();
    using SingleVisitor<T, Us>::operator()...;
    virtual ~MemoryCellVisitor() = default;
};

} // namespace mse::cell

#endif
