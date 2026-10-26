#include "component/BankWithHtree.hpp"
#include "global.hpp"

#include <cmath>

mse::component::BankWithHtree::BankWithHtree(
    int matRows,
    int matColumns,
    long long capacity,
    long blockSize,
    int associativity,
    int rowsPerSet,
    int activeMatsPerRow,
    int activeMatsPerColumn,
    int bitlineMuxInputs,
    bool internalSenseAmp,
    int senseAmpMuxLevel1Inputs,
    int senseAmpMuxLevel2Inputs,
    int subarrayRows,
    int subarrayColumns,
    int activeSubarraysPerRow,
    int activeSubarraysPerColumn,
    BufferDesignTarget areaOptimizationLevel,
    MemoryType memoryType)
        : Bank(matRows, matColumns, capacity, blockSize, associativity, rowsPerSet, activeMatsPerRow, activeMatsPerColumn,
               bitlineMuxInputs, internalSenseAmp, senseAmpMuxLevel1Inputs, senseAmpMuxLevel2Inputs, subarrayRows, subarrayColumns,
               activeSubarraysPerRow, activeSubarraysPerColumn, areaOptimizationLevel, memoryType) {}

bool mse::component::BankWithHtree::setup() noexcept {
    // TODO no external sense amps with H-tree routing
    if (!internalSenseAmp) {
        warn("H-tree routing does not support external sense amps");
        return false;
    }

    // Signals that have to be routed, using double to avoid overflow
    int addressBitsToRoute = static_cast<int>(std::log2(static_cast<double>(capacity) / blockSize / associativity) + 0.1);
    int dataDistributeBitsToRoute = 0;
    int dataBroadcastBitsToRoute = blockSize;
    if (memoryType == dataT) {
        dataDistributeBitsToRoute = blockSize;
        dataBroadcastBitsToRoute = static_cast<int>(std::log2(associativity)); // TODO this is not the only way
    } else if (memoryType == tag) {
        dataDistributeBitsToRoute = associativity; // TODO it seems to only support powers of 2
    }

    _horizontalLevels = static_cast<int>(std::log2(this->matColumns) + 0.1);
    _verticalLevels = static_cast<int>(std::log2(this->matRows) + 0.1);

    int h = _horizontalLevels;
    int v = _verticalLevels;
    int rowsToActivate = this->activeMatsPerColumn;
    int columnsToActivate = this->activeMatsPerRow;
    const auto nothingToRoute = [&dataDistributeBitsToRoute, &dataBroadcastBitsToRoute, &addressBitsToRoute] {
        return dataDistributeBitsToRoute + dataBroadcastBitsToRoute == 0 || addressBitsToRoute == 0;
    };
    const auto routedBits = [&dataDistributeBitsToRoute, &dataBroadcastBitsToRoute, &addressBitsToRoute] (HtreeLevel& level) {
        level.addressBits = addressBitsToRoute;
        level.dataDistributeBits = dataDistributeBitsToRoute;
        level.dataBroadcastBits = dataBroadcastBitsToRoute;
    };

    // Always route horizontally first, TODO this constraint is not valid
    if (h > 0) {
        if (nothingToRoute()) {
            return false;
        }

        routedBits(_horizontal[0]);
        _horizontal[0].wires = 1;
        _horizontal[0].totalWires = 1;
        _horizontal[0].activeWires = 1;
        h--;
    }

    int horizontalWireTiers = 1;
    int verticalWireTiers = 1;

    // Reduce the horizontal levels down to the vertical levels
    while (h > v) {
        if (nothingToRoute()) {
            return false;
        }

        HtreeLevel& level = _horizontal[_horizontalLevels - h];
        const HtreeLevel& previous = _horizontal[_horizontalLevels - h - 1];
        // Reduce the data bits when possible
        if (columnsToActivate > 1) {
            dataDistributeBitsToRoute /= 2;
            columnsToActivate /= 2;
            level.activeWires = 2 * previous.activeWires;
        } else {
            addressBitsToRoute--;
            level.activeWires = previous.activeWires;
        }
        routedBits(level);
        level.wires = 1;
        level.totalWires = 2 * previous.totalWires;
        h--;
        verticalWireTiers *= 2;
    }

    // Reduce the vertical levels down to the horizontal levels
    while (v > h) {
        if (nothingToRoute()) {
            return false;
        }

        HtreeLevel& level = _vertical[_verticalLevels - v];
        const bool first = v == _verticalLevels;
        // Reduce the data bits when possible
        if (rowsToActivate > 1) {
            dataDistributeBitsToRoute /= 2;
            rowsToActivate /= 2;
            level.activeWires = first ? 2 : 2 * _vertical[_verticalLevels - v - 1].activeWires;
        } else {
            addressBitsToRoute--;
            level.activeWires = first ? 1 : _vertical[_verticalLevels - v - 1].activeWires;
        }
        routedBits(level);
        level.wires = 1;
        level.totalWires = first ? 2 : 2 * _vertical[_verticalLevels - v - 1].totalWires;
        v--;
        horizontalWireTiers *= 2;
    }

    // Reduce both down to zero
    while (h > 0) {
        if (nothingToRoute()) {
            return false;
        }

        HtreeLevel& horizontal = _horizontal[_horizontalLevels - h];
        const bool noVertical = v == _verticalLevels;
        // The previous level is horizontal until the first vertical level is routed
        const HtreeLevel& previous = noVertical ? _horizontal[_horizontalLevels - h - 1] : _vertical[_verticalLevels - v - 1];
        // Reduce the data bits when possible
        if (columnsToActivate > 1) {
            dataDistributeBitsToRoute /= 2;
            columnsToActivate /= 2;
            horizontal.activeWires = 2 * previous.activeWires;
        } else {
            addressBitsToRoute--;
            horizontal.activeWires = previous.activeWires;
        }
        routedBits(horizontal);
        horizontal.wires = horizontalWireTiers;
        horizontal.totalWires = 2 * previous.totalWires;

        if (nothingToRoute()) {
            return false;
        }

        HtreeLevel& vertical = _vertical[_verticalLevels - v];
        // Reduce the data bits on the vertical level when possible
        if (rowsToActivate > 1) {
            dataDistributeBitsToRoute /= 2;
            rowsToActivate /= 2;
            vertical.activeWires = 2 * horizontal.activeWires;
        } else {
            addressBitsToRoute--;
            vertical.activeWires = horizontal.activeWires;
        }
        routedBits(vertical);
        vertical.wires = _horizontalLevels == 2 ? verticalWireTiers : 2 * verticalWireTiers;
        vertical.totalWires = 2 * horizontal.totalWires;
        h--;
        v--;
        horizontalWireTiers *= 2;
        verticalWireTiers *= 2;
    }

    if (nothingToRoute()) {
        return false;
    }

    if (columnsToActivate > 1) {
        dataDistributeBitsToRoute /= 2;
        columnsToActivate /= 2;
    } else if (_horizontalLevels > 0) {
        addressBitsToRoute--;
    }

    // A cache data mat needs enough ways to spread a set over multiple rows
    if (memoryType == dataT && this->rowsPerSet > static_cast<int>(std::pow(2, dataBroadcastBitsToRoute))) {
        return false;
    }

    if (memoryType == tag) {
        // Tag arrays cannot share bitlines between rows of a set
        if (this->rowsPerSet > 1) {
            return false;
        }
        // The mat does not hold at least one way
        if (dataDistributeBitsToRoute == 0) {
            return false;
        }
    }

    long matBlockSize = dataBroadcastBitsToRoute;
    int ways = 1;
    MuxInputs muxInputs{this->bitlineMuxInputs, this->senseAmpMuxLevel1Inputs, this->senseAmpMuxLevel2Inputs};
    if (memoryType == dataT) {
        // The distributed bits are the part of a data word assigned to this mat
        matBlockSize = dataDistributeBitsToRoute;
        ways = static_cast<int>(std::pow(2, dataBroadcastBitsToRoute));
        // At least 1, otherwise the bank is already invalid
        muxInputs = muxInputsForWays(ways / this->rowsPerSet);
    } else if (memoryType == tag) {
        // The broadcast bits are the tag width, the distributed bits are the ways assigned to this mat
        ways = dataDistributeBitsToRoute;
    }

    if (!build(mat, this->subarrayRows, this->subarrayColumns, addressBitsToRoute, matBlockSize, ways, this->rowsPerSet,
            this->activeSubarraysPerRow, this->activeSubarraysPerColumn, muxInputs.bitline, internalSenseAmp,
            muxInputs.senseAmpLevel1, muxInputs.senseAmpLevel2, areaOptimizationLevel, memoryType)) {
        return false;
    }

    return meetsAspectRatio();
}

