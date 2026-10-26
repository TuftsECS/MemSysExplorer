#include "component/SubArray.hpp"
#include "formula.hpp"
#include "global.hpp"

#include <cmath>

mse::component::SubArray::SubArray(long long rows, long long columns, bool multipleRowsPerSet, int bitlineMuxInputs, bool internalSenseAmp, int senseAmpMuxLevel1Inputs, int senseAmpMuxLevel2Inputs, BufferDesignTarget areaOptimizationLevel)
        : _multipleRowsPerSet(multipleRowsPerSet),
          _bitlineMuxInputs(bitlineMuxInputs),
          _internalSenseAmp(internalSenseAmp),
          _senseAmpMuxLevel1Inputs(senseAmpMuxLevel1Inputs),
          _senseAmpMuxLevel2Inputs(senseAmpMuxLevel2Inputs),
          _areaOptimizationLevel(areaOptimizationLevel),
          rows(rows),
          columns(columns) {}

void mse::component::SubArray::forEachSubcomponent(const SubcomponentVisitor& visit) noexcept {
    visit(rowDecoder);
    if (readRowDecoder) {
        visit(*readRowDecoder);
    }
    visit(bitlineMuxDecoder);
    visit(senseAmpMuxLevel1Decoder);
    visit(senseAmpMuxLevel2Decoder);
    if (senseAmp) {
        visit(*senseAmp);
    }
    visit(precharger);
    visit(writeDriver);
    visit(bitlineMux);
    visit(senseAmpMuxLevel1);
    visit(senseAmpMuxLevel2);
    if (buffer) {
        visit(*buffer);
    }
}

