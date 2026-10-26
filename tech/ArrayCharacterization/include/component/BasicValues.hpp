// Basic timing and power structs for use in component return types
// These represent general properties each component will have
// Adding or removing members from these structs adds or removes them for all components

#ifndef MSE_COMPONENT_BASICVALUES_HPP
#define MSE_COMPONENT_BASICVALUES_HPP

#include "unit.hpp"
#include "cell/Operation.hpp"

namespace mse::component {

struct BasicTiming {
    unit::Second readLatency;
    unit::Second writeLatency;
};

struct BasicPower {
    unit::Joule dynamicReadEnergy;
    unit::Joule dynamicWriteEnergy;
    unit::Watt leakage;
};

// Timing and power of the components on a cell's access path, with one value for each cell operation
// Only the entries of the operations the cell supports are computed, the rest stay zero

struct PathTiming {
    cell::OperationValues<unit::Second> latency;
};

struct PathPower {
    cell::OperationValues<unit::Joule> dynamicEnergy;
    unit::Watt leakage;
};

} // namespace mse::component

#endif