mse::component::BankWithHtree::HtreeWires mse::component::BankWithHtree::calculateWires() const noexcept {
    using namespace mse::unit;

    HtreeWires wires;
    // Without repeaters the wires are on another metal layer and take no silicon area
    if (gGlobalWire.wireRepeaterType != repeated_none) {
        wires.wiresSharingWidth = static_cast<int>(std::floor(gGlobalWire.repeaterSpacing / gGlobalWire.repeaterHeight));
        wires.effectivePitch = Meter(gGlobalWire.repeatedWirePitch);
    }

    const Meter matWidth = mat.dimensions().width;
    const Meter matHeight = mat.dimensions().height;
    const auto routingWidth = [&wires] (int bits) {
        return std::ceil(static_cast<double>(bits) / wires.wiresSharingWidth) * wires.effectivePitch / 2;
    };

    int h = _horizontalLevels - 1;
    int v = _verticalLevels - 1;
    while (v > h) {
        if (v == _verticalLevels - 1) {
            wires.verticalLengths[v] = matHeight / 2;
        } else {
            wires.verticalLengths[v] = wires.verticalLengths[v + 1] * 2;
        }
        v--;
    }

    while (v >= 0) {
        if (v == _verticalLevels - 1) {
            wires.verticalLengths[v] = matHeight / 2;
        } else if (h == _horizontalLevels - 1) {
            wires.verticalLengths[v] = wires.verticalLengths[v + 1] * 2;
        } else {
            wires.verticalLengths[v] = wires.verticalLengths[v + 1] * 2 + routingWidth(_horizontal[h + 1].bits());
        }

        if (h == _horizontalLevels - 1) {
            wires.horizontalLengths[h] = matWidth;
            for (int i = v; i < _verticalLevels; i++) {
                wires.horizontalLengths[h] += routingWidth(_vertical[i].bits());
            }
        } else {
            wires.horizontalLengths[h] = wires.horizontalLengths[h + 1] * 2 + routingWidth(_vertical[v].bits());
        }
        v--;
        h--;
    }

    while (h >= 0) {
        if (h == _horizontalLevels - 1) {
            wires.horizontalLengths[h] = matWidth;
        } else {
            wires.horizontalLengths[h] = wires.horizontalLengths[h + 1] * 2;
        }
        h--;
    }

    return wires;
}

