#include "component/Mat.hpp"
#include "global.hpp"

#include <cmath>
#include <format>

mse::component::Mat::Mat(int subarrayRows, int subarrayColumns, int addressBits, long dataBits, int ways, int rowsPerSet, int activeSubarraysPerRow, int activeSubarraysPerColumn, int bitlineMuxInputs, bool internalSenseAmp, int senseAmpMuxLevel1Inputs, int senseAmpMuxLevel2Inputs, BufferDesignTarget areaOptimizationLevel, MemoryType memoryType)
        : _subarrayRows(subarrayRows),
          _subarrayColumns(subarrayColumns),
          _addressBits(addressBits),
          _dataBits(dataBits),
          _ways(ways),
          _rowsPerSet(rowsPerSet),
          _activeSubarraysPerRow(activeSubarraysPerRow),
          _activeSubarraysPerColumn(activeSubarraysPerColumn),
          _bitlineMuxInputs(bitlineMuxInputs),
          _internalSenseAmp(internalSenseAmp),
          _senseAmpMuxLevel1Inputs(senseAmpMuxLevel1Inputs),
          _senseAmpMuxLevel2Inputs(senseAmpMuxLevel2Inputs),
          _areaOptimizationLevel(areaOptimizationLevel),
          _memoryType(memoryType) {
    if (_activeSubarraysPerRow > _subarrayColumns) {
        warn(std::format("{} active subarrays per row is more than the {} subarrays per row", _activeSubarraysPerRow, _subarrayColumns));
        _activeSubarraysPerRow = _subarrayColumns;
    }
    if (_activeSubarraysPerColumn > _subarrayRows) {
        warn(std::format("{} active subarrays per column is more than the {} subarrays per column", _activeSubarraysPerColumn, _subarrayRows));
        _activeSubarraysPerColumn = _subarrayRows;
    }
}

void mse::component::Mat::forEachSubcomponent(const SubcomponentVisitor& visit) noexcept {
    visit(subarray);
    visit(_rowPredecoder1);
    visit(_rowPredecoder2);
    visit(_bitlineMuxPredecoder1);
    visit(_bitlineMuxPredecoder2);
    visit(_senseAmpMuxLevel1Predecoder1);
    visit(_senseAmpMuxLevel1Predecoder2);
    visit(_senseAmpMuxLevel2Predecoder1);
    visit(_senseAmpMuxLevel2Predecoder2);
    if (comparator) {
        visit(*comparator);
    }
}

bool mse::component::Mat::setup() noexcept {
    using namespace mse::unit;

    // Some address bits power gate the inactive subarrays, the rest decode within a subarray
    const int gatingAddressBits = static_cast<int>(std::log2(_subarrayRows * _subarrayColumns / _activeSubarraysPerColumn / _activeSubarraysPerRow) + 0.1);
    const int rowAddressBits = _addressBits - gatingAddressBits;
    // Too aggressive partitioning
    if (rowAddressBits <= 0) {
        return false;
    }

    long long rows = 1 << rowAddressBits;
    // Only a cache that partitions a set into multiple rows
    if (_memoryType == dataT) {
        rows *= _ways;
    }
    // Distribute to column decoding
    rows /= _bitlineMuxInputs * _senseAmpMuxLevel1Inputs * _senseAmpMuxLevel2Inputs;
    if (rows == 0) {
        return false;
    }

    // Columns depend on the access type
    long long columns = static_cast<long long>(_dataBits) / (_activeSubarraysPerRow * _activeSubarraysPerColumn);
    if (columns == 0) {
        return false;
    }

    columns *= _bitlineMuxInputs * _senseAmpMuxLevel1Inputs * _senseAmpMuxLevel2Inputs;
    if (_memoryType == tag) {
        columns *= _ways;
    }
    // Data array columns must be a multiple of 64 bits
    if (_memoryType == dataT && columns % 64 > 0) {
        return false;
    }

    // The predecoder loads depend on the subarray size
    if (!build(subarray, rows, columns, _rowsPerSet > 1, _bitlineMuxInputs, _internalSenseAmp,
            _senseAmpMuxLevel1Inputs, _senseAmpMuxLevel2Inputs, _areaOptimizationLevel)) {
        return false;
    }

    const int rowPredecoderAddressBits = rowAddressBits
            - static_cast<int>(std::log2(_bitlineMuxInputs * _senseAmpMuxLevel1Inputs * _senseAmpMuxLevel2Inputs) + 0.1);
    if (rowPredecoderAddressBits < 0) {
        return false;
    }

    // Assume the predecoders are at the center of the mat
    const Dimensions& subarrayDimensions = subarray.dimensions();
    const Unit<FaradAtom<>, MeterAtom<-1>> wireCapacitance{gLocalWire.capWirePerUnit};
    const Farad rowLoadCapacitance = subarrayDimensions.height * wireCapacitance * _subarrayRows / 2
            + subarrayDimensions.width * wireCapacitance * _subarrayColumns / 2;
    const Farad muxLoadCapacitance = std::max(Farad(), subarrayDimensions.height * wireCapacitance * (_subarrayRows - 2) / 2)
            + std::max(Farad(), subarrayDimensions.width * wireCapacitance * (_subarrayColumns - 2) / 2);

    // Cells with boosted wordlines drive the decoders to vpp
    const Volt swing = cell::memoryCell().properties().boostedWordline ? gTech.vpp : gTech.vdd;

    // A second predecoder block takes half of the address bits once one block would need more than 3
    auto buildPredecoders = [this, swing] (PredecodeBlock& first, PredecodeBlock& second, std::string_view name, int addressBits, Farad loadCapacitance) {
        const int secondBlockAddressBits = addressBits > 3 ? addressBits / 2 : 0;

        return build(std::string(name) + " 1", first, addressBits - secondBlockAddressBits, loadCapacitance, Ohm(), swing) // TODO load resistance
                && build(std::string(name) + " 2", second, secondBlockAddressBits, loadCapacitance, Ohm(), swing);
    };

    if (!buildPredecoders(_rowPredecoder1, _rowPredecoder2, "Row", rowPredecoderAddressBits, rowLoadCapacitance)) {
        return false;
    }

    if (!buildPredecoders(_bitlineMuxPredecoder1, _bitlineMuxPredecoder2, "Bitline Mux",
            static_cast<int>(std::log2(_bitlineMuxInputs) + 0.1), muxLoadCapacitance)) {
        return false;
    }

    if (!buildPredecoders(_senseAmpMuxLevel1Predecoder1, _senseAmpMuxLevel1Predecoder2, "Level 1 Mux",
            static_cast<int>(std::log2(_senseAmpMuxLevel1Inputs) + 0.1), muxLoadCapacitance)) {
        return false;
    }

    if (!buildPredecoders(_senseAmpMuxLevel2Predecoder1, _senseAmpMuxLevel2Predecoder2, "Level 2 Mux",
            static_cast<int>(std::log2(_senseAmpMuxLevel2Inputs) + 0.1), muxLoadCapacitance)) {
        return false;
    }

    if (_memoryType == tag && _internalSenseAmp) {
        return build(comparator, _dataBits, Farad()); // TODO fix the load
    }

    return true;
}

