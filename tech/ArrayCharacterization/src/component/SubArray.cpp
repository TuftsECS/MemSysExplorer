#include "component/SubArray.hpp"
#include "cell/visitors.hpp"
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

    const bool dramCell = gCell.memCellType == DRAM || gCell.memCellType == eDRAM
            || gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333;
    const bool resistiveCell = gCell.memCellType == MRAM || gCell.memCellType == PCRAM || gCell.memCellType == memristor
            || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM;
    // 3T cells read through their own wordline, which has its own row decoder
    const bool separateReadWordline = gCell.memCellType == eDRAM3T || gCell.memCellType == eDRAM3T333;

    // Each column holds part of the data in all the ways
    if (gInputParameter.designTarget == cache && gInputParameter.cacheAccessMode != sequential_access_mode
            && columns < gInputParameter.associativity) {
        return false;
    }

    // DRAM does not allow muxed bitlines because of its destructive readout
    if (dramCell && _bitlineMuxInputs > 1) {
        return false;
    }

    if (gCell.memCellType == SLCNAND && rows < gInputParameter.flashBlockSize / gInputParameter.pageSize) {
        return false;
    }

    // There is no room for the sense amp without a bitline mux
    const bool needsBitlineMux = gCell.memCellType == SLCNAND || gCell.memCellType == memristor || gCell.memCellType == FBRAM
            || gCell.memCellType == FeFET || gCell.memCellType == MLCFeFET || gCell.memCellType == MLCRRAM;
    if (needsBitlineMux && _internalSenseAmp && _bitlineMuxInputs < 2) {
        return false;
    }

    Ampere maxWordlineCurrent;
    if (gCell.memCellType == FBRAM) {
        // Bitline too long
        if (gCell.resistanceOff / gCell.resistanceOn < rows / constant::bitlineLeakageTolerance) {
            return false;
        }
        _maxBitlineCurrent = Ampere(std::max(gCell.resetCurrent, gCell.setCurrent) + gCell.leakageCurrentAccessDevice * (rows - 1));
    }

    if (resistiveCell) {
        if (gCell.accessType == CMOS_access) {
            // Bitline too long
            if ((gTech.currentOnNmos[temperatureIndex] / gTech.currentOffNmos[temperatureIndex]).value() < rows / constant::bitlineLeakageTolerance) {
                return false;
            }
            _maxBitlineCurrent = Ampere(std::max(gCell.resetCurrent, gCell.setCurrent) + gCell.leakageCurrentAccessDevice * (rows - 1));
        } else {
            // Write half select limits the array size
            Ampere resetCurrent(gCell.resetCurrent);
            if (gCell.resetCurrent == 0) {
                resetCurrent = Ampere((std::fabs(gCell.resetVoltage) - gCell.voltageDropAccessDevice) / gCell.resistanceOnAtResetVoltage);
            }

            const int selectedColumns = columns / _bitlineMuxInputs / _senseAmpMuxLevel1Inputs / _senseAmpMuxLevel2Inputs;
            if (gCell.accessType == none_access) {
                maxWordlineCurrent = resetCurrent * selectedColumns + resetCurrent * gCell.resistanceOnAtResetVoltage
                        / 2 / gCell.resistanceOnAtHalfResetVoltage * (columns - selectedColumns);
            } else { // Diode or BJT
                maxWordlineCurrent = resetCurrent * selectedColumns + Ampere(gCell.leakageCurrentAccessDevice)
                        * (columns - selectedColumns);
            }

            const Meter minWordlineDriverWidth(CalculateLayoutWidth((maxWordlineCurrent / gTech.currentOnNmos[temperatureIndex]).value(), gTech));
            if (minWordlineDriverWidth > Feature(gInputParameter.maxNmosSize) * gTech.featureSize) {
                return false;
            }

            if (gCell.accessType == none_access) {
                _maxBitlineCurrent = resetCurrent + resetCurrent * gCell.resistanceOnAtResetVoltage / 2
                        / gCell.resistanceOnAtHalfResetVoltage * (rows - 1);
            } else { // Diode or BJT
                _maxBitlineCurrent = resetCurrent + Ampere(gCell.leakageCurrentAccessDevice) * (rows - 1);
            }
        }
    }

    const Meter minBitlineMuxWidth = std::max(constant::minNmosSize * gTech.featureSize,
            Meter(CalculateLayoutWidth((_maxBitlineCurrent / gTech.currentOnNmos[temperatureIndex]).value(), gTech)));
    if (minBitlineMuxWidth > Feature(gInputParameter.maxNmosSize) * gTech.featureSize) {
        return false;
    }

    if (_internalSenseAmp) {
        if (resistiveCell || gCell.memCellType == FBRAM || gCell.memCellType == CTT || gCell.memCellType == MLCCTT) {
            _voltageSense = gCell.readMode;
        } else { // SRAM, DRAM, eDRAM and NAND flash
            _voltageSense = true;
        }
    } else if (dramCell) {
        fatal("DRAM does not support external sense amplifiers");
    }

    _wordlineLength = columns * Feature(gCell.widthInFeatureSize) * gTech.featureSize;
    _bitlineLength = rows * Feature(gCell.heightInFeatureSize) * gTech.featureSize;
    if (gCell.stitching) {
        _wordlineLength += ((columns - 1) / gCell.stitching + 1) * constant::stitchingOverhead * gTech.featureSize;
    }
    // Two select transistors including contacts are 5F long in total
    if (gCell.memCellType == SLCNAND) {
        const int pageCount = gInputParameter.flashBlockSize / gInputParameter.pageSize;
        _bitlineLength += (rows / pageCount) * 5_feat * gTech.featureSize;
    }
    // Access transistor
    if (gCell.memCellType == CTT || gCell.memCellType == MLCCTT) {
        _bitlineLength += 5_feat * gTech.featureSize;
    }

    const Unit<FaradAtom<>, MeterAtom<-1>> wireCapacitance{gLocalWire.capWirePerUnit};
    const Unit<OhmAtom<>, MeterAtom<-1>> wireResistance{gLocalWire.resWirePerUnit};
    _rc.wordlineCapacitance = _wordlineLength * wireCapacitance;
    _rc.wordlineResistance = _wordlineLength * wireResistance;
    _rc.bitlineCapacitance = _bitlineLength * wireCapacitance;
    _rc.bitlineResistance = _bitlineLength * wireResistance;

    const Ohm muxDecoderLoadResistance = _rc.wordlineResistance;
    Farad muxDecoderLoadCapacitance = Farad(CalculateGateCap(minBitlineMuxWidth.value(), gTech)) * columns;
    muxDecoderLoadCapacitance += _rc.wordlineCapacitance;

    if (gCell.memCellType == DRAM || gCell.memCellType == eDRAM) {
        _senseVoltage = gTech.vdd / 2 * gCell.capDRAMCell / (gCell.capDRAMCell + _rc.bitlineCapacitance.value());
        // Bitline too long
        if (_senseVoltage < Volt(gCell.minSenseVoltage)) {
            return false;
        }
    } else if (gCell.memCellType == SLCNAND) {
        // With a 0.5Vdd reference and a 0.6Vdd initial bitline, the sense amp can tell the data apart once the bitline drops to 0.4Vdd
        _senseVoltage = std::max(Volt(gCell.minSenseVoltage), 0.2 * gTech.vdd);
    } else {
        // TODO different memory technologies might have different values here
        _senseVoltage = Volt(gCell.minSenseVoltage);
    }

    if (const cell::MemoryCell* memoryCell = variantBasePtr<cell::MemoryCell>(cell)) {
        const cell::MemoryCellTopology& topology = memoryCell->topology();
        const cell::MemoryCellProperties& properties = memoryCell->properties();

        // Each cell loads its wordlines and bitlines with its largest line capacitance
        Farad cellWordlineCapacitance;
        for (int wordline = 0; wordline < static_cast<int>(topology.wordlineCount()); ++wordline) {
            cellWordlineCapacitance = std::max(cellWordlineCapacitance, topology.wordlineCapacitance(wordline));
        }
        Farad cellBitlineCapacitance;
        for (int bitline = 0; bitline < static_cast<int>(topology.bitlineCount()); ++bitline) {
            cellBitlineCapacitance = std::max(cellBitlineCapacitance, topology.bitlineCapacitance(bitline));
        }

        if (separateReadWordline) {
            // Wordline 0 reads and wordline 1 writes
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
    }

    if (!build(precharger, gTech.vdd, columns, _rc.bitlineCapacitance, _rc.bitlineResistance)) {
        return false;
    }

    // One write driver per column for a 3T row write, since the write wordline opens the whole row,
    // otherwise one per sense amp group, driving through the bitline mux
    const long long writeDrivers = separateReadWordline ? columns : columns / _bitlineMuxInputs;
    if (!build("Write Driver", writeDriver, gTech.vdd, writeDrivers, _rc.bitlineCapacitance, _rc.bitlineResistance)) {
        return false;
    }

    if (!build("Row", rowDecoder, rows, _rc.wordlineCapacitance, _rc.wordlineResistance, _multipleRowsPerSet, _areaOptimizationLevel, maxWordlineCurrent)) {
        return false;
    }

    if (separateReadWordline && !build("Read Row", readRowDecoder, rows, _rc.readWordlineCapacitance, _rc.wordlineResistance, _multipleRowsPerSet,
            _areaOptimizationLevel, maxWordlineCurrent)) {
        return false;
    }

    // TODO fix the mux decoder loads
    if (!build("Bitline Mux", bitlineMuxDecoder, _bitlineMuxInputs, muxDecoderLoadCapacitance, muxDecoderLoadResistance, false, _areaOptimizationLevel, Ampere())) {
        return false;
    }

    if (!build("Level 1 Mux", senseAmpMuxLevel1Decoder, _senseAmpMuxLevel1Inputs, muxDecoderLoadCapacitance, muxDecoderLoadResistance, false,
            _areaOptimizationLevel, Ampere())) {
        return false;
    }

    if (!build("Level 2 Mux", senseAmpMuxLevel2Decoder, _senseAmpMuxLevel2Inputs, muxDecoderLoadCapacitance, muxDecoderLoadResistance, false,
            _areaOptimizationLevel, Ampere())) {
        return false;
    }

    // TODO fix the level 2 mux loads
    if (!build("Level 2", senseAmpMuxLevel2, _senseAmpMuxLevel2Inputs, columns / _bitlineMuxInputs / _senseAmpMuxLevel1Inputs / _senseAmpMuxLevel2Inputs,
            Farad(), Farad(), _maxBitlineCurrent)) {
        return false;
    }

    if (!build("Level 1", senseAmpMuxLevel1, _senseAmpMuxLevel1Inputs, columns / _bitlineMuxInputs / _senseAmpMuxLevel1Inputs,
            senseAmpMuxLevel2.rc().delayLoadCapacitance, senseAmpMuxLevel2.rc().powerLoadCapacitance, _maxBitlineCurrent)) {
        return false;
    }

    if (_internalSenseAmp) {
        const cell::MemoryCell* memoryCell = variantBasePtr<cell::MemoryCell>(cell);
        const bool mlc = memoryCell && memoryCell->properties().isMLC;
        if (!build(senseAmp, columns / _bitlineMuxInputs, !_voltageSense, _senseVoltage, _wordlineLength / columns * _bitlineMuxInputs,
                mlc, gCell.nLvl, gCell.nFingers)) {
            return false;
        }

        if (!build("Bitline", bitlineMux, _bitlineMuxInputs, columns / _bitlineMuxInputs,
                senseAmp->loadCapacitance(), senseAmp->loadCapacitance(), _maxBitlineCurrent)) {
            return false;
        }
    } else {
        if (!build("Bitline", bitlineMux, _bitlineMuxInputs, columns / _bitlineMuxInputs,
                senseAmpMuxLevel1.rc().delayLoadCapacitance, senseAmpMuxLevel1.rc().powerLoadCapacitance, _maxBitlineCurrent)) {
            return false;
        }
    }

    // Control signals are not buffered for now
    if (gInputParameter.differentialWrite) {
        return build(buffer, 1, columns);
    }

    return true;
}