mse::Dimensions mse::component::BankWithHtree::calculateDimensions() const noexcept {
    using namespace mse::unit;

    Meter height = mat.dimensions().height * matRows;
    Meter width = mat.dimensions().width * matColumns;

    // Wire area
    const HtreeWires& wires = this->wires();
    for (int i = 0; i < _horizontalLevels; i++) {
        height += std::ceil(static_cast<double>(_horizontal[i].bits()) * _horizontal[i].wires / wires.wiresSharingWidth) * wires.effectivePitch;
    }
    for (int i = 0; i < _verticalLevels; i++) {
        width += std::ceil(static_cast<double>(_vertical[i].bits()) * _vertical[i].wires / wires.wiresSharingWidth) * wires.effectivePitch;
    }

    return Dimensions{
        .width = width,
        .height = height
    };
}

mse::component::PathTiming mse::component::BankWithHtree::calculateTiming() const noexcept {
    using namespace mse::unit;

    mat.setInputRamp(constant::infiniteRamp);
    const MatTiming& matTiming = mat.timing();
    const cell::OperationSet supported = operations();

    PathTiming timing;
    for (cell::Operation operation : cell::allOperations) {
        if (!supported.contains(operation)) {
            continue;
        }

        timing.latency[operation] = matTiming.latency[operation];
        if (operation == cell::Operation::Refresh) {
            timing.latency[operation] *= matColumns; // Every mat
        }
    }

    const HtreeWires& wires = this->wires();
    const auto addWire = [&timing, &supported] (Meter length) {
        double latency;
        double energy;
        double leakage;
        gGlobalWire.CalculateLatencyAndPower(length.value(), &latency, &energy, &leakage);
        for (cell::Operation operation : cell::allOperations) {
            if (!supported.contains(operation)) {
                continue;
            }

            switch (operation) {
                case cell::Operation::Read:
                    timing.latency[operation] += Second(latency * 2); // In and out
                    break;
                case cell::Operation::Write:
                case cell::Operation::Set:
                case cell::Operation::Reset:
                case cell::Operation::Refresh:
                    timing.latency[operation] += Second(latency); // Only in
                    break;
            }
        }
    };
    for (int i = 0; i < _horizontalLevels; i++) {
        addWire(wires.horizontalLengths[i]);
    }
    for (int i = 0; i < _verticalLevels; i++) {
        addWire(wires.verticalLengths[i]);
    }

    return timing;
}