void mse::component::Mat::report(Reporter& reporter) const noexcept {
    RampedComponent::report(reporter);
    reportOperations(reporter, timing(), power());

    reporter.report(ReportGroup::Timing, "Predecoder Latency", timing().predecoderLatency);
}

void mse::component::Mat::reset() noexcept {
    RampedComponent::reset();
    forEachSubcomponent([] (Component& subcomponent) noexcept { subcomponent.reset(); });
}

mse::Dimensions mse::component::Mat::calculateDimensions() const noexcept {
    using namespace mse::unit;

    const MeterExp<2> predecoderArea = _rowPredecoder1.dimensions().area() + _rowPredecoder2.dimensions().area()
            + _bitlineMuxPredecoder1.dimensions().area() + _bitlineMuxPredecoder2.dimensions().area()
            + _senseAmpMuxLevel1Predecoder1.dimensions().area() + _senseAmpMuxLevel1Predecoder2.dimensions().area()
            + _senseAmpMuxLevel2Predecoder1.dimensions().area() + _senseAmpMuxLevel2Predecoder2.dimensions().area();
    Meter width = subarray.dimensions().width * _subarrayColumns;
    Meter height = subarray.dimensions().height * _subarrayRows;

    // Add the predecoders along the shorter side to avoid too much white space
    const Meter predecoderSide(std::sqrt(predecoderArea.value()));
    if (width > height) {
        width += predecoderSide;
    } else {
        height += predecoderSide;
    }

    if (comparator) {
        height += _ways * comparator->dimensions().area() / width;
    }

    return Dimensions{
        .width = width,
        .height = height
    };
}

mse::component::MatTiming mse::component::Mat::calculateTiming(physics::HorowitzRamp inputRamp) const noexcept {
    using namespace mse::unit;

    MatTiming timing;

    _rowPredecoder1.setInputRamp(inputRamp);
    _rowPredecoder2.setInputRamp(inputRamp);
    _bitlineMuxPredecoder1.setInputRamp(inputRamp);
    _bitlineMuxPredecoder2.setInputRamp(inputRamp);
    _senseAmpMuxLevel1Predecoder1.setInputRamp(inputRamp);
    _senseAmpMuxLevel1Predecoder2.setInputRamp(inputRamp);
    _senseAmpMuxLevel2Predecoder1.setInputRamp(inputRamp);
    _senseAmpMuxLevel2Predecoder2.setInputRamp(inputRamp);

    const Second rowPredecoderLatency = std::max(_rowPredecoder1.timing().readLatency, _rowPredecoder2.timing().readLatency);
    const Second bitlineMuxPredecoderLatency = std::max(_bitlineMuxPredecoder1.timing().readLatency, _bitlineMuxPredecoder2.timing().readLatency);
    const Second senseAmpMuxLevel1PredecoderLatency = std::max(_senseAmpMuxLevel1Predecoder1.timing().readLatency,
            _senseAmpMuxLevel1Predecoder2.timing().readLatency);
    const Second senseAmpMuxLevel2PredecoderLatency = std::max(_senseAmpMuxLevel2Predecoder1.timing().readLatency,
            _senseAmpMuxLevel2Predecoder2.timing().readLatency);
    timing.predecoderLatency = std::max({rowPredecoderLatency, bitlineMuxPredecoderLatency,
            senseAmpMuxLevel1PredecoderLatency, senseAmpMuxLevel2PredecoderLatency});

    subarray.setInputRamp(std::min(_rowPredecoder1.timing().outputRamp, _rowPredecoder2.timing().outputRamp));
    const SubArrayTiming& subarrayTiming = subarray.timing();

    const cell::OperationSet supported = operations();
    for (cell::Operation operation : cell::allOperations) {
        if (!supported.contains(operation)) {
            continue;
        }

        timing.latency[operation] = timing.predecoderLatency + subarrayTiming.latency[operation];
        switch (operation) {
            case cell::Operation::Read:
                // A tag mat compares the tag after the subarray read
                if (comparator) {
                    comparator->setInputRamp(inputRamp);
                    timing.latency[operation] += comparator->timing().readLatency;
                }
                break;
            case cell::Operation::Write:
            case cell::Operation::Set:
            case cell::Operation::Reset:
                break;
            case cell::Operation::Refresh:
                // Covering every subarray
                timing.latency[operation] *= _subarrayColumns;
                break;
        }
    }

    return timing;
}