bool mse::component::SubArray::setup() noexcept {
    using namespace mse::unit;

    const int temperatureIndex = gInputParameter.temperature - 300;

    const cell::MemoryCell& memoryCell = cell::memoryCell();
    const cell::MemoryCellTopology& topology = memoryCell.topology();
    const cell::MemoryCellProperties& properties = memoryCell.properties();

    // Each column holds part of the data in all the ways
    if (gInputParameter.designTarget == cache && gInputParameter.cacheAccessMode != sequential_access_mode
            && columns < gInputParameter.associativity) {
        return false;
    }

    // A destructive read can't share a sense amp between bitlines
    if (properties.destructiveRead && _bitlineMuxInputs > 1) {
        return false;
    }

    if (rows < properties.minimumRows) {
        return false;
    }

    // There is no room for the sense amp without a bitline mux
    if (properties.senseAmpNeedsBitlineMux && _internalSenseAmp && _bitlineMuxInputs < 2) {
        return false;
    }

    const int selectedColumns = columns / _bitlineMuxInputs / _senseAmpMuxLevel1Inputs / _senseAmpMuxLevel2Inputs;
    const std::optional<cell::ArrayCurrents> currents = memoryCell.arrayCurrents(rows, columns, selectedColumns);
    if (!currents) {
        return false;
    }
    _maxBitlineCurrent = currents->maxBitlineCurrent;
    const Ampere maxWordlineCurrent = currents->maxWordlineCurrent;

    const Meter minWordlineDriverWidth(CalculateLayoutWidth((maxWordlineCurrent / gTech.currentOnNmos[temperatureIndex]).value(), gTech));
    if (minWordlineDriverWidth > Feature(gInputParameter.maxNmosSize) * gTech.featureSize) {
        return false;
    }

    const Meter minBitlineMuxWidth = std::max(constant::minNmosSize * gTech.featureSize,
            Meter(CalculateLayoutWidth((_maxBitlineCurrent / gTech.currentOnNmos[temperatureIndex]).value(), gTech)));
    if (minBitlineMuxWidth > Feature(gInputParameter.maxNmosSize) * gTech.featureSize) {
        return false;
    }

    if (_internalSenseAmp) {
        _voltageSense = properties.sensingMethod == cell::MemoryCellProperties::SenseMethod::Voltage;
    } else if (properties.destructiveRead) {
        fatal("DRAM does not support external sense amplifiers");
    }

    const cell::MemoryCellParameters& parameters = memoryCell.parameters();
    _wordlineLength = columns * parameters.cellWidth * gTech.featureSize;
    _bitlineLength = rows * parameters.cellHeight * gTech.featureSize;
    if (parameters.stitching.value()) {
        _wordlineLength += ((columns - 1) / parameters.stitching.value() + 1) * constant::stitchingOverhead * gTech.featureSize;
    }
    _bitlineLength += memoryCell.bitlineOverhead(rows);

    const Unit<FaradAtom<>, MeterAtom<-1>> wireCapacitance{gLocalWire.capWirePerUnit};
    const Unit<OhmAtom<>, MeterAtom<-1>> wireResistance{gLocalWire.resWirePerUnit};
    _rc.wordlineCapacitance = _wordlineLength * wireCapacitance;
    _rc.wordlineResistance = _wordlineLength * wireResistance;
    _rc.bitlineCapacitance = _bitlineLength * wireCapacitance;
    _rc.bitlineResistance = _bitlineLength * wireResistance;

    const Ohm muxDecoderLoadResistance = _rc.wordlineResistance;
    Farad muxDecoderLoadCapacitance = Farad(CalculateGateCap(minBitlineMuxWidth.value(), gTech)) * columns;
    muxDecoderLoadCapacitance += _rc.wordlineCapacitance;

    _senseVoltage = memoryCell.senseVoltage(_rc.bitlineCapacitance);
    // Bitline too long
    if (_senseVoltage < parameters.minSenseVoltage.value()) {
        return false;
    }

    // Each cell loads its wordlines and bitlines with its largest line capacitance
    Farad cellWordlineCapacitance;
    for (int wordline = 0; wordline < static_cast<int>(topology.wordlineCount()); ++wordline) {
        cellWordlineCapacitance = std::max(cellWordlineCapacitance, topology.wordlineCapacitance(wordline));
    }
    Farad cellBitlineCapacitance;
    for (int bitline = 0; bitline < static_cast<int>(topology.bitlineCount()); ++bitline) {
        cellBitlineCapacitance = std::max(cellBitlineCapacitance, topology.bitlineCapacitance(bitline));
    }

    if (properties.separateReadWordline) {
        _rc.readWordlineCapacitance = _rc.wordlineCapacitance + topology.wordlineCapacitance(0) * columns;
        _rc.wordlineCapacitance += topology.wordlineCapacitance(1) * columns;
    } else {
        _rc.wordlineCapacitance += cellWordlineCapacitance * columns;
    }
    _rc.bitlineCapacitance += cellBitlineCapacitance * rows * (properties.sharedContact ? 0.5 : 1.0);
    _rc.prechargeVoltage = properties.prechargeVoltage;

    // The bank reads the resistive states for its global bitline model
    if (const auto* resistive = dynamic_cast<const cell::ResistiveCellTopology*>(&topology)) {
        _rc.cellOnResistance = resistive->resMemCellOn();
        _rc.cellOffResistance = resistive->resMemCellOff();
        _rc.equivalentOnResistance = resistive->resEquivalentOn();
        _rc.equivalentOffResistance = resistive->resEquivalentOff();
        _rc.cellOnVoltage = resistive->voltageMemCellOn();
        _rc.cellOffVoltage = resistive->voltageMemCellOff();

        // TODO a charge pump could raise the sense amp working point
        const bool voltageSensing = properties.sensingMethod == cell::MemoryCellProperties::SenseMethod::Voltage;
        if (voltageSensing && _rc.prechargeVoltage - _rc.cellOnVoltage <= _senseVoltage) {
            warn("No reasonable precharge voltage exists for the read current or voltage");

            return false;
        }
    }

    if (!build(precharger, gTech.vdd, columns, _rc.bitlineCapacitance, _rc.bitlineResistance, parameters.cellWidth * gTech.featureSize)) {
        return false;
    }

    // One write driver per column for a separate wordline row write, since the write wordline opens the whole row,
    // otherwise one per sense amp group, driving through the bitline mux
    const long long writeDrivers = properties.separateReadWordline ? columns : columns / _bitlineMuxInputs;
    if (!build("Write Driver", writeDriver, gTech.vdd, writeDrivers, _rc.bitlineCapacitance, _rc.bitlineResistance,
            parameters.cellWidth * gTech.featureSize)) {
        return false;
    }

    // Cells with boosted wordlines drive the decoders to vpp
    const Volt decoderSwing = properties.boostedWordline ? gTech.vpp : gTech.vdd;

    if (!build("Row", rowDecoder, rows, _rc.wordlineCapacitance, _rc.wordlineResistance, _multipleRowsPerSet, _areaOptimizationLevel, maxWordlineCurrent,
            decoderSwing)) {
        return false;
    }

    if (properties.separateReadWordline && !build("Read Row", readRowDecoder, rows, _rc.readWordlineCapacitance, _rc.wordlineResistance,
            _multipleRowsPerSet, _areaOptimizationLevel, maxWordlineCurrent, decoderSwing)) {
        return false;
    }

    // TODO fix the mux decoder loads
    if (!build("Bitline Mux", bitlineMuxDecoder, _bitlineMuxInputs, muxDecoderLoadCapacitance, muxDecoderLoadResistance, false, _areaOptimizationLevel, Ampere(),
            decoderSwing)) {
        return false;
    }

    if (!build("Level 1 Mux", senseAmpMuxLevel1Decoder, _senseAmpMuxLevel1Inputs, muxDecoderLoadCapacitance, muxDecoderLoadResistance, false,
            _areaOptimizationLevel, Ampere(), decoderSwing)) {
        return false;
    }

    if (!build("Level 2 Mux", senseAmpMuxLevel2Decoder, _senseAmpMuxLevel2Inputs, muxDecoderLoadCapacitance, muxDecoderLoadResistance, false,
            _areaOptimizationLevel, Ampere(), decoderSwing)) {
        return false;
    }

    // TODO fix the level 2 mux loads
    if (!build("Level 2", senseAmpMuxLevel2, _senseAmpMuxLevel2Inputs, columns / _bitlineMuxInputs / _senseAmpMuxLevel1Inputs / _senseAmpMuxLevel2Inputs,
            Farad(), Farad(), _maxBitlineCurrent, properties.maxMuxResistance)) {
        return false;
    }

    if (!build("Level 1", senseAmpMuxLevel1, _senseAmpMuxLevel1Inputs, columns / _bitlineMuxInputs / _senseAmpMuxLevel1Inputs,
            senseAmpMuxLevel2.rc().delayLoadCapacitance, senseAmpMuxLevel2.rc().powerLoadCapacitance, _maxBitlineCurrent, properties.maxMuxResistance)) {
        return false;
    }

    if (_internalSenseAmp) {
        if (!build(senseAmp, columns / _bitlineMuxInputs, !_voltageSense, _senseVoltage, _wordlineLength / columns * _bitlineMuxInputs,
                properties.multiLevel, properties.senseAmp)) {
            return false;
        }

        if (!build("Bitline", bitlineMux, _bitlineMuxInputs, columns / _bitlineMuxInputs,
                senseAmp->loadCapacitance(), senseAmp->loadCapacitance(), _maxBitlineCurrent, properties.maxMuxResistance)) {
            return false;
        }
    } else {
        if (!build("Bitline", bitlineMux, _bitlineMuxInputs, columns / _bitlineMuxInputs,
                senseAmpMuxLevel1.rc().delayLoadCapacitance, senseAmpMuxLevel1.rc().powerLoadCapacitance, _maxBitlineCurrent, properties.maxMuxResistance)) {
            return false;
        }
    }

    // TODO control signals are not buffered
    if (gInputParameter.differentialWrite) {
        return build(buffer, 1, columns);
    }

    return true;
}