mse::component::PathPower mse::component::BankWithHtree::calculatePower() const noexcept {
    using namespace mse::unit;

    const PathPower& matPower = mat.power();
    const cell::OperationSet supported = operations();

    PathPower power;
    for (cell::Operation operation : cell::allOperations) {
        if (!supported.contains(operation)) {
            continue;
        }

        if (operation == cell::Operation::Refresh) {
            power.dynamicEnergy[operation] = matPower.dynamicEnergy[operation] * matRows * matColumns; // Every mat
        } else {
            power.dynamicEnergy[operation] = matPower.dynamicEnergy[operation] * activeMatsPerRow * activeMatsPerColumn;
        }
    }
    power.leakage = matPower.leakage * matRows * matColumns;

    // A fast access cache only activates 1 / associativity of the wires on a write
    int writeFraction = 1;
    if (gInputParameter.designTarget == cache && gInputParameter.cacheAccessMode == fast_access_mode) {
        writeFraction = gInputParameter.associativity;
    }

    const HtreeWires& wires = this->wires();
    // Each wire is activated on exactly one way, so read and write energy are the same
    const auto addWire = [&power, &supported, writeFraction] (Meter length, const HtreeLevel& level) {
        double latency;
        double energy;
        double leakage;
        gGlobalWire.CalculateLatencyAndPower(length.value(), &latency, &energy, &leakage);
        for (cell::Operation operation : cell::allOperations) {
            if (!supported.contains(operation)) {
                continue;
            }

            switch (operation) {
                case cell::Operation::Read:
                    power.dynamicEnergy[operation] += Joule(energy * level.activeWires * level.bits());
                    break;
                case cell::Operation::Write:
                case cell::Operation::Set:
                case cell::Operation::Reset:
                case cell::Operation::Refresh:
                    power.dynamicEnergy[operation] += Joule(energy * level.activeWires * level.bits() / writeFraction);
                    break;
            }
        }
        power.leakage += Watt(leakage * level.totalWires * level.bits());
    };
    for (int i = 0; i < _horizontalLevels; i++) {
        addWire(wires.horizontalLengths[i], _horizontal[i]);
    }
    for (int i = 0; i < _verticalLevels; i++) {
        addWire(wires.verticalLengths[i], _vertical[i]);
    }

    return power;
}