void mse::component::SubArray::report(Reporter& reporter) const noexcept {
    RampedComponent::report(reporter);

    const SubArrayTiming& timing = this->timing();
    reporter.report(ReportGroup::Timing, "Set Latency", timing.setLatency);
    reporter.report(ReportGroup::Timing, "Reset Latency", timing.resetLatency);
    reporter.report(ReportGroup::Timing, "Refresh Latency", timing.refreshLatency);
    reporter.report(ReportGroup::Timing, "Bitline Latency", timing.bitlineLatency);
    reporter.report(ReportGroup::Timing, "Charge Latency", timing.chargeLatency);
    reporter.report(ReportGroup::Timing, "Column Decoder Latency", timing.columnDecoderLatency);

    const SubArrayPower& power = this->power();
    reporter.report(ReportGroup::Power, "Set Dynamic Energy", power.setDynamicEnergy);
    reporter.report(ReportGroup::Power, "Reset Dynamic Energy", power.resetDynamicEnergy);
    reporter.report(ReportGroup::Power, "Refresh Dynamic Energy", power.refreshDynamicEnergy);
    reporter.report(ReportGroup::Power, "Cell Read Energy", power.cellReadEnergy);
    reporter.report(ReportGroup::Power, "Cell Set Energy", power.cellSetEnergy);
    reporter.report(ReportGroup::Power, "Cell Reset Energy", power.cellResetEnergy);
}