void mse::component::SubArray::report(Reporter& reporter) const noexcept {
    RampedComponent::report(reporter);
    reportOperations(reporter, timing(), power());

    const SubArrayTiming& timing = this->timing();
    reporter.report(ReportGroup::Timing, "Bitline Latency", timing.bitlineLatency);
    reporter.report(ReportGroup::Timing, "Charge Latency", timing.chargeLatency);
    reporter.report(ReportGroup::Timing, "Column Decoder Latency", timing.columnDecoderLatency);

    // Only cells that model the energy spent in the cells themselves have it, and only for some operations
    const SubArrayPower& power = this->power();
    for (cell::Operation operation : cell::allOperations) {
        if (power.cellEnergy[operation] != unit::Joule()) {
            reporter.report(ReportGroup::Power, std::format("Cell {} Energy", cell::operationName(operation)), power.cellEnergy[operation]);
        }
    }
}

void mse::component::SubArray::reset() noexcept {
    RampedComponent::reset();
    forEachSubcomponent([] (Component& subcomponent) noexcept { subcomponent.reset(); });
}

mse::cell::OperationSet mse::component::SubArray::operations() const noexcept {
    return cell::memoryCell().properties().operations;
}

mse::Dimensions mse::component::SubArray::calculateDimensions() const noexcept {
    using namespace mse::unit;

    Meter width = _wordlineLength;
    Meter height = _bitlineLength;
    Meter addedWidth;
    Meter addedHeight;

    const Dimensions& rowDecoderDimensions = rowDecoder.dimensions();
    if (rowDecoderDimensions.height > height) {
        // Assume magic folding
        addedWidth = rowDecoderDimensions.area() / height;
    } else {
        // Allow white space
        addedWidth = rowDecoderDimensions.width;
    }
    // Height of the decoder strip, a folded decoder spanning the array
    Meter decoderHeight = std::min(rowDecoderDimensions.height, height);

    const Dimensions& prechargerDimensions = precharger.dimensions();
    if (prechargerDimensions.width > width) {
        // Assume magic folding
        addedHeight = prechargerDimensions.area() / prechargerDimensions.width;
    } else {
        // Allow white space
        addedHeight = prechargerDimensions.height;
    }

    const bool stackedOverPeripherals = cell::memoryCell().properties().stackedOverPeripherals;

    const Dimensions& writeDriverDimensions = writeDriver.dimensions();
    Meter writeDriverHeight;
    if (writeDriverDimensions.width > width) {
        // Assume magic folding
        writeDriverHeight = writeDriverDimensions.area() / writeDriverDimensions.width;
    } else {
        // Allow white space
        writeDriverHeight = writeDriverDimensions.height;
    }
    // Stacked cells put their write drivers under the array, see below
    if (!stackedOverPeripherals) {
        addedHeight += writeDriverHeight;
    }

    if (readRowDecoder) {
        const Dimensions& readRowDecoderDimensions = readRowDecoder->dimensions();
        if (readRowDecoderDimensions.height > height) {
            // Assume magic folding
            addedWidth += readRowDecoderDimensions.area() / height;
        } else {
            // Allow white space
            addedWidth += readRowDecoderDimensions.width;
        }
        decoderHeight = std::max(decoderHeight, std::min(readRowDecoderDimensions.height, height));
    }

    addedHeight += bitlineMux.dimensions().height;

    if (senseAmp) {
        const Dimensions& senseAmpDimensions = senseAmp->dimensions();
        if (senseAmpDimensions.width > width * 1.001) {
            error("Sense amp is wider than the subarray");
        } else {
            addedHeight += senseAmpDimensions.height;
        }
    }

    addedHeight += senseAmpMuxLevel1.dimensions().height;
    addedHeight += senseAmpMuxLevel2.dimensions().height;

    addedWidth = std::max(addedWidth, bitlineMuxDecoder.dimensions().width);
    addedWidth = std::max(addedWidth, senseAmpMuxLevel1Decoder.dimensions().width);
    addedWidth = std::max(addedWidth, senseAmpMuxLevel2Decoder.dimensions().width);
    decoderHeight = std::max({decoderHeight, bitlineMuxDecoder.dimensions().height, senseAmpMuxLevel1Decoder.dimensions().height,
            senseAmpMuxLevel2Decoder.dimensions().height});

    // Assume the buffer is as wide as the subarray
    if (buffer) {
        addedHeight += buffer->dimensions().height;
    }

    if (stackedOverPeripherals) {
        // Monolithic 3D cells are above the peripherals, so the decoders and write drivers sit under the cell array and add width
        // or height only where they do not fit within it, and the pitch matched precharger, sense amps and muxes stay next to it
        width = std::max(_wordlineLength, addedWidth);
        height = std::max(_bitlineLength, decoderHeight + writeDriverHeight) + addedHeight;
    } else {
        width += addedWidth;
        height += addedHeight;
    }

    return Dimensions{
        .width = width,
        .height = height
    };
}

