// This file defines the CellPath interface for the components on a cell's access path
// The subarray, mat and bank compute their latency and energy for each cell operation, in PathTiming and PathPower
// Each one routes an operation through its own subcomponents, choosing which of their read or write values it uses
//   For example the subarray's refresh uses the read energy of its row decoder, precharger and sense amps
// Subcomponents off the path, like decoders and muxes, keep plain read and write values and know nothing of operations
// The operations come from the cell, and only the ones it supports are computed

#ifndef MSE_COMPONENT_CELLPATH_HPP
#define MSE_COMPONENT_CELLPATH_HPP

#include "cell/Operation.hpp"
#include "component/BasicValues.hpp"
#include "component/Reporter.hpp"

#include <format>

namespace mse::component {

class CellPath {
public:

    virtual ~CellPath() = default;

    // The cell operations this part of the access path computes values for
    virtual cell::OperationSet operations() const noexcept = 0;

protected:

    // Reports the latency and dynamic energy of each supported operation, then the leakage
    void reportOperations(Reporter& reporter, const PathTiming& timing, const PathPower& power) const noexcept {
        const cell::OperationSet supported = operations();
        for (cell::Operation operation : cell::allOperations) {
            if (supported.contains(operation)) {
                reporter.report(ReportGroup::Timing, std::format("{} Latency", cell::operationName(operation)), timing.latency[operation]);
            }
        }
        for (cell::Operation operation : cell::allOperations) {
            if (supported.contains(operation)) {
                reporter.report(ReportGroup::Power, std::format("Dynamic {} Energy", cell::operationName(operation)), power.dynamicEnergy[operation]);
            }
        }
        reporter.report(ReportGroup::Power, "Leakage", power.leakage);
    }
};

} // namespace mse::component

#endif