void mse::component::SubArray::reset() noexcept {
    RampedComponent::reset();
    forEachSubcomponent([] (Component& subcomponent) noexcept { subcomponent.reset(); });
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

    const Dimensions& writeDriverDimensions = writeDriver.dimensions();
    Meter writeDriverHeight;
    if (writeDriverDimensions.width > width) {
        // Assume magic folding
        writeDriverHeight = writeDriverDimensions.area() / writeDriverDimensions.width;
    } else {
        // Allow white space
        writeDriverHeight = writeDriverDimensions.height;
    }
    // 333-eDRAM puts its write drivers under the array, see below
    if (gCell.memCellType != eDRAM3T333) {
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

    if (gCell.memCellType == eDRAM3T333) {
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

    if (!std::holds_alternative<std::monostate>(cell)) {
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

        const cell::MemoryCellLatency latency = std::visit(GenericVisitor {
            MonostateThrow<cell::MemoryCellLatency>{},
            [&context] (const auto& memoryCell) -> cell::MemoryCellLatency { return memoryCell.latency(context); }
        }, cell);

        // Each cell only fills the latencies it models
        std::visit([&timing] (const auto& latency) {
            if constexpr (requires { latency.bitline; }) {
                timing.bitlineLatency = latency.bitline;
                timing.bitlineReadLatency = latency.bitline;
                timing.bitlineWriteLatency = latency.bitline;
            }
            if constexpr (requires { latency.bitlineR; }) timing.bitlineReadLatency = latency.bitlineR;
            if constexpr (requires { latency.bitlineW; }) timing.bitlineWriteLatency = latency.bitlineW;
            if constexpr (requires { latency.bitlineOn; }) timing.bitlineOnLatency = latency.bitlineOn;
            if constexpr (requires { latency.bitlineOff; }) timing.bitlineOffLatency = latency.bitlineOff;
            if constexpr (requires { latency.read; }) timing.readLatency = latency.read;
            if constexpr (requires { latency.write; }) timing.writeLatency = latency.write;
            if constexpr (requires { latency.refresh; }) timing.refreshLatency = latency.refresh;
            if constexpr (requires { latency.set; }) timing.setLatency = latency.set;
            if constexpr (requires { latency.reset; }) timing.resetLatency = latency.reset;
        }, latency);

        timing.readLatency += readDecoderLatency + senseAmpLatency + senseAmpMuxLevel1.timing().readLatency + senseAmpMuxLevel2.timing().readLatency;
    }

    return timing;
}

mse::component::SubArrayPower mse::component::SubArray::calculatePower() const noexcept {
    using namespace mse::unit;

    SubArrayPower power;
    // Cells with a separate read wordline read through its row decoder
    power.rowDecoderReadDynamicEnergy = readRowDecoder ? readRowDecoder->power().dynamicReadEnergy : rowDecoder.power().dynamicReadEnergy;
    power.rowDecoderWriteDynamicEnergy = rowDecoder.power().dynamicWriteEnergy;
    Joule rowDecoderSetDynamicEnergy;
    Joule rowDecoderResetDynamicEnergy;

    Second senseAmpLatency;
    BasicPower senseAmpPower;
    if (senseAmp) {
        senseAmpLatency = senseAmp->timing().readLatency;
        senseAmpPower = senseAmp->power();
    }

    if (const cell::MemoryCell* memoryCell = variantBasePtr<cell::MemoryCell>(cell)) {
        cell::MemoryCellEnergyContext context;
        context.bitlineCapacitance = _rc.bitlineCapacitance;
        context.bitlineResistance = _rc.bitlineResistance;
        context.bitlineMuxPreviousPowerCapacitance = bitlineMux.rc().powerLoadCapacitance;
        context.bitlineMuxResistance = bitlineMux.rc().passResistance;
        context.senseVoltage = _senseVoltage;
        context.senseAmpReadLatency = senseAmpLatency;
        context.rowDecoderReadDynamic = power.rowDecoderReadDynamicEnergy;
        context.numRow = rows;
        context.numColumn = columns;
        context.muxSenseAmp = _bitlineMuxInputs;
        context.muxOutputLev1 = _senseAmpMuxLevel1Inputs;
        context.muxOutputLev2 = _senseAmpMuxLevel2Inputs;

        const cell::MemoryCellEnergy energy = std::visit(GenericVisitor {
            MonostateThrow<cell::MemoryCellEnergy>{},
            [&context] (const auto& memoryCell) -> cell::MemoryCellEnergy { return memoryCell.energy(context); }
        }, cell);

        // Each cell only fills the energies it models
        std::visit([&power, &rowDecoderSetDynamicEnergy, &rowDecoderResetDynamicEnergy] (const auto& energy) {
            if constexpr (requires { energy.readDynamic; }) power.dynamicReadEnergy = energy.readDynamic;
            if constexpr (requires { energy.writeDynamic; }) power.dynamicWriteEnergy = energy.writeDynamic;
            if constexpr (requires { energy.setDynamic; }) power.setDynamicEnergy = energy.setDynamic;
            if constexpr (requires { energy.resetDynamic; }) power.resetDynamicEnergy = energy.resetDynamic;
            if constexpr (requires { energy.refreshDynamic; }) power.refreshDynamicEnergy = energy.refreshDynamic;
            if constexpr (requires { energy.cellRead; }) power.cellReadEnergy = energy.cellRead;
            if constexpr (requires { energy.cellSet; }) power.cellSetEnergy = energy.cellSet;
            if constexpr (requires { energy.cellReset; }) power.cellResetEnergy = energy.cellReset;
            // Cells driving wordlines past vdd replace the row decoder energies
            if constexpr (requires { energy.rowDecoderReadDynamic; }) power.rowDecoderReadDynamicEnergy = energy.rowDecoderReadDynamic;
            if constexpr (requires { energy.rowDecoderWriteDynamic; }) power.rowDecoderWriteDynamicEnergy = energy.rowDecoderWriteDynamic;
            if constexpr (requires { energy.rowDecoderSetDynamic; }) rowDecoderSetDynamicEnergy = energy.rowDecoderSetDynamic;
            if constexpr (requires { energy.rowDecoderResetDynamic; }) rowDecoderResetDynamicEnergy = energy.rowDecoderResetDynamic;
        }, energy);

        power.leakage = memoryCell->topology().leakageCurrent() * memoryCell->properties().leakageVoltage.value_or(gTech.vdd) * columns;
        if (memoryCell->properties().columnLeak) {
            power.leakage *= rows;
        }
    }

    if (gInputParameter.designTarget == cache && gInputParameter.cacheAccessMode != sequential_access_mode) {
        power.cellResetEnergy /= gInputParameter.associativity;
        power.cellSetEnergy /= gInputParameter.associativity;
        power.dynamicWriteEnergy /= gInputParameter.associativity;
        power.resetDynamicEnergy /= gInputParameter.associativity;
        power.setDynamicEnergy /= gInputParameter.associativity;
    }

    power.dynamicReadEnergy += power.cellReadEnergy + power.rowDecoderReadDynamicEnergy + bitlineMuxDecoder.power().dynamicReadEnergy
            + senseAmpMuxLevel1Decoder.power().dynamicReadEnergy + senseAmpMuxLevel2Decoder.power().dynamicReadEnergy
            + precharger.power().dynamicReadEnergy + bitlineMux.power().dynamicReadEnergy + senseAmpPower.dynamicReadEnergy
            + senseAmpMuxLevel1.power().dynamicReadEnergy + senseAmpMuxLevel2.power().dynamicReadEnergy;
    power.dynamicWriteEnergy += power.rowDecoderWriteDynamicEnergy + bitlineMuxDecoder.power().dynamicWriteEnergy
            + senseAmpMuxLevel1Decoder.power().dynamicWriteEnergy + senseAmpMuxLevel2Decoder.power().dynamicWriteEnergy
            + bitlineMux.power().dynamicWriteEnergy + senseAmpPower.dynamicWriteEnergy
            + senseAmpMuxLevel1.power().dynamicWriteEnergy + senseAmpMuxLevel2.power().dynamicWriteEnergy
            + writeDriver.power().dynamicReadEnergy;

    // Reading every column with the row decoder, sense amps and precharger refreshes one row
    power.refreshDynamicEnergy += power.rowDecoderReadDynamicEnergy + precharger.power().dynamicReadEnergy + senseAmpPower.dynamicReadEnergy;
    if (readRowDecoder) {
        // Writing the row back drives the write wordline and the write drivers
        power.refreshDynamicEnergy += rowDecoder.power().dynamicReadEnergy + writeDriver.power().dynamicReadEnergy;
    }
    power.refreshDynamicEnergy *= rows;

    // Asymmetric set and reset only
    power.setDynamicEnergy += power.cellSetEnergy + rowDecoderSetDynamicEnergy + bitlineMuxDecoder.power().dynamicWriteEnergy
            + senseAmpMuxLevel1Decoder.power().dynamicWriteEnergy + senseAmpMuxLevel2Decoder.power().dynamicWriteEnergy
            + bitlineMux.power().dynamicWriteEnergy + senseAmpPower.dynamicWriteEnergy
            + senseAmpMuxLevel1.power().dynamicWriteEnergy + senseAmpMuxLevel2.power().dynamicWriteEnergy;
    power.resetDynamicEnergy += power.setDynamicEnergy + rowDecoderResetDynamicEnergy + bitlineMuxDecoder.power().dynamicWriteEnergy
            + senseAmpMuxLevel1Decoder.power().dynamicWriteEnergy + senseAmpMuxLevel2Decoder.power().dynamicWriteEnergy
            + bitlineMux.power().dynamicWriteEnergy + senseAmpPower.dynamicWriteEnergy
            + senseAmpMuxLevel1.power().dynamicWriteEnergy + senseAmpMuxLevel2.power().dynamicWriteEnergy;

    if (gCell.accessType == diode_access || gCell.accessType == none_access) {
        power.dynamicWriteEnergy += bitlineMux.power().dynamicWriteEnergy + senseAmpPower.dynamicWriteEnergy
                + senseAmpMuxLevel1.power().dynamicWriteEnergy + senseAmpMuxLevel2.power().dynamicWriteEnergy;
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