mse::component::SubArrayTiming mse::component::SubArray::calculateTiming(physics::HorowitzRamp inputRamp) const noexcept {
    using namespace mse::unit;

    SubArrayTiming timing;

    Second senseAmpLatency;
    if (senseAmp) {
        senseAmpLatency = senseAmp->timing().readLatency;
    }

    precharger.setInputRamp(inputRamp);
    writeDriver.setInputRamp(inputRamp);
    rowDecoder.setInputRamp(inputRamp);
    if (readRowDecoder) {
        readRowDecoder->setInputRamp(inputRamp);
    }
    bitlineMuxDecoder.setInputRamp(inputRamp);
    senseAmpMuxLevel1Decoder.setInputRamp(inputRamp);
    senseAmpMuxLevel2Decoder.setInputRamp(inputRamp);

    timing.columnDecoderLatency = std::max({bitlineMuxDecoder.timing().readLatency, senseAmpMuxLevel1Decoder.timing().readLatency,
            senseAmpMuxLevel2Decoder.timing().readLatency});
    const Second decoderLatency = std::max(rowDecoder.timing().readLatency, timing.columnDecoderLatency);
    // Cells with a separate read wordline read through its row decoder
    const Second readDecoderLatency = readRowDecoder ? std::max(readRowDecoder->timing().readLatency, timing.columnDecoderLatency) : decoderLatency;

    // TODO revisit this equation
    const Farad passCapacitance = bitlineMux.rc().passCapacitance + senseAmpMuxLevel1.rc().passCapacitance + senseAmpMuxLevel2.rc().passCapacitance;
    const Ohm passResistance = bitlineMux.rc().passResistance + senseAmpMuxLevel1.rc().passResistance + senseAmpMuxLevel2.rc().passResistance;
    const Second chargeTau = passResistance * (passCapacitance + _rc.bitlineCapacitance) + _rc.bitlineResistance * _rc.bitlineCapacitance / 2;
    timing.chargeLatency = physics::horowitz(chargeTau, 0, physics::stepRamp).delay;

    const Farad muxLoadCapacitance = bitlineMux.rc().delayLoadCapacitance;

    const cell::MemoryCell& memoryCell = cell::memoryCell();

    cell::MemoryCellLatencyContext context;
    context.bitlineCapacitance = _rc.bitlineCapacitance;
    context.bitlineResistance = _rc.bitlineResistance;
    context.bitlineMuxCapacitance = muxLoadCapacitance;
    context.rowDecoderRampOutput = rowDecoder.timing().outputRamp;
    context.bitlineMuxReadLatency = bitlineMux.timing().readLatency;
    context.rowDecoderReadLatency = rowDecoder.timing().readLatency;
    context.rowDecoderWriteLatency = rowDecoder.timing().writeLatency;
    context.readRowDecoderRampOutput = readRowDecoder ? readRowDecoder->timing().outputRamp : rowDecoder.timing().outputRamp;
    context.readDecoderLatency = readDecoderLatency;
    context.writeDriverLatency = writeDriver.timing().readLatency;
    context.columnDecoderLatency = timing.columnDecoderLatency;
    context.chargeLatency = timing.chargeLatency;
    context.senseAmpReadLatency = senseAmpLatency;
    context.senseAmpMuxLev1ReadLatency = senseAmpMuxLevel1.timing().readLatency;
    context.senseAmpMuxLev2ReadLatency = senseAmpMuxLevel2.timing().readLatency;
    context.senseVoltage = _senseVoltage;
    context.numColumn = columns;
    context.numRow = rows;

    const cell::CellTiming cellTiming = memoryCell.timing(context);
    timing.bitlineLatency = cellTiming.bitline;
    timing.bitlineReadLatency = cellTiming.bitlineRead;
    timing.bitlineWriteLatency = cellTiming.bitlineWrite;
    timing.bitlineOnLatency = cellTiming.bitlineOn;
    timing.bitlineOffLatency = cellTiming.bitlineOff;

    const cell::OperationSet supported = memoryCell.properties().operations;
    for (cell::Operation operation : cell::allOperations) {
        if (!supported.contains(operation)) {
            continue;
        }

        timing.latency[operation] = cellTiming.latency[operation];
        switch (operation) {
            case cell::Operation::Read:
                // Reads pass the decoders, the sense amps and the output muxes after the cell
                timing.latency[operation] += readDecoderLatency + senseAmpLatency + senseAmpMuxLevel1.timing().readLatency
                        + senseAmpMuxLevel2.timing().readLatency;
                break;
            case cell::Operation::Write:
            case cell::Operation::Set:
            case cell::Operation::Reset:
            case cell::Operation::Refresh:
                // The cell's latency already includes the decoders it waits for
                break;
        }
    }

    return timing;
}