mse::component::PathPower mse::component::Mat::calculatePower() const noexcept {
    using namespace mse::unit;

    const Joule predecoderReadEnergy = _rowPredecoder1.power().dynamicReadEnergy + _rowPredecoder2.power().dynamicReadEnergy
            + _bitlineMuxPredecoder1.power().dynamicReadEnergy + _bitlineMuxPredecoder2.power().dynamicReadEnergy
            + _senseAmpMuxLevel1Predecoder1.power().dynamicReadEnergy + _senseAmpMuxLevel1Predecoder2.power().dynamicReadEnergy
            + _senseAmpMuxLevel2Predecoder1.power().dynamicReadEnergy + _senseAmpMuxLevel2Predecoder2.power().dynamicReadEnergy;
    const Joule predecoderWriteEnergy = _rowPredecoder1.power().dynamicWriteEnergy + _rowPredecoder2.power().dynamicWriteEnergy
            + _bitlineMuxPredecoder1.power().dynamicWriteEnergy + _bitlineMuxPredecoder2.power().dynamicWriteEnergy
            + _senseAmpMuxLevel1Predecoder1.power().dynamicWriteEnergy + _senseAmpMuxLevel1Predecoder2.power().dynamicWriteEnergy
            + _senseAmpMuxLevel2Predecoder1.power().dynamicWriteEnergy + _senseAmpMuxLevel2Predecoder2.power().dynamicWriteEnergy;

    PathPower power;
    power.leakage = _rowPredecoder1.power().leakage + _rowPredecoder2.power().leakage
            + _bitlineMuxPredecoder1.power().leakage + _bitlineMuxPredecoder2.power().leakage
            + _senseAmpMuxLevel1Predecoder1.power().leakage + _senseAmpMuxLevel1Predecoder2.power().leakage
            + _senseAmpMuxLevel2Predecoder1.power().leakage + _senseAmpMuxLevel2Predecoder2.power().leakage;

    const PathPower& subarrayPower = subarray.power();
    const cell::OperationSet supported = operations();
    for (cell::Operation operation : cell::allOperations) {
        if (!supported.contains(operation)) {
            continue;
        }

        Joule& energy = power.dynamicEnergy[operation];
        switch (operation) {
            case cell::Operation::Read:
                energy = predecoderReadEnergy;
                energy += subarrayPower.dynamicEnergy[operation] * _activeSubarraysPerRow * _activeSubarraysPerColumn;
                if (comparator) {
                    energy += comparator->power().dynamicReadEnergy * _ways;
                }
                break;
            case cell::Operation::Write:
                energy = predecoderWriteEnergy;
                energy += subarrayPower.dynamicEnergy[operation] * _activeSubarraysPerRow * _activeSubarraysPerColumn;
                if (comparator) {
                    energy += comparator->power().dynamicWriteEnergy * _ways;
                }
                break;
            case cell::Operation::Set:
            case cell::Operation::Reset:
                energy = predecoderWriteEnergy + subarrayPower.dynamicEnergy[operation] * _activeSubarraysPerRow * _activeSubarraysPerColumn;
                break;
            case cell::Operation::Refresh:
                // The predecoder bits are broadcast, so every refreshed row pays the row predecoder energy once
                energy = _rowPredecoder1.power().dynamicReadEnergy + _rowPredecoder2.power().dynamicReadEnergy;
                energy *= subarray.rows * _subarrayRows;
                // Covering every subarray
                energy += subarrayPower.dynamicEnergy[operation] * _subarrayRows * _subarrayColumns;
                break;
        }
    }

    power.leakage += subarrayPower.leakage * _subarrayRows * _subarrayColumns;
    if (comparator) {
        power.leakage += comparator->power().leakage * _ways;
    }

    return power;
}