mse::component::SubArrayPower mse::component::SubArray::calculatePower() const noexcept {
    using namespace mse::unit;

    SubArrayPower power;
    // Cells with a separate read wordline read through its row decoder
    power.rowDecoderEnergy[cell::Operation::Read] = readRowDecoder ? readRowDecoder->power().dynamicReadEnergy : rowDecoder.power().dynamicReadEnergy;
    power.rowDecoderEnergy[cell::Operation::Write] = rowDecoder.power().dynamicWriteEnergy;

    Second senseAmpLatency;
    BasicPower senseAmpPower;
    if (senseAmp) {
        senseAmpLatency = senseAmp->timing().readLatency;
        senseAmpPower = senseAmp->power();
    }

    const cell::MemoryCell& memoryCell = cell::memoryCell();

    cell::MemoryCellEnergyContext context;
    context.bitlineCapacitance = _rc.bitlineCapacitance;
    context.bitlineResistance = _rc.bitlineResistance;
    context.bitlineMuxPreviousPowerCapacitance = bitlineMux.rc().powerLoadCapacitance;
    context.bitlineMuxResistance = bitlineMux.rc().passResistance;
    context.senseVoltage = _senseVoltage;
    context.senseAmpReadLatency = senseAmpLatency;
    context.rowDecoderReadDynamic = power.rowDecoderEnergy[cell::Operation::Read];
    context.numRow = rows;
    context.numColumn = columns;
    context.muxSenseAmp = _bitlineMuxInputs;
    context.muxOutputLev1 = _senseAmpMuxLevel1Inputs;
    context.muxOutputLev2 = _senseAmpMuxLevel2Inputs;

    const cell::CellPower cellPower = memoryCell.power(context);
    power.dynamicEnergy = cellPower.dynamicEnergy;
    power.cellEnergy = cellPower.cellEnergy;
    // Cells driving wordlines past vdd replace the row decoder energies
    if (cellPower.rowDecoderEnergy) {
        power.rowDecoderEnergy = *cellPower.rowDecoderEnergy;
    }

    power.leakage = memoryCell.topology().leakageCurrent() * memoryCell.properties().leakageVoltage.value_or(gTech.vdd) * columns;
    if (memoryCell.properties().columnLeak) {
        power.leakage *= rows;
    }

    if (gInputParameter.designTarget == cache && gInputParameter.cacheAccessMode != sequential_access_mode) {
        power.cellEnergy[cell::Operation::Reset] /= gInputParameter.associativity;
        power.cellEnergy[cell::Operation::Set] /= gInputParameter.associativity;
        power.dynamicEnergy[cell::Operation::Write] /= gInputParameter.associativity;
        power.dynamicEnergy[cell::Operation::Reset] /= gInputParameter.associativity;
        power.dynamicEnergy[cell::Operation::Set] /= gInputParameter.associativity;
    }

    // Each operation adds the energy of the subcomponents it passes through
    const cell::OperationSet supported = operations();
    for (cell::Operation operation : cell::allOperations) {
        if (!supported.contains(operation)) {
            continue;
        }

        Joule& energy = power.dynamicEnergy[operation];
        switch (operation) {
            case cell::Operation::Read:
                energy += power.cellEnergy[cell::Operation::Read] + power.rowDecoderEnergy[cell::Operation::Read] + bitlineMuxDecoder.power().dynamicReadEnergy
                        + senseAmpMuxLevel1Decoder.power().dynamicReadEnergy + senseAmpMuxLevel2Decoder.power().dynamicReadEnergy
                        + precharger.power().dynamicReadEnergy + bitlineMux.power().dynamicReadEnergy + senseAmpPower.dynamicReadEnergy
                        + senseAmpMuxLevel1.power().dynamicReadEnergy + senseAmpMuxLevel2.power().dynamicReadEnergy;
                break;
            case cell::Operation::Write:
                energy += power.rowDecoderEnergy[cell::Operation::Write] + bitlineMuxDecoder.power().dynamicWriteEnergy
                        + senseAmpMuxLevel1Decoder.power().dynamicWriteEnergy + senseAmpMuxLevel2Decoder.power().dynamicWriteEnergy
                        + bitlineMux.power().dynamicWriteEnergy + senseAmpPower.dynamicWriteEnergy
                        + senseAmpMuxLevel1.power().dynamicWriteEnergy + senseAmpMuxLevel2.power().dynamicWriteEnergy
                        + writeDriver.power().dynamicReadEnergy;
                if (memoryCell.properties().crosspoint) {
                    energy += bitlineMux.power().dynamicWriteEnergy + senseAmpPower.dynamicWriteEnergy
                            + senseAmpMuxLevel1.power().dynamicWriteEnergy + senseAmpMuxLevel2.power().dynamicWriteEnergy;
                }
                break;
            case cell::Operation::Set:
                energy += power.cellEnergy[cell::Operation::Set] + power.rowDecoderEnergy[cell::Operation::Set] + bitlineMuxDecoder.power().dynamicWriteEnergy
                        + senseAmpMuxLevel1Decoder.power().dynamicWriteEnergy + senseAmpMuxLevel2Decoder.power().dynamicWriteEnergy
                        + bitlineMux.power().dynamicWriteEnergy + senseAmpPower.dynamicWriteEnergy
                        + senseAmpMuxLevel1.power().dynamicWriteEnergy + senseAmpMuxLevel2.power().dynamicWriteEnergy;
                break;
            case cell::Operation::Reset:
                energy += power.cellEnergy[cell::Operation::Reset] + power.rowDecoderEnergy[cell::Operation::Reset] + bitlineMuxDecoder.power().dynamicWriteEnergy
                        + senseAmpMuxLevel1Decoder.power().dynamicWriteEnergy + senseAmpMuxLevel2Decoder.power().dynamicWriteEnergy
                        + bitlineMux.power().dynamicWriteEnergy + senseAmpPower.dynamicWriteEnergy
                        + senseAmpMuxLevel1.power().dynamicWriteEnergy + senseAmpMuxLevel2.power().dynamicWriteEnergy;
                break;
            case cell::Operation::Refresh:
                // Reading every column with the row decoder, sense amps and precharger refreshes one row
                energy += power.rowDecoderEnergy[cell::Operation::Read] + precharger.power().dynamicReadEnergy + senseAmpPower.dynamicReadEnergy;
                if (readRowDecoder) {
                    // Writing the row back drives the write wordline and the write drivers
                    energy += rowDecoder.power().dynamicReadEnergy + writeDriver.power().dynamicReadEnergy;
                }
                energy *= rows;
                break;
        }
    }

    if (readRowDecoder) {
        power.leakage += readRowDecoder->power().leakage;
    }
    power.leakage += rowDecoder.power().leakage + bitlineMuxDecoder.power().leakage + senseAmpMuxLevel1Decoder.power().leakage
            + senseAmpMuxLevel2Decoder.power().leakage + precharger.power().leakage + bitlineMux.power().leakage
            + senseAmpPower.leakage + senseAmpMuxLevel1.power().leakage + senseAmpMuxLevel2.power().leakage + writeDriver.power().leakage;
    if (buffer) {
        power.leakage += buffer->power().leakage + buffer->power().xorLeakage;
    }

    return power;
}
